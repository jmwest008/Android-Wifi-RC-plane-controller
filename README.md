# Android RC Plane Controller

A Kotlin Android application for controlling a Raspberry Pi Pico W-based RC plane via Wi-Fi. The app provides intuitive dual-joystick controls and sends UDP packets containing flight control data at 20Hz for responsive control.

## Features

### 🎮 Intuitive Controls
- **Left Joystick**: Roll and Pitch control (-1.0 to 1.0 range)
- **Right Joystick**: Yaw control (-1.0 to 1.0 range)
- **Throttle Slider**: Vertical slider for throttle control (0-100%)
- **ARM/DISARM Buttons**: Safety controls with visual feedback

### 📡 Network Communication
- Connects to Pico W Access Point (default: 192.168.4.1)
- Sends UDP packets to port 4444 at 20Hz refresh rate
- Binary packet format for efficient transmission
- Automatic connection monitoring and status display

### 🛡️ Safety Features
- Auto-disarm when app goes to background
- Throttle automatically resets to 0% when disarming
- Visual connection status indicator
- Real-time flight data display

## Network Protocol

The app sends 21-byte UDP packets with the following structure:

| Field    | Type       | Size    | Range       | Description                  |
|----------|------------|---------|-------------|------------------------------|
| Roll     | float      | 4 bytes | -1.0 to 1.0 | Left joystick X-axis         |
| Pitch    | float      | 4 bytes | -1.0 to 1.0 | Left joystick Y-axis         |
| Yaw      | float      | 4 bytes | -1.0 to 1.0 | Right joystick X-axis        |
| Throttle | float      | 4 bytes | 0.0 to 1.0  | Throttle slider (normalized) |
| Armed    | int + byte | 5 bytes | 0 or 1      | Safety arm/disarm status     |

Data is sent in little-endian format for compatibility with microcontrollers.

## Setup and Installation

### Prerequisites
- Android Studio or compatible IDE
- Android device with API level 24+ (Android 7.0)
- Raspberry Pi Pico W configured as Access Point

### Building the App
1. Clone this repository
2. Open the project in Android Studio
3. Build and install on your Android device

### Connecting to Pico W
1. Configure your Pico W as a Wi-Fi Access Point (SSID: typically "PicoW-AP")
2. Connect your Android device to the Pico W's Wi-Fi network
3. Launch the RC Plane Controller app
4. The app will automatically attempt to connect to 192.168.4.1:4444

## Usage

1. **Connect**: Ensure your device is connected to the Pico W's Wi-Fi network
2. **ARM**: Tap the ARM button to enable control (button turns red)
3. **Control**: Use the joysticks and throttle slider to control the plane
4. **Monitor**: Watch the real-time data display for current control values
5. **DISARM**: Tap DISARM button to disable control (for safety)

## File Structure

```
app/src/main/java/com/rcplane/controller/
├── MainActivity.kt          # Main activity with UI handling
├── JoystickView.kt         # Custom joystick view component
└── UdpService.kt           # UDP communication service

app/src/main/res/
├── layout/
│   └── activity_main.xml   # Main UI layout (landscape)
├── values/
│   ├── colors.xml          # App color scheme
│   ├── strings.xml         # String resources
│   └── themes.xml          # App themes
└── xml/
    ├── backup_rules.xml    # Backup configuration
    └── data_extraction_rules.xml
```

## Pico W Configuration

Your Pico W should be configured to:
1. Create a Wi-Fi Access Point
2. Listen for UDP packets on port 4444
3. Parse the 21-byte packet format described above
4. Control servo motors/ESCs based on received data

Example Pico W code structure:
```python
# Pseudo-code for Pico W
while True:
    data = receive_udp_packet()
    roll, pitch, yaw, throttle, armed = parse_packet(data)
    if armed:
        control_servos(roll, pitch, yaw, throttle)
    else:
        set_safe_position()
```

## Troubleshooting

### Connection Issues
- Verify Pico W is broadcasting its Access Point
- Check that Android device is connected to Pico W's network
- Ensure Pico W is listening on the correct IP and port (192.168.4.1:4444)

### Control Issues
- Make sure to ARM the system before expecting controls to work
- Check that UDP packets are being received by monitoring Pico W logs
- Verify joystick calibration by watching the real-time data display

## License

This project is open source and available under the MIT License.

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss proposed changes.
