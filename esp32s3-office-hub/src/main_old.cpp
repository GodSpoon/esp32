#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <FastLED.h>
#include <Preferences.h>
#include "esp_temperature_sensor.h"
#include "USB.h"
#include "usb_host.h"

// Pin Definitions
#define RGB_LED_PIN 48        // RGB LED strip on GPIO 48
#define LARGE_LED_PIN 47      // Large LED control on GPIO 47  
#define NUM_RGB_LEDS 1        // Number of RGB LEDs
#define ONBOARD_LED 2         // Onboard LED

// LED Control Variables
CRGB rgbLeds[NUM_RGB_LEDS];
bool largeLedState = false;
uint8_t largeLedBrightness = 255;
String ledMode = "solid";
uint8_t hue = 0;
unsigned long lastModeUpdate = 0;

// Web Server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// NTP Client  
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -18000, 60000); // EST timezone

// Preferences for persistent storage
Preferences preferences;

// Temperature sensor
temperature_sensor_handle_t temp_sensor = NULL;

// System variables
unsigned long lastTempReading = 0;
unsigned long lastWiFiScan = 0;
unsigned long lastHourlyCheck = 0;
float currentTemperature = 0.0;
bool hourlyAlertEnabled = true;
String currentSSID = "";
String currentPassword = "";

// USB Host Manager
USBHostManager usbManager;

// File system and logs
struct TempLog {
    unsigned long timestamp;
    float temperature;
};

struct WiFiScanResult {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    unsigned long timestamp;
};

// HTML/CSS/JS embedded as string constants
const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-S3 Office Hub</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <style>
        .sidebar {
            min-height: 100vh;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        .nav-link {
            color: rgba(255,255,255,0.8) !important;
            transition: all 0.3s ease;
        }
        .nav-link:hover, .nav-link.active {
            color: white !important;
            background-color: rgba(255,255,255,0.1);
            border-radius: 8px;
        }
        .content-area {
            background-color: #f8f9fa;
            min-height: 100vh;
        }
        .card {
            border: none;
            box-shadow: 0 0.125rem 0.25rem rgba(0, 0, 0, 0.075);
            transition: transform 0.2s ease-in-out;
        }
        .card:hover {
            transform: translateY(-2px);
        }
        .temp-gauge {
            width: 100px;
            height: 100px;
            margin: 0 auto;
        }
        #colorPicker {
            width: 100px;
            height: 50px;
            border: none;
            border-radius: 8px;
        }
        .wifi-signal {
            display: inline-block;
            width: 20px;
            height: 20px;
        }
        .signal-bar {
            display: inline-block;
            width: 3px;
            margin: 0 1px;
            background: #28a745;
            border-radius: 1px;
        }
        .section {
            display: none;
        }
        .section.active {
            display: block;
        }
        .log-container {
            max-height: 400px;
            overflow-y: auto;
            border: 1px solid #dee2e6;
            border-radius: 0.375rem;
            padding: 1rem;
        }
    </style>
</head>
<body>
    <div class="container-fluid">
        <div class="row">
            <!-- Sidebar -->
            <div class="col-md-3 col-lg-2 d-md-block sidebar collapse">
                <div class="position-sticky pt-3">
                    <h4 class="text-white text-center mb-4">
                        <i class="fas fa-microchip"></i> ESP32-S3 Hub
                    </h4>
                    <ul class="nav nav-pills flex-column mb-auto">
                        <li class="nav-item">
                            <a class="nav-link active" href="#" onclick="showSection('dashboard')">
                                <i class="fas fa-tachometer-alt"></i> Dashboard
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('leds')">
                                <i class="fas fa-lightbulb"></i> LED Controls
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('usb')">
                                <i class="fas fa-usb"></i> USB Storage
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('wifi')">
                                <i class="fas fa-wifi"></i> WiFi Scanner
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('clock')">
                                <i class="fas fa-clock"></i> Clock & Alerts
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('temperature')">
                                <i class="fas fa-thermometer-half"></i> Temperature
                            </a>
                        </li>
                        <li class="nav-item">
                            <a class="nav-link" href="#" onclick="showSection('settings')">
                                <i class="fas fa-cog"></i> Settings
                            </a>
                        </li>
                    </ul>
                </div>
            </div>
            
            <!-- Main content -->
            <div class="col-md-9 ms-sm-auto col-lg-10 px-md-4 content-area">
                <div class="d-flex justify-content-between flex-wrap flex-md-nowrap align-items-center pt-3 pb-2 mb-3 border-bottom">
                    <h1 class="h2" id="pageTitle">Dashboard</h1>
                    <div class="btn-toolbar mb-2 mb-md-0">
                        <button type="button" class="btn btn-sm btn-outline-secondary" onclick="refreshData()">
                            <i class="fas fa-sync-alt"></i> Refresh
                        </button>
                    </div>
                </div>

                <!-- Dashboard Section -->
                <div id="dashboard-section" class="section active">
                    <div class="row">
                        <div class="col-xl-3 col-md-6 mb-4">
                            <div class="card text-center">
                                <div class="card-body">
                                    <h5 class="card-title">System Uptime</h5>
                                    <h3 class="text-primary" id="uptime">--</h3>
                                </div>
                            </div>
                        </div>
                        <div class="col-xl-3 col-md-6 mb-4">
                            <div class="card text-center">
                                <div class="card-body">
                                    <h5 class="card-title">WiFi Status</h5>
                                    <h3 class="text-success" id="wifi-status">--</h3>
                                </div>
                            </div>
                        </div>
                        <div class="col-xl-3 col-md-6 mb-4">
                            <div class="card text-center">
                                <div class="card-body">
                                    <h5 class="card-title">Temperature</h5>
                                    <h3 class="text-warning" id="current-temp">--°C</h3>
                                </div>
                            </div>
                        </div>
                        <div class="col-xl-3 col-md-6 mb-4">
                            <div class="card text-center">
                                <div class="card-body">
                                    <h5 class="card-title">USB Storage</h5>
                                    <h3 class="text-info" id="usb-status">--</h3>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- LED Controls Section -->
                <div id="leds-section" class="section">
                    <div class="row">
                        <div class="col-lg-6 mb-4">
                            <div class="card">
                                <div class="card-body">
                                    <h5 class="card-title"><i class="fas fa-palette"></i> RGB LED Control</h5>
                                    <div class="mb-3">
                                        <label for="colorPicker" class="form-label">Color</label>
                                        <input type="color" id="colorPicker" value="#ff0000" onchange="updateRGBColor()">
                                    </div>
                                    <div class="mb-3">
                                        <label for="rgbMode" class="form-label">Mode</label>
                                        <select class="form-select" id="rgbMode" onchange="updateRGBMode()">
                                            <option value="solid">Solid Color</option>
                                            <option value="blink">Blink</option>
                                            <option value="pulse">Pulse</option>
                                            <option value="rainbow">Rainbow</option>
                                        </select>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div class="col-lg-6 mb-4">
                            <div class="card">
                                <div class="card-body">
                                    <h5 class="card-title"><i class="fas fa-lightbulb"></i> Large LED Control</h5>
                                    <div class="mb-3">
                                        <div class="form-check form-switch">
                                            <input class="form-check-input" type="checkbox" id="largeLedToggle" onchange="toggleLargeLED()">
                                            <label class="form-check-label" for="largeLedToggle">
                                                LED Power
                                            </label>
                                        </div>
                                    </div>
                                    <div class="mb-3">
                                        <label for="brightnessSlider" class="form-label">Brightness: <span id="brightnessValue">255</span></label>
                                        <input type="range" class="form-range" min="0" max="255" value="255" id="brightnessSlider" oninput="updateBrightness()">
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- USB Storage Section -->
                <div id="usb-section" class="section">
                    <div class="card">
                        <div class="card-header">
                            <h5><i class="fas fa-usb"></i> USB Storage Management</h5>
                        </div>
                        <div class="card-body">
                            <div id="usb-file-list">
                                <p class="text-muted">USB storage functionality will be implemented here...</p>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- WiFi Scanner Section -->
                <div id="wifi-section" class="section">
                    <div class="card">
                        <div class="card-header">
                            <h5><i class="fas fa-wifi"></i> WiFi Network Scanner</h5>
                        </div>
                        <div class="card-body">
                            <div id="wifi-scan-results">
                                <p class="text-muted">WiFi scanning results will appear here...</p>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Clock & Alerts Section -->
                <div id="clock-section" class="section">
                    <div class="row">
                        <div class="col-lg-6 mb-4">
                            <div class="card">
                                <div class="card-header">
                                    <h5><i class="fas fa-clock"></i> Current Time</h5>
                                </div>
                                <div class="card-body text-center">
                                    <h2 id="current-time">--:--:--</h2>
                                    <p id="current-date">----/--/--</p>
                                </div>
                            </div>
                        </div>
                        <div class="col-lg-6 mb-4">
                            <div class="card">
                                <div class="card-header">
                                    <h5><i class="fas fa-bell"></i> Hourly Alerts</h5>
                                </div>
                                <div class="card-body">
                                    <div class="form-check form-switch">
                                        <input class="form-check-input" type="checkbox" id="hourlyAlerts" checked onchange="toggleHourlyAlerts()">
                                        <label class="form-check-label" for="hourlyAlerts">
                                            Enable hourly LED alerts
                                        </label>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Temperature Section -->
                <div id="temperature-section" class="section">
                    <div class="card">
                        <div class="card-header">
                            <h5><i class="fas fa-thermometer-half"></i> Temperature Monitoring</h5>
                        </div>
                        <div class="card-body">
                            <div class="row">
                                <div class="col-lg-6">
                                    <h6>Current Reading</h6>
                                    <h3 class="text-warning" id="temp-current">--°C</h3>
                                </div>
                                <div class="col-lg-6">
                                    <h6>Temperature Log</h6>
                                    <div class="log-container" id="temp-log">
                                        <p class="text-muted">Temperature readings will appear here...</p>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Settings Section -->
                <div id="settings-section" class="section">
                    <div class="card">
                        <div class="card-header">
                            <h5><i class="fas fa-cog"></i> System Settings</h5>
                        </div>
                        <div class="card-body">
                            <h6>WiFi Configuration</h6>
                            <div class="row">
                                <div class="col-md-6 mb-3">
                                    <label for="wifiSSID" class="form-label">SSID</label>
                                    <input type="text" class="form-control" id="wifiSSID" placeholder="Enter WiFi SSID">
                                </div>
                                <div class="col-md-6 mb-3">
                                    <label for="wifiPassword" class="form-label">Password</label>
                                    <input type="password" class="form-control" id="wifiPassword" placeholder="Enter WiFi Password">
                                </div>
                            </div>
                            <button class="btn btn-primary" onclick="saveWiFiSettings()">Save WiFi Settings</button>
                            <hr>
                            <h6>System Information</h6>
                            <table class="table table-sm">
                                <tr><td>Chip Model:</td><td id="chip-model">--</td></tr>
                                <tr><td>Free Heap:</td><td id="free-heap">--</td></tr>
                                <tr><td>Flash Size:</td><td id="flash-size">--</td></tr>
                                <tr><td>PSRAM Size:</td><td id="psram-size">--</td></tr>
                            </table>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let socket;
        
        function initWebSocket() {
            socket = new WebSocket('ws://' + window.location.hostname + '/ws');
            socket.onopen = function(event) {
                console.log('WebSocket connected');
                refreshData();
            };
            
            socket.onmessage = function(event) {
                const data = JSON.parse(event.data);
                handleWebSocketMessage(data);
            };
            
            socket.onclose = function(event) {
                console.log('WebSocket disconnected, attempting to reconnect...');
                setTimeout(initWebSocket, 3000);
            };
        }
        
        function handleWebSocketMessage(data) {
            if (data.type === 'status') {
                document.getElementById('uptime').textContent = data.uptime;
                document.getElementById('wifi-status').textContent = data.wifi;
                document.getElementById('current-temp').textContent = data.temperature + '°C';
                document.getElementById('temp-current').textContent = data.temperature + '°C';
                document.getElementById('usb-status').textContent = data.usb;
                
                // Update system info
                if (data.chip_model) document.getElementById('chip-model').textContent = data.chip_model;
                if (data.free_heap) document.getElementById('free-heap').textContent = data.free_heap + ' bytes';
                if (data.flash_size) document.getElementById('flash-size').textContent = data.flash_size;
                if (data.psram_size) document.getElementById('psram-size').textContent = data.psram_size;
            } else if (data.type === 'time') {
                document.getElementById('current-time').textContent = data.time;
                document.getElementById('current-date').textContent = data.date;
            }
        }
        
        function showSection(sectionName) {
            // Hide all sections
            const sections = document.querySelectorAll('.section');
            sections.forEach(section => section.classList.remove('active'));
            
            // Show selected section
            document.getElementById(sectionName + '-section').classList.add('active');
            
            // Update page title
            const titles = {
                'dashboard': 'Dashboard',
                'leds': 'LED Controls',
                'usb': 'USB Storage',
                'wifi': 'WiFi Scanner',
                'clock': 'Clock & Alerts',
                'temperature': 'Temperature Log',
                'settings': 'Settings'
            };
            document.getElementById('pageTitle').textContent = titles[sectionName];
            
            // Update active nav link
            document.querySelectorAll('.nav-link').forEach(link => link.classList.remove('active'));
            event.target.classList.add('active');
        }
        
        function updateRGBColor() {
            const color = document.getElementById('colorPicker').value;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'rgb_color',
                    color: color
                }));
            }
        }
        
        function updateRGBMode() {
            const mode = document.getElementById('rgbMode').value;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'rgb_mode',
                    mode: mode
                }));
            }
        }
        
        function toggleLargeLED() {
            const state = document.getElementById('largeLedToggle').checked;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'large_led',
                    state: state
                }));
            }
        }
        
        function updateBrightness() {
            const brightness = document.getElementById('brightnessSlider').value;
            document.getElementById('brightnessValue').textContent = brightness;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'brightness',
                    value: parseInt(brightness)
                }));
            }
        }
        
        function toggleHourlyAlerts() {
            const enabled = document.getElementById('hourlyAlerts').checked;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'hourly_alerts',
                    enabled: enabled
                }));
            }
        }
        
        function saveWiFiSettings() {
            const ssid = document.getElementById('wifiSSID').value;
            const password = document.getElementById('wifiPassword').value;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({
                    type: 'wifi_config',
                    ssid: ssid,
                    password: password
                }));
            }
        }
        
        function refreshData() {
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({type: 'get_status'}));
                socket.send(JSON.stringify({type: 'get_time'}));
            }
        }
        
        // Initialize WebSocket when page loads
        window.addEventListener('load', initWebSocket);
        
        // Update time every second
        setInterval(() => {
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(JSON.stringify({type: 'get_time'}));
            }
        }, 1000);
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // Initialize Preferences
    preferences.begin("office-hub", false);
    
    // Load saved settings
    currentSSID = preferences.getString("ssid", "");
    currentPassword = preferences.getString("password", "");
    hourlyAlertEnabled = preferences.getBool("hourly_alert", true);
    ledMode = preferences.getString("led_mode", "solid");
    largeLedState = preferences.getBool("large_led_state", false);
    largeLedBrightness = preferences.getUChar("brightness", 255);
    
    // Initialize LEDs
    FastLED.addLeds<WS2812, RGB_LED_PIN, GRB>(rgbLeds, NUM_RGB_LEDS);
    FastLED.setBrightness(255);
    pinMode(LARGE_LED_PIN, OUTPUT);
    pinMode(ONBOARD_LED, OUTPUT);
    
    // Set initial LED states
    digitalWrite(LARGE_LED_PIN, largeLedState ? largeLedBrightness : LOW);
    rgbLeds[0] = CRGB::Red;
    FastLED.show();
    
    // Initialize temperature sensor
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
    
    // Initialize USB Host Manager
    usbManager.initialize();
    
    // Connect to WiFi
    if (currentSSID.length() > 0) {
        WiFi.begin(currentSSID.c_str(), currentPassword.c_str());
        Serial.print("Connecting to WiFi");
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.print("Connected! IP address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println();
            Serial.println("WiFi connection failed!");
        }
    } else {
        Serial.println("No WiFi credentials stored");
    }
    
    // Initialize NTP
    timeClient.begin();
    timeClient.update();
    
    // Web server routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });
    
    // API endpoints
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(1024);
        
        doc["uptime"] = formatUptime(millis());
        doc["wifi"] = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
        doc["temperature"] = String(currentTemperature, 1);
        doc["usb"] = usbManager.isMounted() ? "Connected" : "Not Connected";
        doc["free_heap"] = ESP.getFreeHeap();
        doc["chip_model"] = ESP.getChipModel();
        doc["flash_size"] = ESP.getFlashChipSize();
        doc["psram_size"] = ESP.getPsramSize();
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // WebSocket handling
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    
    // Start server
    server.begin();
    Serial.println("Web server started");
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                     void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            sendStatusUpdate(client);
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA:
            handleWebSocketMessage(client, (char*)data, len);
            break;
            
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void handleWebSocketMessage(AsyncWebSocketClient *client, char *data, size_t len) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, data, len);
    
    String type = doc["type"];
    
    if (type == "get_status") {
        sendStatusUpdate(client);
    } else if (type == "get_time") {
        sendTimeUpdate(client);
    } else if (type == "rgb_color") {
        String color = doc["color"];
        setRGBColor(color);
    } else if (type == "rgb_mode") {
        ledMode = doc["mode"].as<String>();
        preferences.putString("led_mode", ledMode);
    } else if (type == "large_led") {
        largeLedState = doc["state"];
        analogWrite(LARGE_LED_PIN, largeLedState ? largeLedBrightness : 0);
        preferences.putBool("large_led_state", largeLedState);
    } else if (type == "brightness") {
        largeLedBrightness = doc["value"];
        analogWrite(LARGE_LED_PIN, largeLedState ? largeLedBrightness : 0);
        preferences.putUChar("brightness", largeLedBrightness);
    } else if (type == "hourly_alerts") {
        hourlyAlertEnabled = doc["enabled"];
        preferences.putBool("hourly_alert", hourlyAlertEnabled);
    } else if (type == "wifi_config") {
        currentSSID = doc["ssid"].as<String>();
        currentPassword = doc["password"].as<String>();
        preferences.putString("ssid", currentSSID);
        preferences.putString("password", currentPassword);
        
        // Reconnect to new WiFi
        WiFi.begin(currentSSID.c_str(), currentPassword.c_str());
    }
}

void sendStatusUpdate(AsyncWebSocketClient *client = nullptr) {
    DynamicJsonDocument doc(1024);
    doc["type"] = "status";
    doc["uptime"] = formatUptime(millis());
    doc["wifi"] = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
    doc["temperature"] = String(currentTemperature, 1);
    doc["usb"] = usbManager.isMounted() ? "Connected" : "Not Connected";
    doc["free_heap"] = ESP.getFreeHeap();
    doc["chip_model"] = ESP.getChipModel();
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["psram_size"] = ESP.getPsramSize();
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void sendTimeUpdate(AsyncWebSocketClient *client = nullptr) {
    DynamicJsonDocument doc(512);
    doc["type"] = "time";
    
    timeClient.update();
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo = localtime(&rawTime);
    
    char timeStr[10];
    char dateStr[12];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeInfo);
    strftime(dateStr, sizeof(dateStr), "%Y/%m/%d", timeInfo);
    
    doc["time"] = timeStr;
    doc["date"] = dateStr;
    
    String response;
    serializeJson(doc, response);
    
    if (client) {
        client->text(response);
    } else {
        ws.textAll(response);
    }
}

void setRGBColor(String hexColor) {
    // Convert hex color to RGB values
    long number = strtol(&hexColor[1], NULL, 16);
    int r = number >> 16;
    int g = number >> 8 & 0xFF;
    int b = number & 0xFF;
    
    rgbLeds[0] = CRGB(r, g, b);
    FastLED.show();
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

void updateTemperature() {
    if (millis() - lastTempReading >= 5000) { // Every 5 seconds
        float temp;
        if (temperature_sensor_get_celsius(temp_sensor, &temp) == ESP_OK) {
            currentTemperature = temp;
        }
        lastTempReading = millis();
    }
}

void checkHourlyAlert() {
    if (!hourlyAlertEnabled) return;
    
    if (millis() - lastHourlyCheck >= 60000) { // Check every minute
        timeClient.update();
        time_t now = timeClient.getEpochTime();
        struct tm *timeinfo = localtime(&now);
        
        // Flash LED at the top of each hour
        if (timeinfo->tm_min == 0 && timeinfo->tm_sec == 0) {
            // Flash onboard LED 3 times
            for (int i = 0; i < 3; i++) {
                digitalWrite(ONBOARD_LED, HIGH);
                delay(200);
                digitalWrite(ONBOARD_LED, LOW);
                delay(200);
            }
        }
        
        lastHourlyCheck = millis();
    }
}

void performWiFiScan() {
    if (millis() - lastWiFiScan >= 300000) { // Every 5 minutes
        int n = WiFi.scanNetworks();
        
        // Store scan results (implement logging to SPIFFS/USB)
        DynamicJsonDocument scanResults(4096);
        scanResults["timestamp"] = timeClient.getEpochTime();
        JsonArray networks = scanResults.createNestedArray("networks");
        
        for (int i = 0; i < n; ++i) {
            JsonObject network = networks.createNestedObject();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["channel"] = WiFi.channel(i);
            network["encryption"] = WiFi.encryptionType(i);
        }
        
        // Log to serial for now (could be saved to file)
        String scanJson;
        serializeJson(scanResults, scanJson);
        Serial.println("WiFi Scan: " + scanJson);
        
        lastWiFiScan = millis();
    }
}

void handleLEDModes() {
    if (millis() - lastModeUpdate >= 50) { // Update every 50ms
        if (ledMode == "blink") {
            static bool blinkState = false;
            blinkState = !blinkState;
            rgbLeds[0] = blinkState ? CRGB::Red : CRGB::Black;
        } else if (ledMode == "pulse") {
            static uint8_t brightness = 0;
            static int8_t direction = 1;
            brightness += direction * 5;
            if (brightness >= 255 || brightness <= 0) direction *= -1;
            rgbLeds[0] = CRGB::Red;
            FastLED.setBrightness(brightness);
        } else if (ledMode == "rainbow") {
            hue += 2;
            rgbLeds[0] = CHSV(hue, 255, 255);
        }
        
        FastLED.show();
        lastModeUpdate = millis();
    }
}

void loop() {
    // Update NTP time
    timeClient.update();
    
    // Handle various periodic tasks
    updateTemperature();
    checkHourlyAlert();
    performWiFiScan();
    handleLEDModes();
    
    // Clean WebSocket connections
    ws.cleanupClients();
    
    // Send periodic status updates
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate >= 10000) { // Every 10 seconds
        sendStatusUpdate();
        lastStatusUpdate = millis();
    }
    
    delay(10);
}
