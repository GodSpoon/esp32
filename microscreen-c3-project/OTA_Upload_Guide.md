# ESP32 OTA Upload Guide

Your ESP32 at IP address **10.0.0.157** now supports Over-The-Air (OTA) firmware updates! Here's how to use it.

## Method 1: OTA Upload (Recommended) 🚀

### Prerequisites
- ESP32 is connected to WiFi and reachable at 10.0.0.157
- OTA password is set to "admin" (configured in the code)

### Upload Commands

**Upload firmware using OTA:**
```bash
pio run -e esp32c3 -t upload
```

**OR explicitly specify the IP:**
```bash
pio run -e esp32c3 -t upload --upload-port 10.0.0.157
```

### What happens during OTA upload:
1. PlatformIO connects to your ESP32 at 10.0.0.157:3232
2. Authenticates with password "admin"
3. ESP32 shows "OTA Update Starting..." on the OLED
4. Progress bar appears on the display
5. When complete, device reboots with new firmware

---

## Method 2: Serial Upload (Backup) 🔌

If OTA fails, use serial upload via USB:

```bash
# Connect ESP32 via USB cable first
pio run -e esp32c3_serial -t upload
```

---

## Troubleshooting 🔧

### OTA Upload Issues:

**"Connection refused" or timeout:**
```bash
# Check if ESP32 is reachable
ping 10.0.0.157

# Check if OTA port is open
nc -zv 10.0.0.157 3232
```

**Wrong IP address:**
```bash
# Find ESP32 on network
nmap -sn 10.0.0.0/24 | grep -B2 "ESP32"
```

**Authentication failed:**
- OTA password is hardcoded as "admin" in the firmware
- Make sure platformio.ini has `--auth=admin` flag

### Serial Upload Issues:

**Device not found:**
```bash
# List available ports
pio device list

# Use specific port
pio run -e esp32c3_serial -t upload --upload-port /dev/ttyUSB0
```

---

## Current Configuration 📋

### OTA Settings (in platformio.ini):
- **Protocol:** espota
- **IP Address:** 10.0.0.157
- **Port:** 3232
- **Password:** admin

### WiFi Networks (in order of preference):
1. "Hail Satan" 
2. "ToTo"
3. AP Mode: "ESP32-MEME-MACHINE" (fallback)

---

## Quick Commands 💨

```bash
# Build only (no upload)
pio run -e esp32c3

# Build and upload via OTA
pio run -e esp32c3 -t upload

# Build and upload via serial
pio run -e esp32c3_serial -t upload

# Monitor serial output
pio device monitor -e esp32c3

# Clean build
pio run -e esp32c3 -t clean
```

---

## Display Features During Upload 📺

Your ESP32's OLED will show:
- **"OTA Update Starting..."** when upload begins
- **Progress percentage** with visual progress bar
- **"OTA Complete! Rebooting..."** when finished
- **Error messages** if upload fails

The meme rotation will pause during updates and resume after reboot.

---

## Network Discovery 🔍

If you lose track of your ESP32's IP:

```bash
# Scan for ESP32 devices
nmap -sn 192.168.1.0/24 | grep -B2 -A2 "ESP32"

# Look for "ESP32-MEME-MACHINE" hostname
avahi-browse -t _arduino._tcp
```

The device hostname is set to "ESP32-MEME-MACHINE" so you might also be able to reach it at:
- `esp32-meme-machine.local` (if mDNS is working)
