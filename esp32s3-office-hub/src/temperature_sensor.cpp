#include "temperature_sensor.h"

TemperatureSensor::TemperatureSensor() {
    tempSensor = NULL;
    currentTemperature = 0.0;
    maxTemperature = -100.0;
    minTemperature = 100.0;
    lastReading = 0;
    readingInterval = 5000; // 5 seconds
    isInitialized = false;
    maxLogEntries = 1000;
    logFilePath = "/logs/temperature.json";
}

TemperatureSensor::~TemperatureSensor() {
    end();
}

bool TemperatureSensor::begin() {
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    
    esp_err_t result = temperature_sensor_install(&temp_sensor_config, &tempSensor);
    if (result != ESP_OK) {
        Serial.printf("Failed to install temperature sensor: %s\n", esp_err_to_name(result));
        return false;
    }
    
    result = temperature_sensor_enable(tempSensor);
    if (result != ESP_OK) {
        Serial.printf("Failed to enable temperature sensor: %s\n", esp_err_to_name(result));
        temperature_sensor_uninstall(tempSensor);
        tempSensor = NULL;
        return false;
    }
    
    isInitialized = true;
    
    // Create logs directory if it doesn't exist
    if (!SPIFFS.exists("/logs")) {
        // SPIFFS doesn't support mkdir, so we'll create the log file directly
    }
    
    // Load existing log
    loadLogFromFile();
    
    // Take initial reading
    readTemperature();
    
    Serial.println("Temperature sensor initialized successfully");
    return true;
}

void TemperatureSensor::end() {
    if (tempSensor) {
        temperature_sensor_disable(tempSensor);
        temperature_sensor_uninstall(tempSensor);
        tempSensor = NULL;
    }
    isInitialized = false;
}

bool TemperatureSensor::readTemperature() {
    if (!isInitialized || !tempSensor) {
        return false;
    }
    
    float temp;
    esp_err_t result = temperature_sensor_get_celsius(tempSensor, &temp);
    
    if (result == ESP_OK) {
        currentTemperature = temp;
        
        // Update min/max
        if (temp > maxTemperature) maxTemperature = temp;
        if (temp < minTemperature) minTemperature = temp;
        
        lastReading = millis();
        
        // Log temperature reading
        logTemperature();
        
        return true;
    } else {
        Serial.printf("Failed to read temperature: %s\n", esp_err_to_name(result));
        return false;
    }
}

float TemperatureSensor::getCurrentTemperature() {
    return currentTemperature;
}

float TemperatureSensor::getMaxTemperature() {
    return maxTemperature;
}

float TemperatureSensor::getMinTemperature() {
    return minTemperature;
}

bool TemperatureSensor::isTemperatureValid() {
    return isInitialized && (millis() - lastReading) < (readingInterval * 3);
}

void TemperatureSensor::setReadingInterval(unsigned long interval) {
    readingInterval = interval;
}

unsigned long TemperatureSensor::getReadingInterval() {
    return readingInterval;
}

void TemperatureSensor::update() {
    if (millis() - lastReading >= readingInterval) {
        readTemperature();
    }
}

void TemperatureSensor::logTemperature() {
    TemperatureReading reading;
    reading.timestamp = millis();
    reading.temperature = currentTemperature;
    reading.isValid = true;
    
    temperatureLog.push_back(reading);
    
    // Maintain maximum log size
    if (temperatureLog.size() > maxLogEntries) {
        temperatureLog.erase(temperatureLog.begin());
    }
    
    // Save to file periodically (every 10 readings)
    if (temperatureLog.size() % 10 == 0) {
        saveLogToFile();
    }
}

bool TemperatureSensor::saveLogToFile() {
    File file = SPIFFS.open(logFilePath, "w");
    if (!file) {
        Serial.println("Failed to open temperature log file for writing");
        return false;
    }
    
    DynamicJsonDocument doc(8192);
    JsonArray readings = doc.createNestedArray("readings");
    
    // Save last 500 readings to prevent file from getting too large
    size_t startIndex = temperatureLog.size() > 500 ? temperatureLog.size() - 500 : 0;
    
    for (size_t i = startIndex; i < temperatureLog.size(); i++) {
        JsonObject reading = readings.createNestedObject();
        reading["timestamp"] = temperatureLog[i].timestamp;
        reading["temperature"] = temperatureLog[i].temperature;
        reading["valid"] = temperatureLog[i].isValid;
    }
    
    serializeJson(doc, file);
    file.close();
    
    Serial.printf("Saved %d temperature readings to file\n", readings.size());
    return true;
}

bool TemperatureSensor::loadLogFromFile() {
    if (!SPIFFS.exists(logFilePath)) {
        Serial.println("No existing temperature log file found");
        return true;
    }
    
    File file = SPIFFS.open(logFilePath, "r");
    if (!file) {
        Serial.println("Failed to open temperature log file");
        return false;
    }
    
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse temperature log: %s\n", error.c_str());
        return false;
    }
    
    temperatureLog.clear();
    JsonArray readings = doc["readings"];
    
    for (JsonObject reading : readings) {
        TemperatureReading tempReading;
        tempReading.timestamp = reading["timestamp"];
        tempReading.temperature = reading["temperature"];
        tempReading.isValid = reading["valid"] | true;
        
        temperatureLog.push_back(tempReading);
        
        // Update min/max from loaded data
        float temp = tempReading.temperature;
        if (temp > maxTemperature) maxTemperature = temp;
        if (temp < minTemperature) minTemperature = temp;
    }
    
    Serial.printf("Loaded %d temperature readings from file\n", temperatureLog.size());
    return true;
}

String TemperatureSensor::getLogJSON(int entries) {
    DynamicJsonDocument doc(4096);
    JsonArray readings = doc.createNestedArray("readings");
    
    // Get the last 'entries' number of readings
    size_t startIndex = temperatureLog.size() > entries ? temperatureLog.size() - entries : 0;
    
    for (size_t i = startIndex; i < temperatureLog.size(); i++) {
        JsonObject reading = readings.createNestedObject();
        reading["timestamp"] = temperatureLog[i].timestamp;
        reading["temperature"] = temperatureLog[i].temperature;
        reading["valid"] = temperatureLog[i].isValid;
        
        // Convert timestamp to human-readable format
        time_t timestamp = temperatureLog[i].timestamp / 1000;
        struct tm timeinfo;
        localtime_r(&timestamp, &timeinfo);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        reading["time_str"] = timeStr;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

void TemperatureSensor::clearLog() {
    temperatureLog.clear();
    
    // Remove log file
    if (SPIFFS.exists(logFilePath)) {
        SPIFFS.remove(logFilePath);
    }
    
    Serial.println("Temperature log cleared");
}

void TemperatureSensor::setMaxLogEntries(size_t maxEntries) {
    maxLogEntries = maxEntries;
    
    // Trim current log if necessary
    if (temperatureLog.size() > maxLogEntries) {
        temperatureLog.erase(temperatureLog.begin(), 
                           temperatureLog.begin() + (temperatureLog.size() - maxLogEntries));
    }
}

float TemperatureSensor::getAverageTemperature(unsigned long timeWindow) {
    if (temperatureLog.empty()) {
        return 0.0;
    }
    
    unsigned long cutoffTime = millis() - timeWindow;
    float sum = 0.0;
    int count = 0;
    
    for (const auto& reading : temperatureLog) {
        if (reading.timestamp >= cutoffTime && reading.isValid) {
            sum += reading.temperature;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}

int TemperatureSensor::getReadingCount() {
    return temperatureLog.size();
}

unsigned long TemperatureSensor::getLastReadingTime() {
    return lastReading;
}

bool TemperatureSensor::isOverTemperature(float threshold) {
    return isTemperatureValid() && currentTemperature > threshold;
}

bool TemperatureSensor::isUnderTemperature(float threshold) {
    return isTemperatureValid() && currentTemperature < threshold;
}

float TemperatureSensor::getTemperatureTrend() {
    if (temperatureLog.size() < 10) {
        return 0.0; // Not enough data
    }
    
    // Calculate trend over last 10 readings
    float oldAvg = 0.0, newAvg = 0.0;
    int half = temperatureLog.size() / 2;
    
    // Average of first half
    for (int i = 0; i < half; i++) {
        oldAvg += temperatureLog[i].temperature;
    }
    oldAvg /= half;
    
    // Average of second half
    for (int i = half; i < temperatureLog.size(); i++) {
        newAvg += temperatureLog[i].temperature;
    }
    newAvg /= (temperatureLog.size() - half);
    
    return newAvg - oldAvg; // Positive = warming, negative = cooling
}

void TemperatureSensor::setTemperatureOffset(float offset) {
    // This would apply a calibration offset to readings
    // Implementation depends on specific needs
}

float TemperatureSensor::getTemperatureOffset() {
    return 0.0; // Placeholder
}

float TemperatureSensor::celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0 / 5.0) + 32.0;
}

float TemperatureSensor::fahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0) * 5.0 / 9.0;
}

String TemperatureSensor::formatTemperature(float temp, bool useFahrenheit) {
    if (useFahrenheit) {
        return String(celsiusToFahrenheit(temp), 1) + "°F";
    } else {
        return String(temp, 1) + "°C";
    }
}
