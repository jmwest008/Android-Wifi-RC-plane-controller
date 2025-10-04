"""
Example Raspberry Pi Pico W code to receive UDP packets from the Android RC controller.
This is a basic example showing how to parse the flight control data.

Requirements:
- MicroPython firmware on Pico W
- Additional servo/PWM libraries as needed for your specific hardware
"""

import network
import socket
import struct
import machine
import time

# Wi-Fi Access Point Configuration
AP_SSID = "PicoW-RCPlane"
AP_PASSWORD = "rcplane123"

# Network settings
UDP_PORT = 4444
BUFFER_SIZE = 16

# Initialize access point
def setup_ap():
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid=AP_SSID, password=AP_PASSWORD, authmode=network.AUTH_WPA_WPA2_PSK)
    
    while not ap.active():
        pass
    
    print(f"Access Point active: {AP_SSID}")
    print(f"IP Address: {ap.ifconfig()[0]}")
    return ap

# Initialize UDP socket
def setup_udp():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', UDP_PORT))
    sock.settimeout(0.1)  # Non-blocking with timeout
    print(f"UDP server listening on port {UDP_PORT}")
    return sock

# Parse flight control packet (16 bytes)
def parse_flight_data(data):
    if len(data) != BUFFER_SIZE:
        return None
    
    try:
        # Unpack binary data: roll, pitch, yaw, throttle
        roll, pitch, yaw, throttle_norm = struct.unpack('<ffff', data)

        return {
            'roll': roll,           # -1.0 to 1.0
            'pitch': pitch,         # -1.0 to 1.0
            'yaw': yaw,             # -1.0 to 1.0
            'throttle': int(throttle_norm * 100)  # 0 to 100
        }
    except:
        return None

# Convert control values to servo positions
def controls_to_servo(roll, pitch, yaw, throttle):
    """
    Convert normalized control values to servo positions.
    Adjust these mappings based on your servo specifications.
    """
    # Servo positions typically range from 500 to 2500 microseconds
    # Center position is usually 1500 microseconds
    
    roll_servo = int(1500 + (roll * 500))      # 1000-2000 range
    pitch_servo = int(1500 + (pitch * 500))    # 1000-2000 range  
    yaw_servo = int(1500 + (yaw * 500))        # 1000-2000 range
    throttle_esc = int(1000 + (throttle * 10)) # 1000-2000 range (ESC)
    
    return roll_servo, pitch_servo, yaw_servo, throttle_esc

# Main control loop
def main():
    # Setup network and socket
    ap = setup_ap()
    sock = setup_udp()
    
    # Initialize servos (example with PWM)
    # You'll need to configure these based on your hardware connections
    roll_pwm = machine.PWM(machine.Pin(0))
    pitch_pwm = machine.PWM(machine.Pin(1))  
    yaw_pwm = machine.PWM(machine.Pin(2))
    throttle_pwm = machine.PWM(machine.Pin(3))
    
    # Set PWM frequency (50Hz for servos)
    for pwm in [roll_pwm, pitch_pwm, yaw_pwm, throttle_pwm]:
        pwm.freq(50)
    
    # Control state
    last_packet_time = time.ticks_ms()
    safety_timeout = 1000  # 1 second timeout
    
    print("RC Plane controller ready!")
    
    while True:
        try:
            # Receive UDP packet
            data, addr = sock.recvfrom(BUFFER_SIZE)
            current_time = time.ticks_ms()
            
            # Parse flight data
            flight_data = parse_flight_data(data)
            
            if flight_data:
                last_packet_time = current_time


                # Convert to servo positions
                roll_pos, pitch_pos, yaw_pos, throttle_pos = controls_to_servo(
                    flight_data['roll'],
                    flight_data['pitch'],
                    flight_data['yaw'],
                    flight_data['throttle']
                )

                # Apply servo positions
                roll_pwm.duty_u16(int(roll_pos * 65535 / 20000))
                pitch_pwm.duty_u16(int(pitch_pos * 65535 / 20000))
                yaw_pwm.duty_u16(int(yaw_pos * 65535 / 20000))
                throttle_pwm.duty_u16(int(throttle_pos * 65535 / 20000))

                print(f"Controls: R:{flight_data['roll']:.2f} P:{flight_data['pitch']:.2f} Y:{flight_data['yaw']:.2f} T:{flight_data['throttle']}")
                    
        except OSError:
            # Timeout or no data - check for safety timeout
            if time.ticks_diff(time.ticks_ms(), last_packet_time) > safety_timeout:
                # No packets received for too long - enter safe mode
                roll_pwm.duty_u16(int(1500 * 65535 / 20000))
                pitch_pwm.duty_u16(int(1500 * 65535 / 20000))
                yaw_pwm.duty_u16(int(1500 * 65535 / 20000))
                throttle_pwm.duty_u16(int(1000 * 65535 / 20000))
                
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    main()