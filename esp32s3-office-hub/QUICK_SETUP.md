# Quick Setup Guide

## 1. Hardware Connections
```
ESP32-S3-WROOM-1-N8R2 Connections:
┌─────────────────────┐
│  GPIO 48 ──────────────→ RGB LED Data Pin (WS2812)
│  GPIO 47 ──────────────→ Large LED Anode (+)
│  GND     ──────────────→ LED Cathode (-) & Ground
│  3.3V    ──────────────→ Power (if needed)
│  USB-C   ──────────────→ Programming & Power
└─────────────────────┘
```

## 2. Software Setup
```bash
# Install PlatformIO (if not already installed)
pip install platformio

# Navigate to project directory
cd /home/sam/SPOON_GIT/esp32/esp32s3-office-hub

# Build the project
pio run

# Upload to ESP32-S3
pio run -t upload

# Monitor serial output
pio device monitor
```

## 3. First Boot
1. Connect ESP32-S3 to computer via USB-C
2. Upload firmware using commands above
3. Open serial monitor to see boot messages
4. Note the IP address displayed (if WiFi connects)
5. Open web browser to that IP address

## 4. Web Interface
- **Dashboard**: System overview and status
- **LED Controls**: RGB and large LED management
- **Settings**: WiFi configuration and system info
- **Temperature**: Real-time temperature monitoring
- **WiFi Scanner**: Network discovery and logging
- **Clock**: NTP time sync and hourly alerts
- **USB Storage**: File management (experimental)

## 5. Default Settings
- **LED Mode**: Solid red RGB LED
- **Large LED**: Off, brightness 255
- **Temperature**: 5-second reading interval
- **WiFi Scan**: Every 5 minutes
- **Time Sync**: Every 1 minute with NTP
- **Hourly Alerts**: Enabled (LED flash at hour mark)

## 6. Troubleshooting
- **No web interface**: Check serial monitor for IP address
- **LEDs not working**: Verify GPIO connections and power
- **WiFi issues**: Configure credentials in Settings tab
- **Build errors**: Run `pio lib update` and `pio run -t clean`

## Next Steps
1. Configure WiFi credentials via web interface
2. Test LED controls and modes
3. Monitor temperature readings
4. Set up hourly alerts as desired
5. Explore USB storage features (when available)
