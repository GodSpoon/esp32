#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "esp_temperature_sensor.h"

struct TemperatureReading {
    unsigned long timestamp;
    float temperature;
    bool isValid;
};

class TemperatureSensor {
private:
    temperature_sensor_handle_t tempSensor;
    float currentTemperature;
    float maxTemperature;
    float minTemperature;
    unsigned long lastReading;
    unsigned long readingInterval;
    bool isInitialized;
    std::vector<TemperatureReading> temperatureLog;
    size_t maxLogEntries;
    String logFilePath;
    
public:
    TemperatureSensor();
    ~TemperatureSensor();
    
    bool begin();
    void end();
    
    // Temperature reading
    bool readTemperature();
    float getCurrentTemperature();
    float getMaxTemperature();
    float getMinTemperature();
    bool isTemperatureValid();
    
    // Temperature monitoring
    void setReadingInterval(unsigned long interval);
    unsigned long getReadingInterval();
    void update();
    
    // Temperature logging
    void logTemperature();
    bool saveLogToFile();
    bool loadLogFromFile();
    String getLogJSON(int entries = 100);
    void clearLog();
    void setMaxLogEntries(size_t maxEntries);
    
    // Statistics
    float getAverageTemperature(unsigned long timeWindow = 3600000); // 1 hour default
    int getReadingCount();
    unsigned long getLastReadingTime();
    
    // Alerts
    bool isOverTemperature(float threshold);
    bool isUnderTemperature(float threshold);
    float getTemperatureTrend(); // Returns positive for warming, negative for cooling
    
    // Calibration
    void setTemperatureOffset(float offset);
    float getTemperatureOffset();
    
    // Utility functions
    float celsiusToFahrenheit(float celsius);
    float fahrenheitToCelsius(float fahrenheit);
    String formatTemperature(float temp, bool useFahrenheit = false);
};

#endif // TEMPERATURE_SENSOR_H
