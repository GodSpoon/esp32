# ESP32-C3 Microscreen Project - Wokwi Circuit Diagram

## Hardware Wiring Guide

This document provides the complete wiring diagram and component specifications for the ESP32-C3 Microscreen project.

## Components Required

1. **ESP32-C3 SuperMini** - Main microcontroller
2. **SSD1306 OLED Display** (128x64, I2C interface, 0x3C address)
3. **2x Momentary Push Buttons** (tactile switches)
4. **2x 10kΩ Resistors** (for voltage divider circuit)
5. **3.7V Li-ion Battery** (optional, for portable operation)
6. **Breadboard and jumper wires**

## Pin Connections

### OLED Display (SSD1306)
- **VCC** → ESP32-C3 **3.3V**
- **GND** → ESP32-C3 **GND**
- **SDA** → ESP32-C3 **GPIO6**
- **SCL** → ESP32-C3 **GPIO7**

### Buttons
- **Menu Button (Blue)**:
  - One terminal → ESP32-C3 **GPIO2**
  - Other terminal → ESP32-C3 **GND**
  - *Uses internal pullup resistor*

- **Reset Button (Red)**:
  - One terminal → ESP32-C3 **GPIO3**
  - Other terminal → ESP32-C3 **GND**
  - *Uses internal pullup resistor*

### Battery Monitoring Circuit
- **Battery Positive** → **10kΩ Resistor (R1)** → **GPIO0**
- **GPIO0** → **10kΩ Resistor (R2)** → **GND**
- **Battery Negative** → ESP32-C3 **GND**

This creates a 2:1 voltage divider that allows monitoring of battery voltage up to 6.6V safely.

## Circuit Specifications

### Voltage Divider Calculation
- **Voltage Divider Ratio**: 2.0 (as defined in code)
- **R1 = R2 = 10kΩ** creates exactly 2:1 ratio
- **Maximum Input Voltage**: 6.6V (3.3V × 2)
- **ADC Resolution**: 12-bit (0-4095 counts)

### Battery Monitoring
- **Maximum Battery Voltage**: 4.2V (fully charged Li-ion)
- **Minimum Battery Voltage**: 3.0V (discharge cutoff)
- **ADC Input Range**: 0-3.3V (ESP32-C3 maximum)
- **Voltage at GPIO0**: Battery voltage ÷ 2

## Button Operation
- **Active Low**: Buttons read LOW when pressed (pulled to GND)
- **Idle State**: HIGH (pulled up by internal 10kΩ pullup resistors)
- **Debounce**: 50ms software debouncing implemented
- **Menu Button**: Cycles through menu modes (Normal → Battery → System → Normal)
- **Reset Button**: Immediately changes to next panel/meme

## Display Configuration
- **Resolution**: 128×64 pixels
- **Interface**: I2C
- **Address**: 0x3C
- **Voltage**: 3.3V compatible
- **Reset Pin**: Not used (set to -1 in code)

## Wokwi Simulation

The `diagram.json` file contains the complete circuit simulation that can be opened in [Wokwi](https://wokwi.com).

### How to Use the Wokwi Diagram:
1. Open [wokwi.com](https://wokwi.com)
2. Create a new project or click "Import from file"
3. Upload the `diagram.json` file from this project
4. The complete circuit will load with all components properly connected
5. Use the `wokwi.toml` configuration to run your firmware in simulation

### Component IDs in Diagram:
- **esp32c3**: ESP32-C3 microcontroller
- **oled_display**: SSD1306 OLED display (128x64)
- **menu_button**: Blue menu button (GPIO2)
- **reset_button**: Red reset button (GPIO3)
- **voltage_divider_r1**: 10kΩ resistor (battery side)
- **voltage_divider_r2**: 10kΩ resistor (ground side)
- **lipo_battery**: 3.7V lithium battery

### Color Coding in Diagram:
- **Red**: Power (3.3V)
- **Black**: Ground
- **Green**: I2C SDA (GPIO6)
- **Blue**: I2C SCL (GPIO7)
- **Yellow**: Menu Button (GPIO2)
- **Orange**: Reset Button (GPIO3)
- **Purple**: Battery ADC (GPIO0)
- **Gray**: Voltage divider connection

## Software Features

The firmware includes:
- **Panel Cycling**: 6 meme images + 1 WiFi info panel
- **Transition Effects**: Pixelated, slide, fade, wipe, and spiral animations
- **WiFi Connectivity**: Auto-connects to preferred networks or creates AP
- **Battery Monitoring**: Real-time voltage and percentage display
- **Menu System**: Battery info and system information panels
- **GPIO Monitoring**: Web interface for real-time GPIO state viewing
- **OTA Updates**: Over-the-air firmware updates

## Power Consumption

- **Active Mode**: ~80mA (display on, WiFi active)
- **Sleep Mode**: Not implemented (continuous operation)
- **Battery Life**: Approximately 12-24 hours with 2000mAh battery

## Assembly Notes

1. **Test buttons first**: Use the built-in button test mode during startup
2. **Check I2C connections**: Ensure SDA/SCL are not swapped
3. **Verify voltage divider**: Measure voltage at GPIO0 should be half of battery voltage
4. **WiFi credentials**: Update `wifiNetworks[]` array with your networks
5. **GPIO Viewer**: Access real-time monitoring at `http://[ESP32-IP]:8080`

## Troubleshooting

- **Display not working**: Check I2C wiring and address (0x3C)
- **Buttons not responding**: Verify GPIO2/GPIO3 connections and internal pullups
- **Battery reading incorrect**: Check voltage divider resistor values and connections
- **WiFi not connecting**: Verify credentials in code and check serial output

## File Structure

```
esp32/microscreen-c3-project/
├── src/
│   └── main.cpp              # Main firmware code
├── images/
│   └── bmp/                  # Bitmap image headers
├── diagram.json              # Circuit diagram for Wokwi
├── wokwi.toml               # Wokwi configuration file
└── WIRING_GUIDE.md          # This file
```

## Next Steps

1. **Build the circuit** according to the wiring diagram
2. **Flash the firmware** using PlatformIO or Arduino IDE
3. **Test all functions** using the serial monitor
4. **Access GPIO Viewer** for real-time monitoring
5. **Customize images** by replacing bitmap files

---

*This project demonstrates ESP32-C3 capabilities including GPIO control, I2C communication, ADC reading, WiFi connectivity, and OLED display management.*
