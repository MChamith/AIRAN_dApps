#!/usr/bin/env python3
"""
dApp for Scheduler Control
"""

__author__ = "Scheduler dApp Team"

import time
import os
import random
import asn1tools
import threading
from typing import Optional

from dapp.dapp import DApp
from e3interface.e3_logging import dapp_logger

# Optional Flask support for REST API
try:
    from flask import Flask, request, jsonify
    from flask_cors import CORS
    FLASK_AVAILABLE = True
except ImportError:
    FLASK_AVAILABLE = False
    dapp_logger.warning("Flask not available. Install with: pip install 'dapps[gui]' flask-cors")


class SchedulerDApp(DApp):
    """
    Scheduler dApp for dynamic scheduling policy control.
    
    Receives scheduler statistics from RAN and can send policy control messages
    to switch between Round Robin and Proportional Fair scheduling policies.
    """
    
    # RAN Function ID for scheduler service model
    RAN_FUNCTION_ID = 2
    
    # Scheduling policy constants
    POLICY_ROUND_ROBIN = 0
    POLICY_PROPORTIONAL_FAIR = 1
    
    # Policy names for display
    POLICY_NAMES = {
        POLICY_ROUND_ROBIN: "Round Robin",
        POLICY_PROPORTIONAL_FAIR: "Proportional Fair"
    }
    
    def __init__(self, id: int = 1, link: str = 'posix', transport: str = 'ipc', 
                 encoding_method: str = "asn1",
                 enable_rest_api: bool = False,
                 rest_api_port: int = 8080,
                 **kwargs):
        """
        Initialize the Scheduler dApp.
        
        Args:
            id: dApp identifier (unique per dApp)
            link: Connection layer ('posix', 'zmq', etc.)
            transport: Transport protocol ('ipc', 'tcp', etc.)
            encoding_method: Message encoding method ('asn1' or 'json')
            enable_rest_api: Enable Flask REST API for policy control
            rest_api_port: Port for REST API server
            **kwargs: Additional arguments passed to base class
        """
        super().__init__(id=id, link=link, transport=transport, 
                        encoding_method=encoding_method, **kwargs)
        
        # Initialize scheduler encoder based on encoding method
        self._init_scheduler_encoder()
        
        # Register callback for receiving scheduler statistics
        self.e3_interface.add_callback(self.dapp_id, self.get_stats_from_ran)
        
        # Current policy tracking
        self.current_policy = self.POLICY_ROUND_ROBIN
        
        # Flask REST API setup
        self.flask_app = None
        self.flask_thread = None
        
        if FLASK_AVAILABLE and enable_rest_api:
            self._setup_flask_api(rest_api_port)
        
        dapp_logger.info(f"Scheduler dApp initialized (encoding: {encoding_method})")
    
    def _setup_flask_api(self, port: int):
        """Setup Flask REST API for policy control"""
        self.flask_app = Flask(__name__)
        try:
            CORS(self.flask_app)
        except:
            pass  # CORS is optional
        
        @self.flask_app.route('/policy', methods=['GET'])
        def get_policy():
            """Get current scheduling policy"""
            return jsonify({
                'policy': self.current_policy,
                'policy_name': self.POLICY_NAMES.get(self.current_policy, 'Unknown')
            })
        
        @self.flask_app.route('/policy', methods=['POST'])
        def set_policy():
            """Set scheduling policy"""
            data = request.get_json()
            if not data or 'policy' not in data:
                return jsonify({'error': 'Missing policy parameter'}), 400
            
            policy = data['policy']
            if policy not in [self.POLICY_ROUND_ROBIN, self.POLICY_PROPORTIONAL_FAIR]:
                return jsonify({'error': 'Invalid policy value. Use 0 (RR) or 1 (PF)'}), 400
            
            try:
                # Create and send policy control message
                control_payload = self.create_policy_control(policy=policy)
                self.e3_interface.schedule_control(
                    dappId=self.dapp_id,
                    ranFunctionId=self.RAN_FUNCTION_ID,
                    actionData=control_payload
                )
                
                dapp_logger.info(f"Policy change requested via REST API: {self.POLICY_NAMES[policy]}")
                return jsonify({
                    'success': True,
                    'policy': policy,
                    'policy_name': self.POLICY_NAMES[policy]
                })
            except Exception as e:
                dapp_logger.error(f"Failed to set policy: {e}")
                return jsonify({'error': str(e)}), 500
        
        # Start Flask in daemon thread
        self.flask_thread = threading.Thread(
            target=lambda: self.flask_app.run(host='0.0.0.0', port=port, debug=False, use_reloader=False),
            daemon=True
        )
        self.flask_thread.start()
        dapp_logger.info(f"Flask REST API started on port {port}")
    
    def _init_scheduler_encoder(self):
        """Initialize the scheduler encoder based on the encoding method"""
        if self.encoding_method == "asn1":
            asn_file_path = os.path.join(os.path.dirname(__file__), "defs", "e3sm_scheduler.asn")
            self.scheduler_encoder = asn1tools.compile_files(asn_file_path, codec="per")
            dapp_logger.debug(f"Scheduler ASN.1 encoder initialized from {asn_file_path}")
        elif self.encoding_method == "json":
            self.scheduler_encoder = None
            dapp_logger.error("JSON encoding not yet implemented for scheduler SM")
            raise NotImplementedError("JSON encoding not yet implemented for scheduler SM")
        else:
            raise ValueError(f"Unsupported encoding method: {self.encoding_method}")
    
    def decode_stats_indication(self, data: bytes) -> dict:
        """
        Decode a scheduler statistics indication message.
        
        Args:
            data: Encoded bytes from E3-IndicationMessage.protocolData
            
        Returns:
            Dictionary containing:
                - currentPolicy (int): Current scheduling policy (0=RR, 1=PF)
                - frameNumber (int): Frame number (0-1023)
                - slotNumber (int): Slot number (0-319)
                - numUEs (int): Number of UEs
                - ueStats (list): List of per-UE statistics dictionaries
        """
        return self._decode_scheduler_message("SchedulerPolicyIndication", data)
    
    def create_policy_control(self, policy: int, sampling_threshold: int = None) -> bytes:
        """
        Create a scheduler policy control message.
        
        Args:
            policy: Requested scheduling policy (0=RR, 1=PF)
            sampling_threshold: Optional sampling threshold to control reporting frequency
            
        Returns:
            Encoded bytes for E3-ControlAction.actionData
        """
        control_data = {
            "requestedPolicy": policy
        }
        
        if sampling_threshold is not None:
            control_data["samplingThreshold"] = sampling_threshold
        
        dapp_logger.debug(f"Creating policy control: {control_data}")
        
        return self._encode_scheduler_message("SchedulerPolicyControl", control_data)
    
    def _encode_scheduler_message(self, message_type: str, data: dict) -> bytes:
        """
        Encode a scheduler message using the configured encoding method.
        
        Args:
            message_type: The scheduler message type to encode
            data: The data dictionary to encode
            
        Returns:
            Encoded bytes
        """
        if self.encoding_method == "asn1":
            if self.scheduler_encoder is None:
                raise RuntimeError("ASN.1 encoder not initialized")
            return self.scheduler_encoder.encode(message_type, data)
        elif self.encoding_method == "json":
            raise NotImplementedError("JSON encoding for scheduler SM not implemented yet")
        else:
            raise ValueError(f"Unsupported encoding method: {self.encoding_method}")
    
    def _decode_scheduler_message(self, message_type: str, data: bytes) -> dict:
        """
        Decode a scheduler message using the configured encoding method.
        
        Args:
            message_type: The scheduler message type to decode
            data: The encoded bytes to decode
            
        Returns:
            Decoded data dictionary
        """
        if self.encoding_method == "asn1":
            if self.scheduler_encoder is None:
                raise RuntimeError("ASN.1 encoder not initialized")
            return self.scheduler_encoder.decode(message_type, data)
        elif self.encoding_method == "json":
            raise NotImplementedError("JSON encoding for scheduler SM not implemented yet")
        else:
            raise ValueError(f"Unsupported encoding method: {self.encoding_method}")
    
    def get_stats_from_ran(self, dapp_identifier, data):
        """
        Callback function to receive scheduler statistics from RAN.
        
        This function is called automatically when scheduler statistics are received.
        It decodes the message and displays formatted statistics.
        Policy changes are only triggered via the REST API from the frontend.
        
        Args:
            dapp_identifier: The dApp ID
            data: Raw protocolData bytes from E3-IndicationMessage
        """
        dapp_logger.debug(f'Triggered callback for dApp {dapp_identifier}')
        
        try:
            # Decode the indication message
            stats = self.decode_stats_indication(data)
            
            # Update current policy
            self.current_policy = stats["currentPolicy"]
            
            # Extract basic information
            current_policy = stats["currentPolicy"]
            frame_number = stats["frameNumber"]
            slot_number = stats["slotNumber"]
            num_ues = stats["numUEs"]
            ue_stats_list = stats["ueStats"]
            
            policy_name = self.POLICY_NAMES.get(current_policy, f"Unknown({current_policy})")
            
            # Print formatted header
            print("\n" + "="*80)
            print(f"SCHEDULER STATISTICS | Frame: {frame_number} | Slot: {slot_number} | "
                  f"Policy: {policy_name} | UEs: {num_ues}")
            print("="*80)
            
            if num_ues > 0 and len(ue_stats_list) > 0:
                # Print table header
                print(f"{'RNTI':<8} {'Tput(Mbps)':<12} {'BLER(%)':<10} {'MCS':<6} "
                      f"{'Pend(KB)':<10} {'ReTx':<6} {'RBs':<6} {'Beam':<8}")
                print("-" * 80)
                
                # Print per-UE statistics
                for ue_stat in ue_stats_list:
                    rnti = ue_stat["rnti"]
                    throughput_mbps = ue_stat["avgThroughput"] / 1e6  # Convert bps to Mbps
                    bler_percent = ue_stat["bler"] / 100.0  # Convert scaled value to percentage
                    mcs = ue_stat["currentMCS"]
                    pending_kb = ue_stat["pendingBytes"] / 1024.0  # Convert bytes to KB
                    is_retx = "Yes" if ue_stat["isRetx"] else "No"
                    rbs = ue_stat["rbsAllocated"]
                    beam = ue_stat["beamIndex"]
                    
                    print(f"{rnti:<8} {throughput_mbps:<12.2f} {bler_percent:<10.2f} {mcs:<6} "
                          f"{pending_kb:<10.1f} {is_retx:<6} {rbs:<6} {beam:<8}")
            else:
                print("No active UEs")
            
            print("="*80 + "\n")
        
        except Exception as e:
            dapp_logger.exception(f"Error processing scheduler statistics: {e}")
    
    def _control_loop(self):
        """
        Minimal control loop implementation.
        
        Since all logic is event-driven through callbacks, this just sleeps
        to avoid busy-waiting.
        """
        time.sleep(0.1)
    
    def _stop(self):
        """
        Cleanup method called when the dApp is stopping.
        """
        dapp_logger.info("Scheduler dApp cleanup completed")
