#include "led_controller.h"

LEDController::LEDController() {
    largeLedState = false;
    largeLedBrightness = 255;
    rgbBrightness = 128;
    currentMode = SOLID;
    currentColor = CRGB::Red;
    hue = 0;
    breathingBrightness = 0;
    breathingDirection = 1;
    blinkState = false;
    lastUpdate = 0;
    modeUpdateInterval = 50;
}

bool LEDController::begin() {
    // Initialize FastLED
    FastLED.addLeds<WS2812B, RGB_LED_PIN, GRB>(rgbLeds, NUM_RGB_LEDS);
    FastLED.setBrightness(rgbBrightness);
    FastLED.clear();
    FastLED.show();
    
    // Initialize large LED pin
    pinMode(LARGE_LED_PIN, OUTPUT);
    analogWrite(LARGE_LED_PIN, largeLedState ? largeLedBrightness : 0);
    
    // Initialize onboard LED
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, LOW);
    
    Serial.println("LED Controller initialized");
    return true;
}

void LEDController::update() {
    if (millis() - lastUpdate >= modeUpdateInterval) {
        switch (currentMode) {
            case SOLID:
                updateSolidMode();
                break;
            case BLINK:
                updateBlinkMode();
                break;
            case PULSE:
                updatePulseMode();
                break;
            case RAINBOW:
                updateRainbowMode();
                break;
            case BREATHING:
                updateBreathingMode();
                break;
            case FADE:
                updateFadeMode();
                break;
        }
        lastUpdate = millis();
    }
}

void LEDController::setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
    currentColor = CRGB(r, g, b);
    if (currentMode == SOLID) {
        rgbLeds[0] = currentColor;
        FastLED.show();
    }
}

void LEDController::setRGBColor(CRGB color) {
    currentColor = color;
    if (currentMode == SOLID) {
        rgbLeds[0] = currentColor;
        FastLED.show();
    }
}

void LEDController::setRGBColorHex(const String& hexColor) {
    CRGB color = hexToColor(hexColor);
    setRGBColor(color);
}

void LEDController::setRGBBrightness(uint8_t brightness) {
    rgbBrightness = brightness;
    FastLED.setBrightness(rgbBrightness);
    FastLED.show();
}

uint8_t LEDController::getRGBBrightness() {
    return rgbBrightness;
}

void LEDController::setMode(LEDMode mode) {
    currentMode = mode;
    Serial.printf("LED mode changed to: %s\n", getModeString().c_str());
}

void LEDController::setMode(const String& modeStr) {
    if (modeStr == "solid") currentMode = SOLID;
    else if (modeStr == "blink") currentMode = BLINK;
    else if (modeStr == "pulse") currentMode = PULSE;
    else if (modeStr == "rainbow") currentMode = RAINBOW;
    else if (modeStr == "breathing") currentMode = BREATHING;
    else if (modeStr == "fade") currentMode = FADE;
    else currentMode = SOLID;
    
    Serial.printf("LED mode changed to: %s\n", modeStr.c_str());
}

LEDMode LEDController::getMode() {
    return currentMode;
}

String LEDController::getModeString() {
    switch (currentMode) {
        case SOLID: return "solid";
        case BLINK: return "blink";
        case PULSE: return "pulse";
        case RAINBOW: return "rainbow";
        case BREATHING: return "breathing";
        case FADE: return "fade";
        default: return "solid";
    }
}

void LEDController::setModeUpdateInterval(unsigned long interval) {
    modeUpdateInterval = interval;
}

void LEDController::setLargeLedState(bool state) {
    largeLedState = state;
    analogWrite(LARGE_LED_PIN, largeLedState ? largeLedBrightness : 0);
}

bool LEDController::getLargeLedState() {
    return largeLedState;
}

void LEDController::setLargeLedBrightness(uint8_t brightness) {
    largeLedBrightness = brightness;
    if (largeLedState) {
        analogWrite(LARGE_LED_PIN, largeLedBrightness);
    }
}

uint8_t LEDController::getLargeLedBrightness() {
    return largeLedBrightness;
}

void LEDController::toggleLargeLed() {
    setLargeLedState(!largeLedState);
}

void LEDController::setOnboardLed(bool state) {
    digitalWrite(ONBOARD_LED, state ? HIGH : LOW);
}

void LEDController::blinkOnboardLed(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        setOnboardLed(true);
        delay(delayMs);
        setOnboardLed(false);
        if (i < times - 1) delay(delayMs);
    }
}

CRGB LEDController::hexToColor(const String& hexColor) {
    long number = strtol(&hexColor[1], NULL, 16);
    uint8_t r = number >> 16;
    uint8_t g = number >> 8 & 0xFF;
    uint8_t b = number & 0xFF;
    return CRGB(r, g, b);
}

String LEDController::colorToHex(CRGB color) {
    char hex[8];
    sprintf(hex, "#%02X%02X%02X", color.r, color.g, color.b);
    return String(hex);
}

void LEDController::setAllOff() {
    FastLED.clear();
    FastLED.show();
    setLargeLedState(false);
    setOnboardLed(false);
}

void LEDController::flashAlert(CRGB color, int times) {
    CRGB originalColor = currentColor;
    LEDMode originalMode = currentMode;
    
    for (int i = 0; i < times; i++) {
        rgbLeds[0] = color;
        FastLED.show();
        setOnboardLed(true);
        delay(200);
        
        rgbLeds[0] = CRGB::Black;
        FastLED.show();
        setOnboardLed(false);
        delay(200);
    }
    
    currentColor = originalColor;
    currentMode = originalMode;
}

void LEDController::updateSolidMode() {
    rgbLeds[0] = currentColor;
    FastLED.show();
}

void LEDController::updateBlinkMode() {
    blinkState = !blinkState;
    rgbLeds[0] = blinkState ? currentColor : CRGB::Black;
    FastLED.show();
}

void LEDController::updatePulseMode() {
    static uint8_t brightness = 0;
    static int8_t direction = 1;
    
    brightness += direction * 5;
    if (brightness >= 255 || brightness <= 0) {
        direction *= -1;
    }
    
    rgbLeds[0] = currentColor;
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void LEDController::updateRainbowMode() {
    hue += 2;
    rgbLeds[0] = CHSV(hue, 255, 255);
    FastLED.show();
}

void LEDController::updateBreathingMode() {
    breathingBrightness += breathingDirection * 3;
    if (breathingBrightness >= 255 || breathingBrightness <= 0) {
        breathingDirection *= -1;
    }
    
    rgbLeds[0] = currentColor;
    FastLED.setBrightness(breathingBrightness);
    FastLED.show();
}

void LEDController::updateFadeMode() {
    static uint8_t colorIndex = 0;
    CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Cyan, CRGB::Magenta};
    
    rgbLeds[0] = colors[colorIndex];
    FastLED.show();
    
    static unsigned long lastColorChange = 0;
    if (millis() - lastColorChange >= 1000) {
        colorIndex = (colorIndex + 1) % 6;
        lastColorChange = millis();
    }
}
