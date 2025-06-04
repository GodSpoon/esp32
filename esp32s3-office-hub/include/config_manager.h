#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>

class ConfigManager {
private:
    Preferences preferences;
    DynamicJsonDocument config;
    
public:
    ConfigManager();
    bool begin();
    bool loadConfig();
    bool saveConfig();
    
    // System settings
    String getHostname();
    void setHostname(const String& hostname);
    String getTimezone();
    void setTimezone(const String& timezone);
    String getNTPServer();
    void setNTPServer(const String& server);
    
    // Network settings
    bool getWiFiAutoConnect();
    void setWiFiAutoConnect(bool enabled);
    int getWiFiTimeout();
    void setWiFiTimeout(int timeout);
    
    // LED settings
    uint8_t getRGBBrightness();
    void setRGBBrightness(uint8_t brightness);
    uint8_t getLargeLEDBrightness();
    void setLargeLEDBrightness(uint8_t brightness);
    String getDefaultLEDMode();
    void setDefaultLEDMode(const String& mode);
    String getDefaultColor();
    void setDefaultColor(const String& color);
    
    // Alert settings
    bool getHourlyAlertEnabled();
    void setHourlyAlertEnabled(bool enabled);
    float getTemperatureThreshold();
    void setTemperatureThreshold(float threshold);
    
    // Logging settings
    unsigned long getTempLogInterval();
    void setTempLogInterval(unsigned long interval);
    unsigned long getWiFiScanInterval();
    void setWiFiScanInterval(unsigned long interval);
};

#endif // CONFIG_MANAGER_H
