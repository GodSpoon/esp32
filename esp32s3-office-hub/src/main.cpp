#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <time.h>
#include <Preferences.h>

// Custom headers
#include "config_manager.h"
#include "led_controller.h"
#include "usb_host.h"
#include "temperature_sensor.h"
#include "wifi_manager.h"

// Global objects
ConfigManager configManager;
LEDController ledController;
USBHostManager usbManager;
TemperatureSensor tempSensor;
WiFiManager wifiMgr;

// Web Server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -18000, 60000); // EST timezone

// System variables
unsigned long lastStatusUpdate = 0;
unsigned long lastPeriodicCheck = 0;
bool systemInitialized = false;
String firmwareVersion = "1.0.0";
String buildDate = __DATE__ " " __TIME__;

// Function declarations
void initializeSystem();
void handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(AsyncWebSocketClient *client, const String& message);
void sendStatusUpdate(AsyncWebSocketClient *client = nullptr);
void sendTemperatureData(AsyncWebSocketClient *client = nullptr);
void sendWiFiScanData(AsyncWebSocketClient *client = nullptr);
void sendUSBStatusData(AsyncWebSocketClient *client = nullptr);
void performPeriodicTasks();
String formatUptime(unsigned long ms);
void handleSystemCommand(const String& command, AsyncWebSocketClient *client);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP32-S3 Office Control & Monitoring Hub ===");
    Serial.printf("Firmware Version: %s\n", firmwareVersion.c_str());
    Serial.printf("Build Date: %s\n", buildDate.c_str());
    
    initializeSystem();
}

void initializeSystem() {
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS Mount Failed");
        return;
    }
    Serial.println("✓ SPIFFS initialized");
    
    // Initialize configuration manager
    if (!configManager.begin()) {
        Serial.println("ERROR: Failed to initialize configuration");
        return;
    }
    Serial.println("✓ Configuration loaded");
    
    // Initialize LED controller
    if (!ledController.begin()) {
        Serial.println("ERROR: Failed to initialize LED controller");
        return;
    }
    Serial.println("✓ LED controller initialized");
    
    // Initialize temperature sensor
    if (!tempSensor.begin()) {
        Serial.println("WARNING: Failed to initialize temperature sensor");
    } else {
        Serial.println("✓ Temperature sensor initialized");
    }
    
    // Initialize USB host manager
    if (!usbManager.initialize()) {
        Serial.println("WARNING: USB host initialization failed");
    } else {
        Serial.println("✓ USB host initialized");
    }
    
    // Initialize WiFi manager
    if (!wifiMgr.begin()) {
        Serial.println("ERROR: Failed to initialize WiFi manager");
        return;
    }
    Serial.println("✓ WiFi manager initialized");
    
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    wifiMgr.handleAutoConnect();
    
    // Wait for WiFi connection
    int attempts = 0;
    while (!wifiMgr.isWiFiConnected() && attempts < 30) {
        delay(1000);
        attempts++;
        Serial.print(".");
    }
    
    if (wifiMgr.isWiFiConnected()) {
        Serial.printf("\n✓ WiFi connected to: %s\n", wifiMgr.getCurrentSSID().c_str());
        Serial.printf("✓ IP address: %s\n", wifiMgr.getCurrentIP().c_str());
        
        // Initialize NTP
        timeClient.begin();
        timeClient.update();
        Serial.println("✓ NTP client initialized");
    } else {
        Serial.println("\n⚠ WiFi connection failed, starting hotspot...");
        wifiMgr.startHotspot("ESP32-Office-Hub", "office123");
    }
    
    // Setup web server routes
    setupWebServer();
    
    // Start web server
    server.begin();
    Serial.println("✓ Web server started");
    
    // Signal successful initialization
    ledController.flashAlert(CRGB::Green, 3);
    systemInitialized = true;
    
    Serial.println("=== System initialization complete ===\n");
}

void setupWebServer() {
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
    // API endpoints
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(2048);
        
        doc["uptime"] = formatUptime(millis());
        doc["wifi"] = wifiMgr.isWiFiConnected() ? "Connected" : "Disconnected";
        doc["wifi_ssid"] = wifiMgr.getCurrentSSID();
        doc["wifi_ip"] = wifiMgr.getCurrentIP();
        doc["wifi_rssi"] = wifiMgr.getCurrentRSSI();
        doc["temperature"] = tempSensor.getCurrentTemperature();
        doc["temp_valid"] = tempSensor.isTemperatureValid();
        doc["usb"] = usbManager.isMounted() ? "Connected" : "Not Connected";
        doc["free_heap"] = ESP.getFreeHeap();
        doc["chip_model"] = ESP.getChipModel();
        doc["firmware_version"] = firmwareVersion;
        doc["build_date"] = buildDate;
        doc["mac_address"] = WiFi.macAddress();
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\":\"restarting\"}");
        delay(1000);
        ESP.restart();
    });
    
    server.on("/api/factory_reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\":\"resetting\"}");
        // Clear preferences and restart
        Preferences prefs;
        prefs.begin("office-hub", false);
        prefs.clear();
        prefs.end();
        delay(1000);
        ESP.restart();
    });
    
    // WebSocket handling
    ws.onEvent(handleWebSocketEvent);
    server.addHandler(&ws);
    
    // Error handling
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}

void handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                         AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            sendStatusUpdate(client);
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0; // Null terminate
                String message = (char*)data;
                handleWebSocketMessage(client, message);
            }
            break;
        }
        
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void handleWebSocketMessage(AsyncWebSocketClient *client, const String& message) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("Failed to parse WebSocket message: %s\n", error.c_str());
        return;
    }
    
    String type = doc["type"];
    Serial.printf("WebSocket message: %s\n", type.c_str());
    
    if (type == "get_status") {
        sendStatusUpdate(client);
    }
    else if (type == "rgb_color") {
        String color = doc["color"];
        ledController.setRGBColorHex(color);
        configManager.setDefaultColor(color);
    }
    else if (type == "rgb_mode") {
        String mode = doc["mode"];
        ledController.setMode(mode);
        configManager.setDefaultLEDMode(mode);
    }
    else if (type == "rgb_brightness") {
        uint8_t brightness = doc["value"];
        ledController.setRGBBrightness(brightness);
        configManager.setRGBBrightness(brightness);
    }
    else if (type == "large_led") {
        bool state = doc["state"];
        ledController.setLargeLedState(state);
    }
    else if (type == "brightness") {
        uint8_t brightness = doc["value"];
        ledController.setLargeLedBrightness(brightness);
        configManager.setLargeLEDBrightness(brightness);
    }
    else if (type == "wifi_scan") {
        wifiMgr.performScan();
        sendWiFiScanData(client);
    }
    else if (type == "usb_list_files") {
        sendUSBStatusData(client);
    }
    else if (type == "get_temperature") {
        sendTemperatureData(client);
    }
    else if (type == "save_config") {
        configManager.saveConfig();
        client->text("{\"type\":\"config_saved\",\"status\":\"success\"}");
    }
    else if (type == "system_command") {
        String command = doc["command"];
        handleSystemCommand(command, client);
    }
    else if (type == "update_setting") {
        String setting = doc["setting"];
        String value = doc["value"];
        
        if (setting == "hostname") {
            configManager.setHostname(value);
        } else if (setting == "timezone") {
            configManager.setTimezone(value);
        } else if (setting == "ntp_server") {
            configManager.setNTPServer(value);
        } else if (setting == "temp_threshold") {
            configManager.setTemperatureThreshold(value.toFloat());
        } else if (setting == "hourly_alert") {
            configManager.setHourlyAlertEnabled(doc["value"].as<bool>());
        }
        
        configManager.saveConfig();
    }
    else {
        Serial.printf("Unknown WebSocket message type: %s\n", type.c_str());
    }
}

void sendStatusUpdate(AsyncWebSocketClient *client) {
    DynamicJsonDocument doc(2048);
    doc["type"] = "status";
    doc["uptime"] = formatUptime(millis());
    doc["wifi"] = wifiMgr.isWiFiConnected() ? "Connected" : "Disconnected";
    doc["wifi_ssid"] = wifiMgr.getCurrentSSID();
    doc["wifi_ip"] = wifiMgr.getCurrentIP();
    doc["wifi_rssi"] = wifiMgr.getCurrentRSSI();
    doc["temperature"] = String(tempSensor.getCurrentTemperature(), 1);
    doc["temp_valid"] = tempSensor.isTemperatureValid();
    doc["usb"] = usbManager.isMounted() ? "Connected" : "Not Connected";
    doc["free_heap"] = ESP.getFreeHeap();
    doc["chip_model"] = ESP.getChipModel();
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void sendTemperatureData(AsyncWebSocketClient *client) {
    DynamicJsonDocument doc(2048);
    doc["type"] = "temperature";
    doc["current"] = tempSensor.getCurrentTemperature();
    doc["max"] = tempSensor.getMaxTemperature();
    doc["min"] = tempSensor.getMinTemperature();
    doc["valid"] = tempSensor.isTemperatureValid();
    doc["log"] = tempSensor.getLogJSON(100);
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void sendWiFiScanData(AsyncWebSocketClient *client) {
    DynamicJsonDocument doc(2048);
    doc["type"] = "wifi_scan";
    doc["networks"] = wifiMgr.getScanResultsJSON();
    doc["saved"] = wifiMgr.getSavedNetworksJSON();
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void sendUSBStatusData(AsyncWebSocketClient *client) {
    DynamicJsonDocument doc(2048);
    doc["type"] = "usb_status";
    doc["mounted"] = usbManager.isMounted();
    
    if (usbManager.isMounted()) {
        doc["device_info"] = usbManager.getDeviceInfo();
        doc["total_space"] = usbManager.getTotalSpace();
        doc["free_space"] = usbManager.getFreeSpace();
        doc["files"] = usbManager.listFiles();
    }
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void handleSystemCommand(const String& command, AsyncWebSocketClient *client) {
    DynamicJsonDocument response(512);
    response["type"] = "command_response";
    response["command"] = command;
    
    if (command == "restart") {
        response["status"] = "restarting";
        String resp;
        serializeJson(response, resp);
        client->text(resp);
        delay(1000);
        ESP.restart();
    }
    else if (command == "factory_reset") {
        response["status"] = "resetting";
        String resp;
        serializeJson(response, resp);
        client->text(resp);
        
        // Clear all preferences
        Preferences prefs;
        prefs.begin("office-hub", false);
        prefs.clear();
        prefs.end();
        
        delay(1000);
        ESP.restart();
    }
    else if (command == "led_test") {
        ledController.flashAlert(CRGB::Blue, 5);
        response["status"] = "led_test_complete";
    }
    else {
        response["status"] = "unknown_command";
    }
    
    String resp;
    serializeJson(response, resp);
    client->text(resp);
}

String formatUptime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    String uptime = "";
    if (days > 0) uptime += String(days) + "d ";
    if (hours > 0) uptime += String(hours) + "h ";
    if (minutes > 0) uptime += String(minutes) + "m ";
    uptime += String(seconds) + "s";
    
    return uptime;
}

void performPeriodicTasks() {
    static unsigned long lastTempCheck = 0;
    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastHourlyCheck = 0;
    
    unsigned long now = millis();
    
    // Update temperature sensor
    tempSensor.update();
    
    // Update LED animations
    ledController.update();
    
    // Check temperature alerts
    if (now - lastTempCheck >= 30000) { // Every 30 seconds
        float temp = tempSensor.getCurrentTemperature();
        float threshold = configManager.getTemperatureThreshold();
        
        if (temp > threshold) {
            ledController.flashAlert(CRGB::Red, 2);
            Serial.printf("High temperature alert: %.1f°C\n", temp);
        }
        
        lastTempCheck = now;
    }
    
    // WiFi connection monitoring
    if (now - lastWiFiCheck >= 60000) { // Every minute
        if (!wifiMgr.isWiFiConnected()) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            wifiMgr.handleAutoConnect();
            ledController.flashAlert(CRGB::Yellow, 1);
        }
        lastWiFiCheck = now;
    }
    
    // Hourly alerts
    if (configManager.getHourlyAlertEnabled() && now - lastHourlyCheck >= 3600000) { // Every hour
        time_t rawTime;
        struct tm timeInfo;
        time(&rawTime);
        localtime_r(&rawTime, &timeInfo);
        
        if (timeInfo.tm_min == 0) { // Top of the hour
            ledController.flashAlert(CRGB::Cyan, 3);
            Serial.println("Hourly alert triggered");
        }
        
        lastHourlyCheck = now;
    }
    
    // Update NTP time
    timeClient.update();
}

void loop() {
    if (!systemInitialized) {
        delay(1000);
        return;
    }
    
    // Handle periodic tasks
    if (millis() - lastPeriodicCheck >= 1000) { // Every second
        performPeriodicTasks();
        lastPeriodicCheck = millis();
    }
    
    // Send periodic status updates
    if (millis() - lastStatusUpdate >= 10000) { // Every 10 seconds
        sendStatusUpdate();
        lastStatusUpdate = millis();
    }
    
    // Clean up WebSocket connections
    ws.cleanupClients();
    
    // Small delay to prevent watchdog issues
    delay(10);
}
