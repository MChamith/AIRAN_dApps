#!/usr/bin/env python3
"""
Interactive terminal control for scheduler policy.

Simple chat-like interface for changing policies:
- Type "pf" for Proportional Fair
- Type "rr" for Round Robin
- Type "get" to see current policy
- Type "help" for commands
- Type "exit" to quit

Natural language support:
- Type "change: <scenario>" for intent-based policy selection
"""

import requests
import sys
import time
import json
import os
import argparse
from openai import OpenAI
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

DAPP_URL = "http://localhost:8080"

# System prompt for LLM intent recognition
SYSTEM_PROMPT = """You are a wireless network scheduling policy advisor. Your task is to analyze network scenarios and recommend the best scheduling policy.

Available policies:
- Round Robin (RR): Allocates resources equally to all users in rotation, ensuring fairness regardless of channel conditions. Best for scenarios requiring equal treatment, many active users, or when fairness is prioritized over throughput.
- Proportional Fair (PF): Allocates resources based on channel quality and historical throughput, maximizing overall network efficiency. Best for scenarios with varying channel conditions, high-priority throughput, or when network capacity is critical.

Guidelines:
- Use Round Robin when: scenario emphasizes fairness, equal access, many competing users, or congestion
- Use Proportional Fair when: scenario emphasizes efficiency, throughput, quality-sensitive applications, or varying channel conditions

Respond ONLY with valid JSON in this exact format:
{"policy": 0, "reasoning": "brief explanation"}

Where policy is: 0 for Round Robin, 1 for Proportional Fair

Examples:
User: "Many users in a crowded area need internet access"
Response: {"policy": 0, "reasoning": "High user count requires fair distribution to prevent starvation"}

User: "Video streaming service needs consistent quality"
Response: {"policy": 1, "reasoning": "Quality-sensitive application benefits from channel-aware allocation"}

User: "Emergency scenario with limited bandwidth"
Response: {"policy": 0, "reasoning": "Fair allocation ensures all users get equal emergency access"}"""

# Simple command mapping
COMMANDS = {
    'pf': (1, 'Proportional Fair'),
    'proportional': (1, 'Proportional Fair'),
    'fair': (1, 'Proportional Fair'),
    'rr': (0, 'Round Robin'),
    'robin': (0, 'Round Robin'),
    'round': (0, 'Round Robin'),
}

def print_header(llm_config=None):
    """Print welcome header"""
    llm_backend = "LLM" if llm_config is None else ("OpenAI ChatGPT" if llm_config['type'] == 'openai' else "NVIDIA NIM")
    
    print("\n" + "="*70)
    print("  📡 Scheduler Policy Control - Interactive Terminal")
    print("="*70)
    print("\nCommands:")
    print("  pf, proportional, fair  → Switch to Proportional Fair")
    print("  rr, robin, round        → Switch to Round Robin")
    print("  get                     → Show current policy")
    print("  help                    → Show this help")
    print("  exit, quit              → Exit")
    print(f"\nNatural Language (with {llm_backend}):")
    print("  change: <scenario>      → Intent-based policy selection")
    print("  Example: change: many users need internet access")
    if llm_config:
        print("\nCLI Options:")
        print("  --model {openai,nim}    → Select LLM backend (default: openai)")
        print("  --modelname <name>      → Specify model name")
        print("  --nim-url <url>         → Custom NIM endpoint URL")
    print("="*70 + "\n")

def build_llm_config(args):
    """Build LLM configuration from CLI args and environment variables.
    
    Args:
        args: Parsed command line arguments
        
    Returns:
        dict: Configuration with 'type', 'base_url', 'model', 'api_key'
    """
    config = {'type': args.model}
    
    if args.model == 'openai':
        # OpenAI configuration
        config['base_url'] = 'https://api.openai.com/v1'
        config['model'] = args.modelname if args.modelname else 'gpt-4'
        config['api_key'] = os.getenv('OPENAI_API_KEY', '')
    else:  # nim
        # NIM configuration with environment variable fallbacks
        if args.nim_url:
            config['base_url'] = args.nim_url
        else:
            config['base_url'] = os.getenv('NIM_BASE_URL', 'http://localhost:8000/v1')
        
        if args.modelname:
            config['model'] = args.modelname
        else:
            config['model'] = os.getenv('NIM_MODEL', 'meta/llama-3.1-8b-instruct')
        
        # For NIM, API key is optional (check NGC_API_KEY or NIM_API_KEY)
        config['api_key'] = os.getenv('NGC_API_KEY', os.getenv('NIM_API_KEY', 'not-needed'))
    
    return config

def check_nim_connection(base_url):
    """Check if NIM endpoint is reachable.
    
    Args:
        base_url: The NIM base URL to check
        
    Returns:
        bool: True if connection successful, False otherwise
    """
    print("Checking connection to NIM endpoint...", end=" ")
    try:
        # Try to get models endpoint
        models_url = base_url.rstrip('/v1') + '/v1/models'
        response = requests.get(models_url, timeout=5)
        
        if response.status_code == 200:
            print("✓ Connected")
            return True
        else:
            # Some NIM deployments might not have /models endpoint, try health check
            health_url = base_url.rstrip('/v1') + '/health'
            health_response = requests.get(health_url, timeout=5)
            if health_response.status_code == 200:
                print("✓ Connected")
                return True
            print("❌ Failed")
            return False
    except requests.exceptions.RequestException as e:
        print("❌ Failed")
        print(f"\n⚠️  Cannot connect to NIM endpoint: {base_url}")
        print("   Please make sure NIM container is running:")
        print("   $ docker run --gpus=all -e NGC_API_KEY=$NGC_API_KEY \\")
        print("       -v \"$LOCAL_NIM_CACHE:/opt/nim/.cache\" -p 8000:8000 \\")
        print("       nvcr.io/nim/meta/llama-3.1-8b-instruct:2.0.1")
        print(f"\n   Error: {e}\n")
        return False

def llm_parse_intent(scenario, llm_config):
    """Use LLM to parse natural language intent and return policy number.
    
    Args:
        scenario: Natural language description of the network scenario
        llm_config: LLM configuration dict with 'type', 'base_url', 'model', 'api_key'
        
    Returns:
        tuple: (policy_number, policy_name, reasoning) or (None, None, error_msg)
    """
    try:
        # Check for API key (required for OpenAI, optional for NIM)
        api_key = llm_config['api_key']
        if llm_config['type'] == 'openai' and (not api_key or api_key == 'your_openai_api_key_here'):
            return (None, None, "OpenAI API key not configured. Please set OPENAI_API_KEY in .env file")
        
        # Initialize OpenAI client with custom base_url for NIM support
        client = OpenAI(
            api_key=api_key,
            base_url=llm_config['base_url']
        )
        
        model_name = llm_config['model']
        backend = "OpenAI ChatGPT" if llm_config['type'] == 'openai' else "NIM"
        print(f"🤖 Analyzing scenario with {backend} ({model_name})...", end=" ")
        
        # Make API call
        response = client.chat.completions.create(
            model=model_name,
            messages=[
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": scenario}
            ],
            temperature=0.3,
            max_tokens=150
        )
        
        # Parse response
        content = response.choices[0].message.content.strip()
        
        # Try to extract JSON from response
        try:
            # Handle potential markdown code blocks
            if '```json' in content:
                content = content.split('```json')[1].split('```')[0].strip()
            elif '```' in content:
                content = content.split('```')[1].split('```')[0].strip()
            
            result = json.loads(content)
            policy = result.get('policy')
            reasoning = result.get('reasoning', 'No reasoning provided')
            
            # Validate policy
            if policy not in [0, 1]:
                return (None, None, f"Invalid policy number: {policy}")
            
            policy_name = "Round Robin" if policy == 0 else "Proportional Fair"
            print("✓")
            print(f"   💡 Recommendation: {policy_name}")
            print(f"   📝 Reasoning: {reasoning}")
            
            return (policy, policy_name, reasoning)
            
        except json.JSONDecodeError as e:
            return (None, None, f"Failed to parse LLM response: {content}")
            
    except Exception as e:
        backend = llm_config.get('type', 'LLM').upper()
        return (None, None, f"{backend} API error: {str(e)}")

def get_current_policy():
    """Get and display current policy"""
    try:
        response = requests.get(f"{DAPP_URL}/policy", timeout=5)
        response.raise_for_status()
        data = response.json()
        policy_name = data['policy_name']
        policy_num = data['policy']
        print(f"✓ Current Policy: {policy_name} (policy={policy_num})")
        return policy_num
    except requests.exceptions.RequestException as e:
        print(f"❌ Error: Cannot connect to dApp")
        print(f"   Make sure scheduler_dapp.py is running with REST API on port 8081")
        return None

def set_policy(policy, policy_name):
    """Set policy via REST API"""
    try:
        response = requests.post(
            f"{DAPP_URL}/policy",
            json={'policy': policy},
            timeout=5
        )
        response.raise_for_status()
        print(f"✓ Policy changed to: {policy_name} (policy={policy})")
        return True
    except requests.exceptions.RequestException as e:
        print(f"❌ Error: Failed to change policy")
        print(f"   Details: {e}")
        return False

def parse_input(user_input, llm_config):
    """Parse user input and execute command"""
    user_input_stripped = user_input.strip()
    user_input_lower = user_input_stripped.lower()
    
    if not user_input_stripped:
        return True
    
    # Exit commands
    if user_input_lower in ['exit', 'quit', 'q']:
        print("\n👋 Goodbye!\n")
        return False
    
    # Help command
    if user_input_lower in ['help', '?', 'h']:
        print_header(llm_config)
        return True
    
    # Get command
    if user_input_lower in ['get', 'current', 'status', 'show']:
        get_current_policy()
        return True
    
    # Natural language intent-based command
    if user_input_lower.startswith('change:'):
        scenario = user_input_stripped[7:].strip()  # Remove 'change:' prefix
        
        if not scenario:
            print("❌ Please provide a scenario description")
            print("   Example: change: many users need internet access")
            return True
        
        # Use LLM to parse intent
        policy, policy_name, reasoning = llm_parse_intent(scenario, llm_config)
        
        if policy is None:
            print(f"❌ {reasoning}")
            print("   Falling back to keyword commands (pf, rr)")
            return True
        
        # Apply the recommended policy
        set_policy(policy, policy_name)
        return True
    
    # Check for policy change commands (keyword matching)
    for keyword, (policy, policy_name) in COMMANDS.items():
        if keyword in user_input_lower:
            set_policy(policy, policy_name)
            return True
    
    # Unknown command
    print(f"❓ Unknown command: '{user_input_stripped}'")
    print("   Type 'help' to see available commands")
    return True

def check_dapp_connection():
    """Check if dApp is reachable"""
    print("Checking connection to scheduler dApp...", end=" ")
    try:
        response = requests.get(f"{DAPP_URL}/policy", timeout=2)
        response.raise_for_status()
        print("✓ Connected")
        return True
    except requests.exceptions.RequestException:
        print("❌ Failed")
        print("\n⚠️  Cannot connect to dApp REST API")
        print("   Please start the scheduler dApp first:")
        print("   $ python scheduler_dapp.py")
        print("\n   Make sure REST API is enabled on port 8080\n")
        return False

def main():
    """Main interactive loop"""
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='Interactive scheduler policy control with LLM-based intent recognition',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  # Use OpenAI ChatGPT (default)
  python interactive_control.py --model openai
  
  # Use NVIDIA NIM with default settings
  python interactive_control.py --model nim
  
  # Use NIM with custom model
  python interactive_control.py --model nim --modelname meta/llama-3.1-70b-instruct
  
  # Use NIM with custom endpoint
  python interactive_control.py --model nim --nim-url http://192.168.1.100:8000/v1
        """
    )
    parser.add_argument(
        '--model',
        choices=['openai', 'nim'],
        default='openai',
        help='LLM backend to use (default: openai)'
    )
    parser.add_argument(
        '--modelname',
        type=str,
        help='Model name (e.g., gpt-4, meta/llama-3.1-8b-instruct)'
    )
    parser.add_argument(
        '--nim-url',
        type=str,
        help='Custom NIM endpoint URL (default: http://localhost:8000/v1)'
    )
    
    args = parser.parse_args()
    
    # Build LLM configuration
    llm_config = build_llm_config(args)
    
    print_header(llm_config)
    
    # Show LLM configuration
    print("LLM Configuration:")
    print(f"  Backend: {llm_config['type'].upper()}")
    print(f"  Model: {llm_config['model']}")
    print(f"  Endpoint: {llm_config['base_url']}")
    print()
    
    # Check NIM connection if using NIM
    if llm_config['type'] == 'nim':
        if not check_nim_connection(llm_config['base_url']):
            print("\n⚠️  Warning: NIM endpoint not reachable. Natural language commands may fail.")
            print("   You can still use keyword commands (pf, rr, get, etc.)\n")
    
    # Check dApp connection
    if not check_dapp_connection():
        sys.exit(1)
    
    # Show initial policy
    print("\nInitial state:")
    get_current_policy()
    print()
    
    # Interactive loop
    try:
        while True:
            try:
                user_input = input("📡 > ").strip()
                
                if not parse_input(user_input, llm_config):
                    break
                    
            except EOFError:
                print("\n\n👋 Goodbye!\n")
                break
                
    except KeyboardInterrupt:
        print("\n\n👋 Goodbye!\n")
        sys.exit(0)

if __name__ == '__main__':
    main()
