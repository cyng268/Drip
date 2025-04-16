#!/usr/bin/env python3
import serial
import time
import binascii
import logging
import readline
import argparse
import os
from typing import Optional, List

# Import the camera control class from our existing script
from camera_control import CameraControl

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class InteractiveConsole:
    def __init__(self, camera: CameraControl):
        self.camera = camera
        self.running = True
        self.command_history = []
        self.history_file = os.path.expanduser("~/.camera_history")
        
        # Load command history if it exists
        try:
            readline.read_history_file(self.history_file)
        except FileNotFoundError:
            pass
            
        # Common camera commands for quick reference
        self.common_commands = {
            "zoom0": "8101044700000000FF",  # Zoom level 0 (min)
            "zoom1": "8101044700000100FF",  # Zoom level 1
            "zoom_max": "8101044701000000FF",  # Zoom max
            "icr_on": "8101040102FF",       # ICR on
            "icr_off": "8101040103FF",      # ICR off
            "ir_on": "8101041101FF",        # IR correction on
            "ir_off": "8101041100FF",       # IR correction off
            "init": "8101044700000000FF",   # Initialize camera
        }
    
    def show_help(self):
        """Display help information"""
        print("\nCamera Control Interactive Console")
        print("----------------------------------")
        print("Commands:")
        print("  help - Show this help message")
        print("  exit/quit - Exit the program")
        print("  show - Show available preset commands")
        print("  <hex_command> - Send a hex command to the camera")
        print("  !<preset_name> - Execute a preset command")
        print("\nExamples:")
        print("  8101044700000000FF  - Set zoom level to 0")
        print("  !zoom0             - Same as above using preset")
        print("  !icr_on            - Turn on ICR mode")
        print("\n")
    
    def show_presets(self):
        """Display available preset commands"""
        print("\nAvailable preset commands:")
        for name, command in self.common_commands.items():
            print(f"  !{name:<10} - {command}")
        print("\n")
        
    def save_history(self):
        """Save command history to file"""
        try:
            readline.write_history_file(self.history_file)
        except:
            logger.warning("Failed to save command history")
    
    def validate_hex(self, hex_string: str) -> bool:
        """Validate hex string format"""
        # Remove spaces
        hex_string = hex_string.replace(" ", "")
        
        # Check if all characters are valid hex
        if not all(c in "0123456789ABCDEFabcdef" for c in hex_string):
            return False
            
        # Check if length is even (required for hexlify)
        if len(hex_string) % 2 != 0:
            return False
            
        return True
    
    def run(self):
        """Run the interactive console"""
        self.show_help()
        
        while self.running:
            try:
                # Get user input
                user_input = input("\nEnter hex command (help for info): ").strip()
                
                # Process special commands
                if user_input.lower() in ["exit", "quit"]:
                    self.running = False
                    continue
                    
                elif user_input.lower() == "help":
                    self.show_help()
                    continue
                    
                elif user_input.lower() == "show":
                    self.show_presets()
                    continue
                
                # Check for preset command
                if user_input.startswith("!"):
                    preset_name = user_input[1:]
                    if preset_name in self.common_commands:
                        user_input = self.common_commands[preset_name]
                        print(f"Using preset: {user_input}")
                    else:
                        print(f"Preset '{preset_name}' not found. Use 'show' to see available presets.")
                        continue
                
                # Validate and send the command
                if not self.validate_hex(user_input):
                    print("Invalid hex format. Please use only hex characters (0-9, A-F).")
                    continue
                
                # Send the command
                if not self.camera.send_hex_command(user_input):
                    print("Failed to send command.")
                else:
                    # Add to history
                    self.command_history.append(user_input)
                
            except KeyboardInterrupt:
                print("\nExiting...")
                self.running = False
            except Exception as e:
                logger.error(f"Error: {e}")
        
        # Clean up
        self.save_history()
        self.camera.close()
        print("Session ended.")

def main():
    parser = argparse.ArgumentParser(description='Interactive camera control console')
    parser.add_argument('--port', help='Serial port to use')
    parser.add_argument('--baudrate', type=int, default=9600, help='Baudrate for serial communication')
    
    args = parser.parse_args()
    
    # Create camera control instance
    camera = CameraControl(args.port, args.baudrate)
    
    # Initialize serial connection
    if not camera.initialize_serial():
        logger.error("Failed to initialize serial connection")
        return
        
    # Create and run the interactive console
    console = InteractiveConsole(camera)
    console.run()

if __name__ == "__main__":
    main()
