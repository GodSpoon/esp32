#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    uint8_t encryptionType;
    unsigned long lastSeen;
};

struct SavedNetwork {
    String ssid;
    String password;
    int priority;
    bool autoConnect;
    unsigned long lastConnected;
};

class WiFiManager {
private:
    std::vector<WiFiNetwork> scanResults;
    std::vector<SavedNetwork> savedNetworks;
    String currentSSID;
    String currentPassword;
    bool isConnected;
    unsigned long lastScanTime;
    unsigned long lastConnectionAttempt;
    
public:
    WiFiManager();
    bool begin();
    
    // Connection management
    bool connectToNetwork(const String& ssid, const String& password);
    bool connectToBestNetwork();
    void disconnect();
    bool isWiFiConnected();
    
    // Network scanning
    bool performScan();
    String getScanResultsJSON();
    int getNetworkCount();
    WiFiNetwork getNetwork(int index);
    
    // Saved networks
    bool addSavedNetwork(const String& ssid, const String& password, int priority = 1);
    bool removeSavedNetwork(const String& ssid);
    String getSavedNetworksJSON();
    bool loadSavedNetworks();
    bool saveSavedNetworks();
    
    // Status and info
    String getCurrentSSID();
    int32_t getCurrentRSSI();
    uint8_t getCurrentChannel();
    String getCurrentIP();
    String getMACAddress();
    
    // Hotspot mode
    bool startHotspot(const String& ssid, const String& password);
    void stopHotspot();
    bool isHotspotActive();
    
    // Auto-connection
    void handleAutoConnect();
    void setAutoConnectEnabled(bool enabled);
    bool getAutoConnectEnabled();
    
    // Event callbacks
    void onWiFiConnected();
    void onWiFiDisconnected();
    void onScanComplete();
};

#endif // WIFI_MANAGER_H
