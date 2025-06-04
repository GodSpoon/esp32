#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    isConnected = false;
    lastScanTime = 0;
    lastConnectionAttempt = 0;
}

bool WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    
    // Load saved networks from SPIFFS
    loadSavedNetworks();
    
    Serial.println("WiFi Manager initialized");
    return true;
}

bool WiFiManager::connectToNetwork(const String& ssid, const String& password) {
    Serial.printf("Attempting to connect to: %s\n", ssid.c_str());
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        currentSSID = ssid;
        currentPassword = password;
        Serial.printf("\nConnected to %s\n", ssid.c_str());
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        
        // Update saved network last connected time
        for (auto& network : savedNetworks) {
            if (network.ssid == ssid) {
                network.lastConnected = millis();
                break;
            }
        }
        saveSavedNetworks();
        
        onWiFiConnected();
        return true;
    } else {
        Serial.println("\nConnection failed");
        isConnected = false;
        return false;
    }
}

bool WiFiManager::connectToBestNetwork() {
    if (savedNetworks.empty()) {
        Serial.println("No saved networks available");
        return false;
    }
    
    // Perform scan to see available networks
    performScan();
    
    // Sort saved networks by priority
    std::sort(savedNetworks.begin(), savedNetworks.end(), 
              [](const SavedNetwork& a, const SavedNetwork& b) {
                  return a.priority < b.priority;
              });
    
    // Try to connect to the best available network
    for (const auto& saved : savedNetworks) {
        if (!saved.autoConnect) continue;
        
        // Check if this network is available
        for (const auto& scanned : scanResults) {
            if (scanned.ssid == saved.ssid) {
                if (connectToNetwork(saved.ssid, saved.password)) {
                    return true;
                }
                delay(1000);
                break;
            }
        }
    }
    
    return false;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    isConnected = false;
    currentSSID = "";
    currentPassword = "";
    onWiFiDisconnected();
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::performScan() {
    Serial.println("Scanning for WiFi networks...");
    
    int networkCount = WiFi.scanNetworks();
    scanResults.clear();
    
    if (networkCount == 0) {
        Serial.println("No networks found");
        return false;
    }
    
    Serial.printf("Found %d networks:\n", networkCount);
    
    for (int i = 0; i < networkCount; i++) {
        WiFiNetwork network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.channel = WiFi.channel(i);
        network.encryptionType = WiFi.encryptionType(i);
        network.lastSeen = millis();
        
        scanResults.push_back(network);
        
        Serial.printf("  %d: %s (%d dBm) Ch:%d %s\n", 
                     i, network.ssid.c_str(), network.rssi, network.channel,
                     (network.encryptionType == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
    }
    
    lastScanTime = millis();
    onScanComplete();
    return true;
}

String WiFiManager::getScanResultsJSON() {
    DynamicJsonDocument doc(4096);
    JsonArray networks = doc.createNestedArray("networks");
    
    for (const auto& network : scanResults) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = network.ssid;
        net["rssi"] = network.rssi;
        net["channel"] = network.channel;
        net["encryption"] = (network.encryptionType == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
        net["last_seen"] = network.lastSeen;
        
        // Check if this network is saved
        bool isSaved = false;
        for (const auto& saved : savedNetworks) {
            if (saved.ssid == network.ssid) {
                isSaved = true;
                break;
            }
        }
        net["saved"] = isSaved;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

int WiFiManager::getNetworkCount() {
    return scanResults.size();
}

WiFiNetwork WiFiManager::getNetwork(int index) {
    if (index >= 0 && index < scanResults.size()) {
        return scanResults[index];
    }
    return WiFiNetwork();
}

bool WiFiManager::addSavedNetwork(const String& ssid, const String& password, int priority) {
    // Check if network already exists
    for (auto& network : savedNetworks) {
        if (network.ssid == ssid) {
            network.password = password;
            network.priority = priority;
            saveSavedNetworks();
            return true;
        }
    }
    
    // Add new network
    SavedNetwork network;
    network.ssid = ssid;
    network.password = password;
    network.priority = priority;
    network.autoConnect = true;
    network.lastConnected = 0;
    
    savedNetworks.push_back(network);
    saveSavedNetworks();
    
    Serial.printf("Added saved network: %s\n", ssid.c_str());
    return true;
}

bool WiFiManager::removeSavedNetwork(const String& ssid) {
    auto it = std::remove_if(savedNetworks.begin(), savedNetworks.end(),
                           [&ssid](const SavedNetwork& net) {
                               return net.ssid == ssid;
                           });
    
    if (it != savedNetworks.end()) {
        savedNetworks.erase(it, savedNetworks.end());
        saveSavedNetworks();
        Serial.printf("Removed saved network: %s\n", ssid.c_str());
        return true;
    }
    
    return false;
}

String WiFiManager::getSavedNetworksJSON() {
    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("saved_networks");
    
    for (const auto& network : savedNetworks) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = network.ssid;
        net["priority"] = network.priority;
        net["auto_connect"] = network.autoConnect;
        net["last_connected"] = network.lastConnected;
        // Don't include password in JSON for security
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool WiFiManager::loadSavedNetworks() {
    if (!SPIFFS.exists("/config/wifi_networks.json")) {
        Serial.println("No saved WiFi networks file found");
        return true;
    }
    
    File file = SPIFFS.open("/config/wifi_networks.json", "r");
    if (!file) {
        Serial.println("Failed to open WiFi networks file");
        return false;
    }
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse WiFi networks: %s\n", error.c_str());
        return false;
    }
    
    savedNetworks.clear();
    JsonArray networks = doc["saved_networks"];
    
    for (JsonObject net : networks) {
        SavedNetwork network;
        network.ssid = net["ssid"].as<String>();
        network.password = net["password"].as<String>();
        network.priority = net["priority"] | 1;
        network.autoConnect = net["auto_connect"] | true;
        network.lastConnected = net["last_connected"] | 0;
        
        savedNetworks.push_back(network);
    }
    
    Serial.printf("Loaded %d saved networks\n", savedNetworks.size());
    return true;
}

bool WiFiManager::saveSavedNetworks() {
    DynamicJsonDocument doc(4096);
    JsonArray networks = doc.createNestedArray("saved_networks");
    
    for (const auto& network : savedNetworks) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = network.ssid;
        net["password"] = network.password;
        net["priority"] = network.priority;
        net["auto_connect"] = network.autoConnect;
        net["last_connected"] = network.lastConnected;
    }
    
    File file = SPIFFS.open("/config/wifi_networks.json", "w");
    if (!file) {
        Serial.println("Failed to open WiFi networks file for writing");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    Serial.println("Saved WiFi networks to file");
    return true;
}

String WiFiManager::getCurrentSSID() {
    return WiFi.SSID();
}

int32_t WiFiManager::getCurrentRSSI() {
    return WiFi.RSSI();
}

uint8_t WiFiManager::getCurrentChannel() {
    return WiFi.channel();
}

String WiFiManager::getCurrentIP() {
    return WiFi.localIP().toString();
}

String WiFiManager::getMACAddress() {
    return WiFi.macAddress();
}

bool WiFiManager::startHotspot(const String& ssid, const String& password) {
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(ssid.c_str(), password.c_str());
    
    if (result) {
        Serial.printf("Hotspot started: %s\n", ssid.c_str());
        Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("Failed to start hotspot");
    }
    
    return result;
}

void WiFiManager::stopHotspot() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

bool WiFiManager::isHotspotActive() {
    return WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
}

void WiFiManager::handleAutoConnect() {
    if (isWiFiConnected()) {
        return;
    }
    
    if (millis() - lastConnectionAttempt < 30000) { // Don't retry too often
        return;
    }
    
    lastConnectionAttempt = millis();
    
    if (!connectToBestNetwork()) {
        Serial.println("Auto-connect failed, starting hotspot");
        startHotspot("ESP32-Office-Hub", "office123");
    }
}

void WiFiManager::setAutoConnectEnabled(bool enabled) {
    WiFi.setAutoConnect(enabled);
    WiFi.setAutoReconnect(enabled);
}

bool WiFiManager::getAutoConnectEnabled() {
    return WiFi.getAutoConnect();
}

void WiFiManager::onWiFiConnected() {
    Serial.println("WiFi connected event");
}

void WiFiManager::onWiFiDisconnected() {
    Serial.println("WiFi disconnected event");
}

void WiFiManager::onScanComplete() {
    Serial.printf("WiFi scan complete, found %d networks\n", scanResults.size());
}
