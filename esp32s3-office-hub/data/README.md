# SPIFFS Data Directory

This directory contains files that will be uploaded to the ESP32-S3's SPIFFS filesystem.

## Files in this directory:
- **Configuration files** - JSON configs for various system settings
- **Web assets** - CSS, JS, and image files for offline web interface
- **Log files** - Temperature logs, WiFi scan logs, system logs
- **User data** - Custom settings and user preferences

## SPIFFS Upload Command:
```bash
pio run -t uploadfs
```

## File Structure:
```
data/
├── config/
│   ├── settings.json      # System configuration
│   └── wifi_networks.json # Saved WiFi networks
├── logs/
│   ├── temperature.log    # Temperature readings
│   ├── wifi_scan.log      # WiFi scan results
│   └── system.log         # General system logs
└── web/
    ├── style.css          # Additional CSS styles
    ├── script.js          # Additional JavaScript
    └── favicon.ico        # Website favicon
```

Note: The main web interface is embedded in the firmware for faster loading,
but additional assets can be stored here for future enhancements.
