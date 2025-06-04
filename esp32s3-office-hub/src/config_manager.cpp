#include "config_manager.h"

ConfigManager::ConfigManager() : config(4096) {
}

bool ConfigManager::begin() {
    if (!preferences.begin("office-hub", false)) {
        Serial.println("Failed to initialize preferences");
        return false;
    }
    return loadConfig();
}

bool ConfigManager::loadConfig() {
    if (!SPIFFS.exists("/config/settings.json")) {
        Serial.println("Config file not found, using defaults");
        return true;
    }
    
    File file = SPIFFS.open("/config/settings.json", "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }
    
    DeserializationError error = deserializeJson(config, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse config: %s\n", error.c_str());
        return false;
    }
    
    Serial.println("Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfig() {
    File file = SPIFFS.open("/config/settings.json", "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    serializeJsonPretty(config, file);
    file.close();
    
    Serial.println("Configuration saved successfully");
    return true;
}

String ConfigManager::getHostname() {
    return config["system"]["hostname"] | "esp32-office-hub";
}

void ConfigManager::setHostname(const String& hostname) {
    config["system"]["hostname"] = hostname;
    preferences.putString("hostname", hostname);
}

String ConfigManager::getTimezone() {
    return config["system"]["timezone"] | "EST";
}

void ConfigManager::setTimezone(const String& timezone) {
    config["system"]["timezone"] = timezone;
    preferences.putString("timezone", timezone);
}

String ConfigManager::getNTPServer() {
    return config["system"]["ntp_server"] | "pool.ntp.org";
}

void ConfigManager::setNTPServer(const String& server) {
    config["system"]["ntp_server"] = server;
    preferences.putString("ntp_server", server);
}

bool ConfigManager::getWiFiAutoConnect() {
    return config["network"]["wifi_auto_connect"] | true;
}

void ConfigManager::setWiFiAutoConnect(bool enabled) {
    config["network"]["wifi_auto_connect"] = enabled;
    preferences.putBool("wifi_auto", enabled);
}

int ConfigManager::getWiFiTimeout() {
    return config["network"]["wifi_timeout"] | 30000;
}

void ConfigManager::setWiFiTimeout(int timeout) {
    config["network"]["wifi_timeout"] = timeout;
    preferences.putInt("wifi_timeout", timeout);
}

uint8_t ConfigManager::getRGBBrightness() {
    return config["leds"]["rgb_brightness"] | 128;
}

void ConfigManager::setRGBBrightness(uint8_t brightness) {
    config["leds"]["rgb_brightness"] = brightness;
    preferences.putUChar("rgb_brightness", brightness);
}

uint8_t ConfigManager::getLargeLEDBrightness() {
    return config["leds"]["large_brightness"] | 255;
}

void ConfigManager::setLargeLEDBrightness(uint8_t brightness) {
    config["leds"]["large_brightness"] = brightness;
    preferences.putUChar("large_brightness", brightness);
}

String ConfigManager::getDefaultLEDMode() {
    return config["leds"]["default_mode"] | "solid";
}

void ConfigManager::setDefaultLEDMode(const String& mode) {
    config["leds"]["default_mode"] = mode;
    preferences.putString("led_mode", mode);
}

String ConfigManager::getDefaultColor() {
    return config["leds"]["default_color"] | "#FF0000";
}

void ConfigManager::setDefaultColor(const String& color) {
    config["leds"]["default_color"] = color;
    preferences.putString("led_color", color);
}

bool ConfigManager::getHourlyAlertEnabled() {
    return config["alerts"]["hourly_enabled"] | true;
}

void ConfigManager::setHourlyAlertEnabled(bool enabled) {
    config["alerts"]["hourly_enabled"] = enabled;
    preferences.putBool("hourly_alert", enabled);
}

float ConfigManager::getTemperatureThreshold() {
    return config["alerts"]["temperature_threshold"] | 35.0;
}

void ConfigManager::setTemperatureThreshold(float threshold) {
    config["alerts"]["temperature_threshold"] = threshold;
    preferences.putFloat("temp_threshold", threshold);
}

unsigned long ConfigManager::getTempLogInterval() {
    return config["logging"]["temp_log_interval"] | 300000;
}

void ConfigManager::setTempLogInterval(unsigned long interval) {
    config["logging"]["temp_log_interval"] = interval;
    preferences.putULong("temp_interval", interval);
}

unsigned long ConfigManager::getWiFiScanInterval() {
    return config["logging"]["wifi_scan_interval"] | 300000;
}

void ConfigManager::setWiFiScanInterval(unsigned long interval) {
    config["logging"]["wifi_scan_interval"] = interval;
    preferences.putULong("wifi_interval", interval);
}
