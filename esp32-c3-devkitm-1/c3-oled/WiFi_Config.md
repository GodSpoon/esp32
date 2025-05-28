# WiFi Configuration for ESP32-C3 Meme Machine

## WiFi Networks (in order of preference)
1. **Primary Network**: "Hail Satan" 
2. **Backup Network**: "ToTo" 

## Fallback Access Point Mode
If both networks fail, the device will create its own WiFi access point:
- **SSID**: ESP32-MEME-MACHINE
- **Password**: memes420
- **Default IP**: 192.168.4.1

## Features Added
- ✅ Automatic connection attempt to primary network
- ✅ Fallback to backup network if primary fails
- ✅ Access Point mode if all networks fail
- ✅ WiFi status display on OLED during startup
- ✅ Periodic WiFi status checking (every 30 seconds)
- ✅ Automatic reconnection attempts if connection drops
- ✅ Serial monitor logging of WiFi events
- ✅ Connection strength monitoring
- ✅ WiFi information panel in meme rotation with signal bars
- ✅ Beautiful WiFi status display with decorative border
- ✅ Real-time signal quality indicators (Excellent/Good/Fair/Weak)
- ✅ Animated connecting dots when disconnected
- ✅ **Random visual transition effects between panels**
- ✅ **5 different transition types**: Pixelated, Slide, Fade, Wipe, Spiral
- ✅ **Dynamic panel cycling** with consistent "panel" terminology

## Serial Monitor Output
The device will log WiFi connection attempts, successes, failures, and periodic status updates to the serial monitor at 115200 baud.

## How It Works
1. On startup, device attempts to connect to "Hail Satan"
2. If that fails after 20 seconds, tries "ToTo"
3. If that also fails, starts AP mode "ESP32-MEME-MACHINE"
4. During operation, checks WiFi status every 30 seconds
5. If disconnected, attempts to reconnect to preferred networks
6. Falls back to AP mode if reconnection fails

The meme display continues normally regardless of WiFi status!

## Meme Rotation with WiFi Info
The device cycles through 6 meme image panels plus a WiFi information panel:
1. Angy meme panel
2. Aw meme panel  
3. Concorned meme panel
4. Korby meme panel
5. Smudge meme panel
6. Wat meme panel
7. **WiFi Status Panel** - Shows current network info, signal strength bars, and connection quality

Each panel displays for 5 seconds before transitioning to the next with a **random visual effect**:

### Panel Transition Effects (Randomly Selected)
- **Pixelated Transition**: Reveals the new panel in random pixelated blocks
- **Slide Transition**: Slides the panel in from left or right
- **Fade Transition**: Gradually fades in the new panel with a checkerboard pattern  
- **Wipe Transition**: Reveals the panel with a moving line (horizontal or vertical)
- **Spiral Transition**: Reveals the panel expanding from center in a circular pattern

The ESP32 randomly selects one of these 5 transition effects for each panel change, creating a dynamic and engaging visual experience!
