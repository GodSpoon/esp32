# ESP32-S3 Office Control & Monitoring Hub

A comprehensive office control and monitoring system built for the ESP32-S3-WROOM-1-N8R2 development board, featuring a modern web interface, LED controls, temperature monitoring, WiFi scanning, USB storage management, and NTP-based clock functionality.

## Hardware Requirements

### Development Board
- **ESP32-S3-WROOM-1-N8R2** (8MB Flash, 2MB PSRAM)
  - Dual-core Xtensa LX7 processors @ 240MHz
  - WiFi 802.11 b/g/n and Bluetooth 5.0 LE
  - USB OTG functionality
  - Built-in temperature sensor

### Components
- **RGB LED Strip** - Connected to GPIO 48 (WS2812/Neopixel compatible)
- **Large LED** - Connected to GPIO 47 (for brightness control)
- **USB Storage Device** (optional) - For USB OTG functionality

## Features

### 🌐 Web Interface
- **Responsive Bootstrap-based UI** with sidebar navigation
- **Real-time WebSocket communication** for live updates
- **Multiple themed sections** for different functionalities
- **Mobile-friendly design** that works on phones and tablets

### 💡 LED Control System
- **RGB LED Control**
  - Color picker for custom colors
  - Multiple modes: Solid, Blink, Pulse, Rainbow
  - Real-time preview and updates
- **Large LED Control**
  - On/off toggle with brightness adjustment
  - PWM-based brightness control (0-255)
  - Persistent settings across reboots

### 🌡️ Temperature Monitoring
- **Built-in ESP32-S3 temperature sensor**
- **Real-time temperature readings** every 5 seconds
- **Temperature logging** with timestamps
- **Visual gauge display** on web interface

### 📡 WiFi Management
- **Automatic network scanning** every 5 minutes
- **Network strength indicators** with RSSI values
- **Connection logging** with channel and encryption info
- **Dynamic WiFi configuration** through web interface

### 🕒 Time & Alerts
- **NTP time synchronization** with pool.ntp.org
- **EST timezone configuration** (-5 hours from UTC)
- **Hourly LED alerts** with configurable on/off
- **Real-time clock display** on web interface

### 💾 USB Storage (Experimental)
- **USB OTG host functionality** for storage devices
- **File management interface** for upload/download
- **Directory browsing** and file operations
- **Storage space monitoring** with usage statistics

### ⚙️ System Management
- **Persistent settings storage** using ESP32 Preferences
- **System information display** (chip model, memory, etc.)
- **Real-time performance monitoring** with heap usage
- **Automatic reconnection** for dropped WiFi connections

## Getting Started

### 1. Hardware Setup
```
ESP32-S3-WROOM-1-N8R2 Pinout:
├── GPIO 48 → RGB LED Data Pin (WS2812)
├── GPIO 47 → Large LED Control (PWM)
├── GPIO 2  → Onboard LED (status indicator)
└── USB     → USB OTG for storage devices
```

### 2. Software Installation
```bash
# Clone or create the project
cd /path/to/your/workspace
mkdir esp32s3-office-hub
cd esp32s3-office-hub

# Install PlatformIO if not already installed
pip install platformio

# Build and upload the project
pio run -t upload

# Monitor serial output
pio device monitor
```

### 3. Web Interface Access
1. **Connect to WiFi** - Device will attempt to connect to configured networks
2. **Find IP Address** - Check serial monitor for assigned IP
3. **Open Web Browser** - Navigate to `http://[ESP32_IP_ADDRESS]`
4. **Configure Settings** - Use the Settings tab to configure WiFi and preferences

### 4. Initial Configuration
```cpp
// Default WiFi credentials (change in code or via web interface)
SSID: [Configure via web interface]
Password: [Configure via web interface]

// LED Default Settings
RGB LED: Red color, solid mode
Large LED: Off, brightness 255

// Temperature Monitoring: Enabled, 5-second intervals
// Hourly Alerts: Enabled by default
```

## Web Interface Sections

### 📊 Dashboard
- **System uptime** with days/hours/minutes display
- **WiFi connection status** and signal strength
- **Current temperature** reading from internal sensor
- **USB storage status** and connection info
- **Quick refresh button** for manual updates

### 💡 LED Controls
- **RGB LED Panel**
  - Color picker with hex color selection
  - Mode selection (Solid/Blink/Pulse/Rainbow)
  - Real-time color preview
- **Large LED Panel**
  - Power toggle switch
  - Brightness slider (0-255)
  - Live brightness value display

### 💾 USB Storage
- **File browser interface** (when USB device connected)
- **Upload/download functionality** for file management
- **Storage space indicators** with free/total space
- **Device information** display (vendor, capacity, filesystem)

### 📶 WiFi Scanner
- **Real-time network discovery** with auto-refresh
- **Signal strength visualization** with bar indicators
- **Network details** (SSID, channel, encryption, RSSI)
- **Connection history** with timestamps

### 🕒 Clock & Alerts
- **Live digital clock** with seconds precision
- **Current date display** in YYYY/MM/DD format
- **Hourly alert toggle** for LED notifications
- **Timezone configuration** (currently EST -5)

### 🌡️ Temperature Monitoring
- **Current temperature gauge** with degree Celsius
- **Historical temperature log** with scrollable view
- **Temperature trending** with timestamp data
- **Sensor calibration info** and accuracy notes

### ⚙️ Settings
- **WiFi Configuration**
  - SSID and password input fields
  - Save and apply new settings
  - Connection status feedback
- **System Information**
  - Chip model and specifications
  - Memory usage (heap, flash, PSRAM)
  - Firmware version and build info

## API Endpoints

### REST API
```http
GET /api/status          # System status information
GET /api/temperature     # Current temperature reading
GET /api/wifi/scan       # WiFi network scan results
GET /api/usb/status      # USB storage device status
GET /api/time            # Current time and date
```

### WebSocket Commands
```javascript
// Status updates
{type: "get_status"}

// LED control
{type: "rgb_color", color: "#ff0000"}
{type: "rgb_mode", mode: "pulse"}
{type: "large_led", state: true}
{type: "brightness", value: 128}

// Settings
{type: "wifi_config", ssid: "MyWiFi", password: "password"}
{type: "hourly_alerts", enabled: true}
```

## Technical Specifications

### Memory Configuration
- **Flash**: 8MB (optimized partitioning for web assets)
- **PSRAM**: 2MB (enabled with cache issue fix)
- **Heap**: Dynamic allocation with monitoring
- **SPIFFS**: File system for data logging and configuration

### Performance Characteristics
- **CPU**: Dual-core @ 240MHz with task distribution
- **WiFi**: 802.11 b/g/n with auto-reconnection
- **Update Rate**: 10Hz for sensors, 1Hz for web interface
- **Response Time**: <100ms for LED controls, <500ms for WiFi operations

### Power Management
- **Active Mode**: ~150-200mA @ 3.3V (typical usage)
- **WiFi Active**: Additional ~100-150mA during transmission
- **Sleep Modes**: Deep sleep available for battery operation
- **USB Power**: 500mA available via USB-C connector

## Development Notes

### Code Architecture
```
src/
├── main.cpp           # Main application logic and web server
├── usb_host.cpp       # USB OTG host implementation
include/
├── usb_host.h         # USB host manager class definition
data/
├── (future web assets for SPIFFS upload)
```

### Libraries Used
- **ESPAsyncWebServer** - High-performance web server
- **FastLED** - Advanced LED control with effects
- **ArduinoJson** - JSON parsing and generation
- **NTPClient** - Network time synchronization
- **ESP32 Preferences** - Non-volatile settings storage

### Future Enhancements
- [ ] **MQTT Integration** for IoT connectivity
- [ ] **Voice Control** via Google Assistant/Alexa
- [ ] **Mobile App** for native iOS/Android control
- [ ] **Data Logging** to cloud services (ThingSpeak, AWS IoT)
- [ ] **Multi-room Support** with device discovery
- [ ] **Security Features** with authentication and HTTPS

## Troubleshooting

### Common Issues

**WiFi Connection Problems**
```bash
# Check serial monitor for connection attempts
pio device monitor

# Verify WiFi credentials in Settings tab
# Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
```

**LED Not Working**
```bash
# Verify wiring: GPIO48 for RGB, GPIO47 for large LED
# Check power supply (WS2812 needs 5V, ESP32 outputs 3.3V)
# Test with different LED modes in web interface
```

**USB Storage Not Detected**
```bash
# USB OTG functionality is experimental
# Ensure USB device is FAT32 formatted
# Check serial monitor for USB host messages
```

**Web Interface Not Loading**
```bash
# Verify ESP32 IP address in serial monitor
# Check if device is on same network as computer
# Try different web browser or clear cache
```

### Build Issues
```bash
# Clean build if libraries cause conflicts
pio run -t clean

# Update PlatformIO core
pio upgrade

# Check library compatibility
pio lib list
```

## License

This project is open source and available under the MIT License. Feel free to modify, distribute, and use in your own projects.

## Contributing

Contributions are welcome! Please feel free to submit pull requests, report bugs, or suggest new features through the project repository.

---

**Hardware Platform**: ESP32-S3-WROOM-1-N8R2  
**Development Environment**: PlatformIO with Arduino Framework  
**Target Audience**: IoT developers, makers, office automation enthusiasts  
**Skill Level**: Intermediate (basic electronics and programming knowledge required)
