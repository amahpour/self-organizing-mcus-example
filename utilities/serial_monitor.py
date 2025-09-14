#!/usr/bin/env python3
"""
Multi-Serial Monitor for ATmega328P Debug Logging

Monitors multiple serial devices simultaneously and displays output with
timestamps and device identification. Perfect for debugging multi-node
self-organizing systems.

Usage:
    ./serial_monitor.py /dev/ttyAMA2,/dev/ttyAMA3
    ./serial_monitor.py /dev/ttyUSB0 /dev/ttyUSB1 --baud 38400
    ./serial_monitor.py --help
"""

import argparse
import asyncio
import sys
import time
from datetime import datetime
from pathlib import Path
import aioserial
import colorama
from colorama import Fore, Back, Style

# Initialize colorama for cross-platform colored output
colorama.init(autoreset=True)

# Color scheme for different devices
DEVICE_COLORS = [
    Fore.CYAN,
    Fore.GREEN, 
    Fore.YELLOW,
    Fore.MAGENTA,
    Fore.BLUE,
    Fore.RED,
]

class SerialMonitor:
    def __init__(self, devices, baud_rate=38400):
        self.devices = devices
        self.baud_rate = baud_rate
        self.device_colors = {}
        self.running = True
        
        # Assign colors to devices
        for i, device in enumerate(devices):
            self.device_colors[device] = DEVICE_COLORS[i % len(DEVICE_COLORS)]
    
    def get_device_name(self, device_path):
        """Get the device path as the name"""
        return device_path
    
    def format_timestamp(self):
        """Format current timestamp"""
        return datetime.now().strftime("%H:%M:%S.%f")[:-3]  # Include milliseconds
    
    def colorize_message(self, message):
        """Add color highlighting to protocol keywords and DEBUG messages"""
        # Protocol keywords to highlight
        keywords = {
            'HELLO': Fore.GREEN + Style.BRIGHT,
            'CLAIM': Fore.YELLOW + Style.BRIGHT, 
            'JOIN': Fore.BLUE + Style.BRIGHT,
            'ASSIGN': Fore.MAGENTA + Style.BRIGHT,
            'COORDINATOR': Fore.RED + Style.BRIGHT,
            'MEMBER': Fore.CYAN + Style.BRIGHT,
        }
        
        # Check if this is a DEBUG message
        if message.startswith('DEBUG:'):
            # Make DEBUG messages dimmer
            message = f"{Style.DIM}{message}{Style.RESET_ALL}"
        else:
            # Highlight protocol keywords in non-DEBUG messages
            for keyword, color_code in keywords.items():
                if keyword in message:
                    message = message.replace(keyword, f"{color_code}{keyword}{Style.RESET_ALL}")
        
        return message
    
    def print_message(self, device, message):
        """Print a formatted message with timestamp and device info"""
        timestamp = self.format_timestamp()
        device_name = self.get_device_name(device)
        color = self.device_colors[device]
        
        # Clean up the message (strip whitespace, handle empty lines)
        message = message.strip()
        if not message:
            return
        
        # Apply message colorization
        colored_message = self.colorize_message(message)
            
        # Format: [HH:MM:SS.mmm] DEVICE-NAME: message
        print(f"{Style.DIM}[{timestamp}]{Style.RESET_ALL} "
              f"{color}{Style.BRIGHT}{device_name:>15}{Style.RESET_ALL}: "
              f"{colored_message}")
    
    async def monitor_device(self, device_path):
        """Monitor a single serial device"""
        try:
            # Open serial connection
            serial_conn = aioserial.AioSerial(
                port=device_path,
                baudrate=self.baud_rate,
                timeout=1
            )
            
            print(f"{Fore.GREEN}✓ Connected to {device_path} at {self.baud_rate} baud{Style.RESET_ALL}")
            
            # Read lines continuously
            while self.running:
                try:
                    line = await serial_conn.readline_async()
                    if line:
                        message = line.decode('utf-8', errors='replace')
                        self.print_message(device_path, message)
                except asyncio.TimeoutError:
                    continue
                except Exception as e:
                    print(f"{Fore.RED}Error reading from {device_path}: {e}{Style.RESET_ALL}")
                    break
                    
        except Exception as e:
            print(f"{Fore.RED}✗ Failed to connect to {device_path}: {e}{Style.RESET_ALL}")
            print(f"{Fore.YELLOW}  Make sure the device exists and you have permissions{Style.RESET_ALL}")
    
    async def run(self):
        """Run the monitor for all devices"""
        print(f"{Style.BRIGHT}Multi-Serial Monitor Starting...{Style.RESET_ALL}")
        print(f"Monitoring {len(self.devices)} device(s) at {self.baud_rate} baud")
        print(f"Press Ctrl+C to stop\n")
        
        # Start monitoring tasks for all devices
        tasks = []
        for device in self.devices:
            task = asyncio.create_task(self.monitor_device(device))
            tasks.append(task)
        
        try:
            # Wait for all tasks (they run indefinitely)
            await asyncio.gather(*tasks)
        except KeyboardInterrupt:
            print(f"\n{Fore.YELLOW}Stopping monitor...{Style.RESET_ALL}")
            self.running = False
            
            # Cancel all tasks
            for task in tasks:
                task.cancel()
            
            # Wait for tasks to cleanup
            await asyncio.gather(*tasks, return_exceptions=True)

def parse_device_list(device_arg):
    """Parse device list from command line argument"""
    if ',' in device_arg:
        return [d.strip() for d in device_arg.split(',')]
    else:
        return [device_arg]

def main():
    parser = argparse.ArgumentParser(
        description="Monitor multiple serial devices with timestamps",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s /dev/ttyAMA2,/dev/ttyAMA3
  %(prog)s /dev/ttyUSB0 /dev/ttyUSB1 --baud 115200
  %(prog)s /dev/ttyAMA2,/dev/ttyAMA3,/dev/ttyAMA4 --baud 38400
        """
    )
    
    parser.add_argument(
        'devices',
        nargs='+',
        help='Serial devices to monitor (comma-separated or space-separated)'
    )
    
    parser.add_argument(
        '--baud', '-b',
        type=int,
        default=38400,
        help='Baud rate for all devices (default: 38400)'
    )
    
    args = parser.parse_args()
    
    # Parse device list
    all_devices = []
    for device_arg in args.devices:
        all_devices.extend(parse_device_list(device_arg))
    
    # Remove duplicates while preserving order
    devices = list(dict.fromkeys(all_devices))
    
    if not devices:
        print(f"{Fore.RED}Error: No devices specified{Style.RESET_ALL}")
        sys.exit(1)
    
    # Validate devices exist
    for device in devices:
        if not Path(device).exists():
            print(f"{Fore.RED}Error: Device {device} does not exist{Style.RESET_ALL}")
            sys.exit(1)
    
    # Create and run monitor
    monitor = SerialMonitor(devices, args.baud)
    
    try:
        asyncio.run(monitor.run())
    except KeyboardInterrupt:
        print(f"\n{Fore.GREEN}Monitor stopped.{Style.RESET_ALL}")

if __name__ == '__main__':
    main()
