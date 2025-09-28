# RC Plane Controller Demo

## App Interface Description

Since we cannot run the Android app in this environment, here's a detailed description of what the user interface looks like when running:

### Main Screen Layout (Landscape Mode)

```
┌─────────────────────────────────────────────────────────────────────┐
│                          [Connection Status]                        │
│                            Connected/Disconnected                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│                             [Throttle]                              │
│                         ═══════════════════                         │
│                              50%                                    │
│                                                                     │
│                     ┌─────────┐           ┌─────────┐               │
│                     │ [ARM]   │           │[DISARM] │               │
│                     │ Button  │           │ Button  │               │
│                     └─────────┘           └─────────┘               │
│                                                                     │
│   Roll: 0.52 | Pitch: -0.31 | Yaw: 0.15 | Throttle: 50 | Armed: YES│
│                                                                     │
│  ┌─────────┐                                         ┌─────────┐    │
│  │    ●    │                                         │         │    │
│  │ ╱     ╲ │  Pitch/Roll                      Yaw    │    ●    │    │
│  │╱   ●   ╲│  (Left Stick)                (Right)    │         │    │
│  │╲       ╱│                                         │         │    │
│  │ ╲     ╱ │                                         │         │    │
│  └─────────┘                                         └─────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

### Interface Elements:

1. **Status Bar** (Top)
   - Shows "Connected" in green when linked to Pico W
   - Shows "Disconnected" in red when no connection
   - Updates in real-time

2. **Throttle Control** (Upper Center)
   - Horizontal slider with 0-100% range
   - Live percentage display below slider
   - Orange/red color scheme for visibility

3. **ARM/DISARM Buttons** (Center)
   - ARM button: Green when disarmed, Red when armed
   - DISARM button: Red when disarmed, Green when armed
   - Large, easy-to-tap buttons for safety

4. **Data Display** (Lower Center)
   - Real-time flight data in monospace font
   - Shows: Roll, Pitch, Yaw (-1.0 to 1.0), Throttle (0-100%), Armed status
   - Updates 20 times per second

5. **Left Joystick** (Bottom Left)
   - Controls Roll (X-axis) and Pitch (Y-axis)
   - Circular boundary with crosshairs
   - Green knob that follows finger movement
   - Returns to center when released
   - Label: "Pitch/Roll"

6. **Right Joystick** (Bottom Right)
   - Controls Yaw (X-axis only)
   - Same visual design as left joystick  
   - Y-axis movement ignored for yaw control
   - Label: "Yaw"

### Color Scheme:
- **Background**: Dark gray (#263238)
- **Text**: White
- **Joystick Background**: Semi-transparent black
- **Joystick Knob**: Green (#4CAF50)
- **Armed State**: Red/Orange (#FF5722)
- **Disarmed State**: Green (#4CAF50)
- **Throttle**: Orange accent color

### Behavior:
- App automatically tries to connect to 192.168.4.1:4444 on startup
- Joysticks provide smooth, analog control with visual feedback
- All controls are disabled until ARM button is pressed
- Throttle resets to 0% when DISARM is pressed
- App automatically disarms when going to background for safety
- UDP packets sent at 20Hz (50ms intervals) for responsive control

### Safety Features:
- Visual ARM/DISARM state indication
- Automatic disarm on app backgrounding
- Connection status monitoring
- Throttle auto-reset on disarm
- Large, clearly labeled safety buttons

This interface provides an intuitive, game-controller-like experience for piloting an RC plane while maintaining important safety features for responsible operation.