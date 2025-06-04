#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>

#define RGB_LED_PIN 48
#define LARGE_LED_PIN 47
#define NUM_RGB_LEDS 1
#define ONBOARD_LED 2

enum LEDMode {
    SOLID,
    BLINK,
    PULSE,
    RAINBOW,
    BREATHING,
    FADE
};

class LEDController {
private:
    CRGB rgbLeds[NUM_RGB_LEDS];
    bool largeLedState;
    uint8_t largeLedBrightness;
    uint8_t rgbBrightness;
    LEDMode currentMode;
    CRGB currentColor;
    uint8_t hue;
    uint8_t breathingBrightness;
    int8_t breathingDirection;
    bool blinkState;
    unsigned long lastUpdate;
    unsigned long modeUpdateInterval;
    
public:
    LEDController();
    bool begin();
    void update();
    
    // RGB LED control
    void setRGBColor(uint8_t r, uint8_t g, uint8_t b);
    void setRGBColor(CRGB color);
    void setRGBColorHex(const String& hexColor);
    void setRGBBrightness(uint8_t brightness);
    uint8_t getRGBBrightness();
    
    // LED modes
    void setMode(LEDMode mode);
    void setMode(const String& modeStr);
    LEDMode getMode();
    String getModeString();
    void setModeUpdateInterval(unsigned long interval);
    
    // Large LED control
    void setLargeLedState(bool state);
    bool getLargeLedState();
    void setLargeLedBrightness(uint8_t brightness);
    uint8_t getLargeLedBrightness();
    void toggleLargeLed();
    
    // Onboard LED
    void setOnboardLed(bool state);
    void blinkOnboardLed(int times = 1, int delayMs = 200);
    
    // Utility functions
    CRGB hexToColor(const String& hexColor);
    String colorToHex(CRGB color);
    void setAllOff();
    void flashAlert(CRGB color = CRGB::Red, int times = 3);
    
    // Animation methods
    void updateSolidMode();
    void updateBlinkMode();
    void updatePulseMode();
    void updateRainbowMode();
    void updateBreathingMode();
    void updateFadeMode();
};

#endif // LED_CONTROLLER_H
