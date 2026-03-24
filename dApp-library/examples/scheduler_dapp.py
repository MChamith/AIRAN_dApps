#!/usr/bin/env python3
"""
Example script demonstrating the Scheduler dApp usage.

This script initializes the scheduler dApp, sets up the E3 connection,
subscribes to scheduler statistics, and runs indefinitely until interrupted.

The dApp will:
- Receive scheduler statistics from the RAN (frame, slot, policy, per-UE stats)
- Display formatted statistics in the terminal
- Provide REST API for policy control via interactive_control.py
"""

import sys
import signal
import time

# Add parent directory to path to import dApp modules
sys.path.insert(0, '../src')

from scheduler.scheduler_dapp import SchedulerDApp
from e3interface.e3_logging import dapp_logger


def signal_handler(sig, frame):
    """Handle Ctrl+C gracefully"""
    print("\n\nShutting down scheduler dApp...")
    sys.exit(0)


def main():
    """Main function to run the scheduler dApp"""
    
    # Register signal handler for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    
    print("="*80)
    print("Scheduler dApp Example")
    print("="*80)
    print("This dApp will:")
    print("  - Receive scheduler statistics from RAN")
    print("  - Display formatted statistics (throughput, BLER, MCS, etc.)")
    print("  - Expose REST API for policy control")
    print("  - Policies: 0=Round Robin, 1=Proportional Fair")
    print("  - Policy changes via REST API (use interactive_control.py)")
    print("\nPress Ctrl+C to stop")
    print("="*80 + "\n")
    
    try:
        # Initialize the scheduler dApp with REST API support
        dapp_logger.info("Initializing Scheduler dApp...")
        scheduler_dapp = SchedulerDApp(
            id=1,                      # dApp identifier
            link='posix',              # Connection layer
            transport='ipc',           # Transport protocol
            encoding_method='asn1',    # Use ASN.1 encoding
            # REST API configuration
            enable_rest_api=True,
            rest_api_port=8080
        )
        
        # Perform E3 setup exchange
        dapp_logger.info("Performing E3 setup exchange...")
        success, ran_functions = scheduler_dapp.setup_connection()
        
        if not success:
            dapp_logger.error("Failed to setup E3 connection")
            return 1
        
        dapp_logger.info(f"E3 setup successful. Available RAN functions: {ran_functions}")
        
        # Subscribe to scheduler RAN function (ID: 301)
        dapp_logger.info(f"Subscribing to RAN function {SchedulerDApp.RAN_FUNCTION_ID}...")
        scheduler_dapp.send_subscription_request(
            ranFunctionIds=[SchedulerDApp.RAN_FUNCTION_ID],
            actionType="insert"
        )
        
        dapp_logger.info("Subscription sent successfully")
        
        # Start the dApp control loop
        dapp_logger.info("Starting scheduler dApp control loop...")
        print("\n>>> Scheduler dApp is running and waiting for statistics...\n")
        
        # Start the dApp control loop (this will run indefinitely until Ctrl+C)
        scheduler_dapp.control_loop()
        
    except KeyboardInterrupt:
        print("\n\nReceived interrupt signal")
    except Exception as e:
        dapp_logger.exception(f"Error in main: {e}")
        return 1
    finally:
        # Clean shutdown
        try:
            dapp_logger.info("Stopping scheduler dApp...")
            scheduler_dapp.stop()
            dapp_logger.info("Scheduler dApp stopped successfully")
        except:
            pass
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
