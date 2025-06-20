#include <gpio_viewer.h> // GPIO Viewer - must be first include
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h> // Required for random() and randomSeed()
#include <WiFi.h>    // WiFi functionality
#include <ArduinoOTA.h> // Over-The-Air updates

// Include all bitmap header files
#include "../images/bmp/all.h"

/*
 * ESP32 MEME MACHINE - Panel Display System
 * 
 * This device cycles through 7 different panels:
 * - 6 Meme image panels (Angy, Aw, Concorned, Korby, Smudge, Wat)
 * - 1 WiFi information panel (shows network status, signal strength, etc.)
 * 
 * Each panel displays for 5 seconds with random transition effects:
 * - Pixelated transition (random block reveal)
 * - Slide transition (left/right sliding)
 * - Fade transition (checkerboard fade-in)
 * - Wipe transition (horizontal/vertical line reveal)
 * - Spiral transition (circular expanding reveal)
 * 
 * WiFi auto-connects to preferred networks or creates AP mode as fallback.
 */

// WiFi Configuration
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

// WiFi networks in order of preference
const WiFiNetwork wifiNetworks[] = {
  {"Hail Satan", "stopeatingmyface"},
  {"ToTo", "stopeatingmyface"}
};
const int NUM_WIFI_NETWORKS = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// AP mode configuration (fallback)
const char* AP_SSID = "ESP32-MEME-MACHINE";
const char* AP_PASSWORD = "memes420";

// WiFi connection timeout
const unsigned long WIFI_TIMEOUT = 20000; // 20 seconds per network



// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Adafruit_SSD1306 constructor requires OLED_RESET define
#define I2C_SDA 6 // GPIO6 (D4 on SuperMini)
#define I2C_SCL 7 // GPIO7 (D5 on SuperMini)
#define OLED_ADDR 0x3C

// Button Configuration
#define RESET_BUTTON_PIN 3 // GPIO3 - Reset/Change screen button
#define MENU_BUTTON_PIN 2  // GPIO2 - Menu navigation button (new)
#define ONBOARD_BUTTON_PIN 9 // GPIO9 - Onboard boot/user button
#define BUTTON_DEBOUNCE_DELAY 50 // ms

// LED Configuration
#define ONBOARD_LED_PIN 8 // GPIO8 - Onboard red LED
#define LED_FLASH_DURATION 200 // ms - Duration for LED flash

// Battery Monitoring Configuration
#define BATTERY_ADC_PIN 0 // GPIO0 - Battery voltage monitoring (ADC1_CH0)
#define VOLTAGE_DIVIDER_RATIO 2.0 // Adjust based on your voltage divider circuit
#define BATTERY_MAX_VOLTAGE 4.2 // Maximum battery voltage (4.2V for Li-ion)
#define BATTERY_MIN_VOLTAGE 3.0 // Minimum usable battery voltage
#define BATTERY_SAMPLE_COUNT 10 // Number of samples to average for stable reading

// Initialize display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display cycling "panels" configuration 
enum DisplayMode
{
  DISPLAY_ANGY,      // Meme panel 1
  DISPLAY_AW,        // Meme panel 2
  DISPLAY_CONCORNED, // Meme panel 3
  DISPLAY_KORBY,     // Meme panel 4
  DISPLAY_SMUDGE,    // Meme panel 5
  DISPLAY_WAT,       // Meme panel 6
  DISPLAY_WIFI_INFO, // WiFi information panel
  DISPLAY_BATTERY,   // Battery status panel
  DISPLAY_SYSTEM_INFO // System information panel
};

// Menu system configuration
enum MenuMode
{
  MENU_NORMAL,    // Normal panel cycling mode
  MENU_BATTERY,   // Battery monitoring submenu
  MENU_SYSTEM     // System information submenu
};

DisplayMode currentDisplay = DISPLAY_ANGY; // Start with the first meme panel
MenuMode currentMenu = MENU_NORMAL; // Start in normal mode
unsigned long lastDisplayChange = 0;
const unsigned long DISPLAY_INTERVAL = 5000; // 5 seconds per panel

// Panel cycling configuration
const unsigned long AUTO_CYCLE_INTERVAL = 10000; // 10 seconds per panel
const unsigned long MENU_TIMEOUT = 30000; // 30 seconds before returning to normal mode

// Reset button state tracking
bool resetButtonLastState = HIGH;
unsigned long resetButtonLastChange = 0;
bool resetButtonProcessed = false;

// Menu button state tracking
bool menuButtonLastState = HIGH;
unsigned long menuButtonLastChange = 0;
bool menuButtonProcessed = false;
unsigned long lastMenuActivity = 0;

// Onboard button state tracking
bool onboardButtonLastState = HIGH;
unsigned long onboardButtonLastChange = 0;
bool onboardButtonProcessed = false;

// LED control variables
bool ledState = false;
unsigned long ledFlashStartTime = 0;

// Battery monitoring variables
float batteryVoltage = 0.0;
int batteryPercentage = 0;
unsigned long lastBatteryRead = 0;
const unsigned long BATTERY_READ_INTERVAL = 1000; // Read battery every second

// Add buffer for loading raw bitmaps
constexpr size_t BITMAP_BUF_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT / 8;
static uint8_t bitmapBuffer[BITMAP_BUF_SIZE];

// Structure to hold bitmap info for panels
struct BitmapInfo {
  const unsigned char* bitmap;  // Pointer to PROGMEM bitmap data
  uint16_t width;              // Direct value for width
  uint16_t height;             // Direct value for height
  const char* title;           // For serial logging
};

// Array of bitmap info for easy panel cycling
const BitmapInfo memes[] = {
    {angy, SCREEN_WIDTH, SCREEN_HEIGHT, "Angy"},
    {aw, SCREEN_WIDTH, SCREEN_HEIGHT, "Aw"},
    {concorned, SCREEN_WIDTH, SCREEN_HEIGHT, "Concorned"},
    {korby, SCREEN_WIDTH, SCREEN_HEIGHT, "Korby"},
    {smudge, SCREEN_WIDTH, SCREEN_HEIGHT, "Smudge"},
    {wat, SCREEN_WIDTH, SCREEN_HEIGHT, "Wat"}};

const int TOTAL_MEMES = sizeof(memes) / sizeof(memes[0]); // Total meme panels

// WiFi status tracking
bool wifiConnected = false;
bool isAPMode = false;
String connectedSSID = "";
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000; // Check WiFi every 30 seconds

// GPIOViewer object for real-time GPIO monitoring
GPIOViewer gpioViewer;

// Function prototypes
void displayPanelWithRandomTransition(int memeIndex);
void displayWiFiPanelWithRandomTransition();
void pixelatedTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height);
void slideTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height, int direction);
void fadeTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height);
void wipeTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height, bool vertical);
void spiralTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height);
void cyclePanels();
void displayModeMessage(const char* message, const char* submessage = nullptr);
bool connectToWiFi();
void startAPMode();
void displayWiFiStatus();
void checkWiFiStatus();
void displayWiFiInfoScreen();
void checkResetButton();
void checkMenuButton();
void checkOnboardButton();
void flashLED();
void updateLED();
void readBatteryVoltage();
float getBatteryVoltage();
int getBatteryPercentage();
void displayBatteryPanel();
void displaySystemInfoPanel();
void handleMenuNavigation();
void displayWiFiInfoScreen();

// Reset button function
void checkResetButton();

void setup()
{
  Serial.begin(115200);
  randomSeed(analogRead(BATTERY_ADC_PIN)); // Initialize random seed using battery ADC pin
  
  // Initialize buttons with internal pull-up resistors
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ONBOARD_BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize LED pin and turn it off by default
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  digitalWrite(ONBOARD_LED_PIN, LOW);  // Turn LED off by default
  
  // Initialize ADC for battery monitoring
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  
  // Debug: Check initial button states
  Serial.println("🎮 Button Initialization:");
  Serial.print("Reset Button (GPIO3) initial state: ");
  Serial.println(digitalRead(RESET_BUTTON_PIN) ? "HIGH" : "LOW");
  Serial.print("Menu Button (GPIO2) initial state: ");
  Serial.println(digitalRead(MENU_BUTTON_PIN) ? "HIGH" : "LOW");
  Serial.print("Onboard Button (GPIO9) initial state: ");
  Serial.println(digitalRead(ONBOARD_BUTTON_PIN) ? "HIGH" : "LOW");
  Serial.println("Buttons configured with INPUT_PULLUP");
  Serial.println("💡 LED Initialization:");
  Serial.print("Onboard LED (GPIO8) turned OFF by default");
  Serial.println();
  
  // Initial battery reading
  readBatteryVoltage();
  Serial.print("🔋 Initial battery voltage: ");
  Serial.print(batteryVoltage, 2);
  Serial.print("V (");
  Serial.print(batteryPercentage);
  Serial.println("%)");
  
  // Button test - wait for user to test buttons
  Serial.println("🧪 BUTTON TEST MODE - Press buttons to test:");
  Serial.println("Testing for 10 seconds... (includes LED flash test)");
  unsigned long testStart = millis();
  bool resetPressed = false;
  bool menuPressed = false;
  bool onboardPressed = false;

  while (millis() - testStart < 10000) {
    bool reset = digitalRead(RESET_BUTTON_PIN);
    bool menu = digitalRead(MENU_BUTTON_PIN);
    bool onboard = digitalRead(ONBOARD_BUTTON_PIN);

    if (!reset && !resetPressed) {
      Serial.println("Reset Button (GPIO3) pressed LOW");
      resetPressed = true; // Prevent continuous printing
    } else if (reset && resetPressed) {
      Serial.println("Reset Button (GPIO3) released HIGH");
      resetPressed = false;
    }
    
    if (!menu && !menuPressed) {
      Serial.println("Menu Button (GPIO2) pressed LOW");
      menuPressed = true; // Prevent continuous printing
    } else if (menu && menuPressed) {
      Serial.println("Menu Button (GPIO2) released HIGH");
      menuPressed = false;
    }
    
    if (!onboard && !onboardPressed) {
      Serial.println("Onboard Button (GPIO9) pressed LOW - FLASHING LED!");
      flashLED(); // Flash LED when onboard button is pressed
      onboardPressed = true; // Prevent continuous printing
    } else if (onboard && onboardPressed) {
      Serial.println("Onboard Button (GPIO9) released HIGH");
      onboardPressed = false;
    }
    
    // Update LED state (handle LED flash timing)
    updateLED();
    
    delay(50); // Short delay for debouncing and to avoid spamming serial too much
  }
  Serial.println("Button test complete.");
  
  // Initialize I2C and OLED Display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println("SSD1306 allocation failed");
    while (1)
      ; // Halt if display not found
  }

  // Show epic startup message
  display.clearDisplay();
  display.setTextSize(2); // Larger text for "MEME MACHINE"
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 15); // Adjusted for better centering
  display.println("MEME");
  display.setCursor(10, 35);  
  display.println("MACHINE");
  display.display();
  delay(2000);

  // Initialize WiFi
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(30, 20);
  display.print("Connecting WiFi...");
  display.display();
  
  Serial.println("🔗 Attempting WiFi connection...");
  wifiConnected = connectToWiFi();
  
  if (!wifiConnected) {
    Serial.println("🔥 All networks failed, starting AP mode...");
    startAPMode();
  }
  
  displayWiFiStatus();
  delay(2000);

  // Initialize OTA (Over-The-Air updates) if WiFi is connected
  if (wifiConnected && !isAPMode) {
    ArduinoOTA.setHostname("ESP32-MEME-MACHINE");
    ArduinoOTA.setPassword("admin");
    
    ArduinoOTA.onStart([]() {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(20, 20);
      display.print("OTA Update");
      display.setCursor(25, 35);
      display.print("Starting...");
      display.display();
      Serial.println("🔄 OTA Update Starting...");
    });
    
    ArduinoOTA.onEnd([]() {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(15, 20);
      display.print("OTA Complete!");
      display.setCursor(20, 35);
      display.print("Rebooting...");
      display.display();
      Serial.println("✅ OTA Update Complete!");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      int percent = (progress / (total / 100));
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(20, 15);
      display.print("OTA Update");
      display.setCursor(30, 30);
      display.print(percent);
      display.print("%");
      // Progress bar
      display.drawRect(10, 45, 108, 8, SSD1306_WHITE);
      display.fillRect(12, 47, (104 * percent) / 100, 4, SSD1306_WHITE);
      display.display();
      Serial.printf("OTA Progress: %u%%\r", percent);
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(25, 20);
      display.print("OTA Error:");
      display.setCursor(30, 35);
      if (error == OTA_AUTH_ERROR) display.print("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) display.print("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) display.print("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) display.print("Receive Failed");
      else if (error == OTA_END_ERROR) display.print("End Failed");
      display.display();
      Serial.printf("❌ OTA Error[%u]: ", error);
      delay(3000);
    });
    
    ArduinoOTA.begin();
    Serial.println("🚀 OTA Ready! IP: " + WiFi.localIP().toString());
  }

  // Show loading animation (simplified)
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(30, 28);  
  display.print("Loading Memes...");
  display.display();
  delay(1500);

  // Show button instructions
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 15);
  display.print("Button Controls:");
  display.setCursor(15, 30);
  display.print("Reset: Change Screen");
  display.setCursor(10, 45);
  display.print("Auto-cycles panels");
  display.display();
  delay(3000);

  // Start with first meme panel, no transition, just display it
  displayPanelWithRandomTransition(currentDisplay);
  lastDisplayChange = millis();
  lastWiFiCheck = millis();

  Serial.println("🐱 MEME MACHINE ACTIVATED! 🐱");
  Serial.print("Total meme panels loaded: "); Serial.println(TOTAL_MEMES);
  Serial.print("Displaying panel: "); Serial.println(memes[currentDisplay].title);
  Serial.println("Image rendering using display.drawBitmap().");
  Serial.println("🎮 Controls:");
  Serial.println("   Reset Button (GPIO3): Change panel/navigate in menu");
  Serial.println("   Menu Button (GPIO2): Enter menu/back to normal mode");
  Serial.println("   Auto-cycle: Panels change every 10 seconds (normal mode)");
  
  // Initialize GPIOViewer for real-time GPIO monitoring (must be at end of setup)
  if (wifiConnected && !isAPMode) {
    Serial.println("🔍 Initializing GPIOViewer for GPIO monitoring...");
    
    // Set a faster sampling interval for better button debugging (25ms)
    gpioViewer.setSamplingInterval(25); // 25ms sampling interval for responsive monitoring
    gpioViewer.setPort(8080); // Default port 8080
    
    // Configure GPIOViewer to monitor our button pins specifically
    // Note: GPIOViewer will automatically detect and monitor all GPIO pins
    
    gpioViewer.begin();
    
    Serial.println("🔍 GPIOViewer started successfully!");
    Serial.println("🔍 Web interface: http://" + WiFi.localIP().toString() + ":8080");
    Serial.println("🔍 Monitor buttons and battery in real-time:");
    Serial.println("🔍   - GPIO2 (Menu Button)");
    Serial.println("🔍   - GPIO3 (Reset Button)"); 
    Serial.println("🔍   - GPIO0 (Battery ADC)");
    Serial.println("🔍   - GPIO6 (I2C SDA)");
    Serial.println("🔍   - GPIO7 (I2C SCL)");
    Serial.println("🔍 Press buttons to see real-time GPIO state changes!");
    
    // Display GPIOViewer info on OLED briefly
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 5);
    display.print("GPIOViewer Ready!");
    display.setCursor(0, 20);
    display.print("Web: ");
    display.print(WiFi.localIP());
    display.setCursor(0, 35);
    display.print("Port: 8080");
    display.setCursor(0, 50);
    display.print("Monitor GPIO 3");
    display.display();
    delay(3000);
    
  } else {
    Serial.println("🔍 GPIOViewer skipped - WiFi not connected or in AP mode");
    if (isAPMode) {
      Serial.println("🔍 Note: GPIOViewer requires station mode (not AP mode)");
      Serial.println("🔍 Connect to a WiFi network to enable GPIO monitoring");
    }
  }
}

void loop()
{
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Check button inputs
  checkResetButton();
  checkMenuButton();
  checkOnboardButton();
  
  // Handle menu navigation and timeouts
  handleMenuNavigation();
  
  // Read battery voltage periodically
  if (millis() - lastBatteryRead >= BATTERY_READ_INTERVAL) {
    readBatteryVoltage();
    lastBatteryRead = millis();
  }
  
  // Debug: Print button states and battery info every 3 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 3000) {
    Serial.print("Reset: ");
    Serial.print(digitalRead(RESET_BUTTON_PIN) ? "H" : "L");
    Serial.print(" | Menu: ");
    Serial.print(digitalRead(MENU_BUTTON_PIN) ? "H" : "L");
    Serial.print(" | Mode: ");
    Serial.print(currentMenu == MENU_NORMAL ? "NORMAL" : 
                currentMenu == MENU_BATTERY ? "BATTERY" : "SYSTEM");
    Serial.print(" | Battery: ");
    Serial.print(batteryVoltage, 2);
    Serial.print("V (");
    Serial.print(batteryPercentage);
    Serial.print("%)");
    
    // GPIOViewer reminder
    if (wifiConnected && !isAPMode) {
      Serial.print(" | GPIOViewer: http://");
      Serial.print(WiFi.localIP());
      Serial.print(":8080");
    }
    
    Serial.println();
    lastDebug = millis();
  }
  
  // Check if it's time to change panel (only in normal auto-cycle mode)
  if (currentMenu == MENU_NORMAL) {
    unsigned long currentInterval = AUTO_CYCLE_INTERVAL;
    if (millis() - lastDisplayChange >= currentInterval)
    {
      cyclePanels();
    }
  }

  // Periodically check WiFi status
  if (millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL)
  {
    checkWiFiStatus();
    lastWiFiCheck = millis();
  }

  delay(100); // Small delay to keep things responsive
}

// Display the meme panel with a random transition effect
void displayPanelWithRandomTransition(int memeIndex) {
  if (memeIndex < 0 || memeIndex >= TOTAL_MEMES) {
    Serial.println("Error: Invalid meme index in displayPanelWithRandomTransition.");
    return;
  }
  
  const BitmapInfo& meme = memes[memeIndex];
  
  // Choose random transition effect
  static bool firstDisplay = true;
  if (firstDisplay) {
    // First panel display - clear screen then use transition
    display.clearDisplay();
    display.display();
    delay(200); // Brief pause before first transition
    firstDisplay = false;
  }
  
  // Random transition selection (5 different effects)
  int transitionType = random(0, 5);
  
  Serial.print("Panel transition type: ");
  switch(transitionType) {
    case 0:
      Serial.println("Pixelated");
      pixelatedTransition(meme.bitmap, meme.width, meme.height);
      break;
    case 1:
      Serial.println("Slide from right");
      slideTransition(meme.bitmap, meme.width, meme.height, 0);
      break;
    case 2:
      Serial.println("Slide from left");
      slideTransition(meme.bitmap, meme.width, meme.height, 1);
      break;
    case 3:
      Serial.println("Vertical wipe");
      wipeTransition(meme.bitmap, meme.width, meme.height, true);
      break;
    case 4:
      Serial.println("Spiral");
      spiralTransition(meme.bitmap, meme.width, meme.height);
      break;
    default:
      // Fallback to pixelated
      pixelatedTransition(meme.bitmap, meme.width, meme.height);
      break;
  }
}

// Display the WiFi info panel with a random transition effect
void displayWiFiPanelWithRandomTransition() {
  // Random transition selection for WiFi panel
  int transitionType = random(0, 5);
  
  Serial.print("WiFi panel transition type: ");
  switch(transitionType) {
    case 0:
      Serial.println("Pixelated fade-in");
      // Create a temporary bitmap buffer for WiFi screen
      display.clearDisplay();
      displayWiFiInfoScreen(); // This renders to display buffer
      // Since we can't easily extract the display buffer, use fade effect
      fadeTransition(nullptr, SCREEN_WIDTH, SCREEN_HEIGHT);
      break;
    case 1:
      Serial.println("Horizontal wipe");
      display.clearDisplay();
      display.display();
      delay(100);
      wipeTransition(nullptr, SCREEN_WIDTH, SCREEN_HEIGHT, false);
      displayWiFiInfoScreen();
      break;
    case 2:
      Serial.println("Vertical wipe");
      display.clearDisplay(); 
      display.display();
      delay(100);
      wipeTransition(nullptr, SCREEN_WIDTH, SCREEN_HEIGHT, true);
      displayWiFiInfoScreen();
      break;
    case 3:
      Serial.println("Slide in");
      display.clearDisplay();
      display.display();
      delay(100);
      slideTransition(nullptr, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
      displayWiFiInfoScreen();
      break;
    default:
      Serial.println("Direct display");
      displayWiFiInfoScreen();
      break;
  }
}

// Pixelated transition effect for panels
void pixelatedTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height) {
  const int PIXEL_SIZE = 4; // Size of pixelated blocks
  const int STEPS = 10;     // Increased steps for smoother transition
  
  // Create array to track which blocks have been revealed
  const int blocksX = (width + PIXEL_SIZE - 1) / PIXEL_SIZE;
  const int blocksY = (height + PIXEL_SIZE - 1) / PIXEL_SIZE;
  const int totalBlocks = blocksX * blocksY;
  
  // Create array of block indices for random order
  int* blockOrder = new int[totalBlocks];
  for (int i = 0; i < totalBlocks; i++) {
    blockOrder[i] = i;
  }
  
  // Shuffle the array using Fisher-Yates algorithm
  for (int i = totalBlocks - 1; i > 0; i--) {
    int j = random(i + 1);
    int temp = blockOrder[i];
    blockOrder[i] = blockOrder[j];
    blockOrder[j] = temp;
  }
  
  // Perform transition in steps
  int blocksPerStep = max(1, totalBlocks / STEPS);
  
  for (int step = 0; step < STEPS; step++) {
    int startIdx = step * blocksPerStep;
    int endIdx = min((step + 1) * blocksPerStep, totalBlocks);
    
    // Reveal blocks for this step
    for (int idx = startIdx; idx < endIdx; idx++) {
      int blockNum = blockOrder[idx];
      int blockX = blockNum % blocksX;
      int blockY = blockNum / blocksX;
      
      int pixelX = blockX * PIXEL_SIZE;
      int pixelY = blockY * PIXEL_SIZE;
      int blockWidth = min(PIXEL_SIZE, (int)width - pixelX);
      int blockHeight = min(PIXEL_SIZE, (int)height - pixelY);
      
      // Clear the block area and draw the new bitmap section
      display.fillRect(pixelX, pixelY, blockWidth, blockHeight, SSD1306_BLACK);
      
      // Draw the new bitmap section
      for (int y = 0; y < blockHeight; y++) {
        for (int x = 0; x < blockWidth; x++) {
          int globalX = pixelX + x;
          int globalY = pixelY + y;
          
          // Calculate byte position in bitmap
          int byteIndex = (globalY * width + globalX) / 8;
          int bitIndex = 7 - ((globalY * width + globalX) % 8);
          
          uint8_t byte = pgm_read_byte(&newBitmap[byteIndex]);
          if (byte & (1 << bitIndex)) {
            display.drawPixel(globalX, globalY, SSD1306_WHITE);
          }
        }
      }
    }
    
    display.display();
    delay(40); // Slightly faster transition for smoother effect
  }
  
  // Clean up
  delete[] blockOrder;
  
  // Final complete redraw to ensure everything is correct
  display.clearDisplay();
  display.drawBitmap(0, 0, newBitmap, width, height, SSD1306_WHITE);
  display.display();
}

// Slide transition effect for panels - slides new bitmap from specified direction
void slideTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height, int direction) {
  const int STEPS = 16; // Number of slide steps
  const int stepSize = width / STEPS;
  
  for (int step = 0; step < STEPS; step++) {
    display.clearDisplay();
    
    if (direction == 0) { // Slide from right
      int xOffset = width - (step + 1) * stepSize;
      if (newBitmap != nullptr) {
        display.drawBitmap(xOffset, 0, newBitmap, width, height, SSD1306_WHITE);
      }
    } else { // Slide from left
      int xOffset = -width + (step + 1) * stepSize;
      if (newBitmap != nullptr) {
        display.drawBitmap(xOffset, 0, newBitmap, width, height, SSD1306_WHITE);
      }
    }
    
    display.display();
    delay(30);
  }
  
  // Final position
  if (newBitmap != nullptr) {
    display.clearDisplay();
    display.drawBitmap(0, 0, newBitmap, width, height, SSD1306_WHITE);
    display.display();
  }
}

// Fade transition effect for panels - gradually reveals new bitmap
void fadeTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height) {
  const int STEPS = 8;
  
  for (int step = 0; step < STEPS; step++) {
    display.clearDisplay();
    
    // Create a checkerboard pattern that gets denser each step
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        // Use step to determine probability of showing pixel
        if ((x + y + step) % (STEPS - step + 1) == 0) {
          if (newBitmap != nullptr) {
            int byteIndex = (y * width + x) / 8;
            int bitIndex = 7 - ((y * width + x) % 8);
            uint8_t byte = pgm_read_byte(&newBitmap[byteIndex]);
            if (byte & (1 << bitIndex)) {
              display.drawPixel(x, y, SSD1306_WHITE);
            }
          } else {
            // For WiFi panel, just show a pattern
            if ((x + y) % 3 == 0) {
              display.drawPixel(x, y, SSD1306_WHITE);
            }
          }
        }
      }
    }
    
    display.display();
    delay(60);
  }
  
  // Final complete image
  if (newBitmap != nullptr) {
    display.clearDisplay();
    display.drawBitmap(0, 0, newBitmap, width, height, SSD1306_WHITE);
    display.display();
  }
}

// Wipe transition effect for panels - reveals new bitmap with a moving line
void wipeTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height, bool vertical) {
  const int STEPS = 20;
  
  for (int step = 0; step <= STEPS; step++) {
    display.clearDisplay();
    
    if (vertical) {
      // Vertical wipe from top to bottom
      int revealHeight = (height * step) / STEPS;
      if (newBitmap != nullptr) {
        // Draw only the revealed portion
        for (int y = 0; y < revealHeight; y++) {
          for (int x = 0; x < width; x++) {
            int byteIndex = (y * width + x) / 8;
            int bitIndex = 7 - ((y * width + x) % 8);
            uint8_t byte = pgm_read_byte(&newBitmap[byteIndex]);
            if (byte & (1 << bitIndex)) {
              display.drawPixel(x, y, SSD1306_WHITE);
            }
          }
        }
      }
      // Draw wipe line
      display.drawLine(0, revealHeight, width-1, revealHeight, SSD1306_WHITE);
    } else {
      // Horizontal wipe from left to right
      int revealWidth = (width * step) / STEPS;
      if (newBitmap != nullptr) {
        // Draw only the revealed portion
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < revealWidth; x++) {
            int byteIndex = (y * width + x) / 8;
            int bitIndex = 7 - ((y * width + x) % 8);
            uint8_t byte = pgm_read_byte(&newBitmap[byteIndex]);
            if (byte & (1 << bitIndex)) {
              display.drawPixel(x, y, SSD1306_WHITE);
            }
          }
        }
      }
      // Draw wipe line
      display.drawLine(revealWidth, 0, revealWidth, height-1, SSD1306_WHITE);
    }
    
    display.display();
    delay(40);
  }
  
  // Final complete image
  if (newBitmap != nullptr) {
    display.clearDisplay();
    display.drawBitmap(0, 0, newBitmap, width, height, SSD1306_WHITE);
    display.display();
  }
}

// Spiral transition effect for panels - reveals bitmap in a spiral pattern
void spiralTransition(const unsigned char* newBitmap, uint16_t width, uint16_t height) {
  const int STEPS = 30;
  const int centerX = width / 2;
  const int centerY = height / 2;
  const int maxRadius = max(width, height);
  
  for (int step = 0; step <= STEPS; step++) {
    display.clearDisplay();
    
    int currentRadius = (maxRadius * step) / STEPS;
    
    if (newBitmap != nullptr) {
      // Reveal pixels within spiral radius
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          int dx = x - centerX;
          int dy = y - centerY;
          int distance = sqrt(dx*dx + dy*dy);
          
          if (distance <= currentRadius) {
            int byteIndex = (y * width + x) / 8;
            int bitIndex = 7 - ((y * width + x) % 8);
            uint8_t byte = pgm_read_byte(&newBitmap[byteIndex]);
            if (byte & (1 << bitIndex)) {
              display.drawPixel(x, y, SSD1306_WHITE);
            }
          }
        }
      }
    }
    
    // Draw spiral border
    display.drawCircle(centerX, centerY, currentRadius, SSD1306_WHITE);
    
    display.display();
    delay(50);
  }
  
  // Final complete image
  if (newBitmap != nullptr) {
    display.clearDisplay();
    display.drawBitmap(0, 0, newBitmap, width, height, SSD1306_WHITE);
    display.display();
  }
}

// Function to handle the cycling of panels
void cyclePanels()
{
  // Include WiFi info panel in the rotation
  if (currentDisplay == DISPLAY_WAT) {
    // Switch to WiFi info panel after the last meme panel
    currentDisplay = DISPLAY_WIFI_INFO;
    Serial.println("Displaying WiFi Information Panel");
    displayWiFiPanelWithRandomTransition();
  } else if (currentDisplay == DISPLAY_WIFI_INFO) {
    // Return to first meme panel after WiFi info panel
    currentDisplay = DISPLAY_ANGY;
    Serial.print("Cycling to panel: ");
    Serial.println(memes[currentDisplay].title);
    displayPanelWithRandomTransition(currentDisplay);
  } else {
    // Normal meme panel cycling
    currentDisplay = static_cast<DisplayMode>((static_cast<int>(currentDisplay) + 1));
    Serial.print("Cycling to panel: ");
    Serial.println(memes[currentDisplay].title);
    displayPanelWithRandomTransition(currentDisplay);
  }
  
  lastDisplayChange = millis();
}

// WiFi connection function - tries each network in order
bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  
  for (int i = 0; i < NUM_WIFI_NETWORKS; i++) {
    Serial.print("Trying to connect to: ");
    Serial.println(wifiNetworks[i].ssid);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.print("Connecting to:");
    display.setCursor(0, 25);
    display.print(wifiNetworks[i].ssid);
    display.display();
    
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT) {
      delay(500);
      Serial.print(".");
      
      // Update display with dots to show progress
      static int dotCount = 0;
      display.setCursor(0, 40);
      String dots = "Connecting";
      for (int j = 0; j < (dotCount % 4); j++) {
        dots += ".";
      }
      display.fillRect(0, 40, 128, 10, SSD1306_BLACK); // Clear the line
      display.setCursor(0, 40);
      display.print(dots);
      display.display();
      dotCount++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("✅ Connected to: ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      
      connectedSSID = String(wifiNetworks[i].ssid);
      return true;
    }
    
    Serial.println();
    Serial.print("❌ Failed to connect to: ");
    Serial.println(wifiNetworks[i].ssid);
    WiFi.disconnect();
    delay(1000);
  }
  
  return false;
}

// Start Access Point mode
void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("🔥 AP Mode started! SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  isAPMode = true;
  connectedSSID = String(AP_SSID);
}

// Display WiFi status on OLED
void displayWiFiStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  
  if (isAPMode) {
    display.setCursor(0, 5);
    display.print("AP Mode Active");
    display.setCursor(0, 20);
    display.print("SSID: ");
    display.print(AP_SSID);
    display.setCursor(0, 35);
    display.print("Password: ");
    display.print(AP_PASSWORD);
    display.setCursor(0, 50);
    display.print("IP: ");
    display.print(WiFi.softAPIP());
  } else {
    display.setCursor(0, 10);
    display.print("WiFi Connected!");
    display.setCursor(0, 25);
    display.print("SSID: ");
    display.print(connectedSSID);
    display.setCursor(0, 40);
    display.print("IP: ");
    display.print(WiFi.localIP());
  }
  
  display.display();
}

// Check WiFi status and reconnect if needed
void checkWiFiStatus() {
  if (!isAPMode) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("🔄 WiFi disconnected, attempting reconnection...");
      wifiConnected = connectToWiFi();
      
      if (!wifiConnected) {
        Serial.println("🔥 Reconnection failed, switching to AP mode...");
        startAPMode();
      }
    } else {
      Serial.print("📶 WiFi still connected to: ");
      Serial.print(connectedSSID);
      Serial.print(" (");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm)");
    }
  } else {
    // In AP mode, show connected clients count
    Serial.print("🔥 AP Mode: ");
    Serial.print(WiFi.softAPgetStationNum());
    Serial.println(" clients connected");
  }
}

// Display WiFi information screen with nice formatting
void displayWiFiInfoScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Title with decorative border
  display.setTextSize(1);
  display.drawRect(0, 0, 128, 10, SSD1306_WHITE);
  display.fillRect(1, 1, 126, 8, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(35, 2);
  display.print("WiFi Status");
  display.setTextColor(SSD1306_WHITE);
  
  if (isAPMode) {
    // AP Mode display
    display.setTextSize(1);
    display.setCursor(2, 15);
    display.print("Mode: Access Point");
    
    display.setCursor(2, 25);
    display.print("SSID: ");
    display.print(AP_SSID);
    
    display.setCursor(2, 35);
    display.print("Pass: ");
    display.print(AP_PASSWORD);
    
    display.setCursor(2, 45);
    display.print("IP: ");
    display.print(WiFi.softAPIP());
    
    display.setCursor(2, 55);
    display.print("Clients: ");
    display.print(WiFi.softAPgetStationNum());
    
    // WiFi icon for AP mode (simplified antenna)
    display.drawCircle(115, 20, 3, SSD1306_WHITE);
    display.drawLine(115, 17, 115, 13, SSD1306_WHITE);
    display.drawLine(113, 15, 117, 15, SSD1306_WHITE);
    
  } else if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    // Station Mode display
    display.setTextSize(1);
    display.setCursor(2, 15);
    display.print("Connected to:");
    
    display.setCursor(2, 25);
    display.print(connectedSSID);
    
    display.setCursor(2, 35);
    display.print("IP: ");
    display.print(WiFi.localIP());
    
    // Signal strength
    int rssi = WiFi.RSSI();
    display.setCursor(2, 45);
    display.print("Signal: ");
    display.print(rssi);
    display.print(" dBm");
    
    // Signal quality indicator
    display.setCursor(2, 55);
    display.print("Quality: ");
    if (rssi > -50) {
      display.print("Excellent");
    } else if (rssi > -60) {
      display.print("Good");
    } else if (rssi > -70) {
      display.print("Fair");
    } else {
      display.print("Weak");
    }
    
    // Signal strength bars (visual indicator)
    int barHeight = map(constrain(rssi, -90, -30), -90, -30, 1, 8);
    for (int i = 0; i < 4; i++) {
      int currentBarHeight = constrain(barHeight - (i * 2), 0, 8);
      if (currentBarHeight > 0) {
        display.fillRect(110 + (i * 3), 20 + (8 - currentBarHeight), 2, currentBarHeight, SSD1306_WHITE);
      } else {
        display.drawRect(110 + (i * 3), 20, 2, 8, SSD1306_WHITE);
      }
    }
    
  } else {
    // Disconnected state
    display.setTextSize(1);
    display.setCursor(30, 25);
    display.print("WiFi Disconnected");
    
    display.setCursor(25, 40);
    display.print("Attempting to");
    display.setCursor(35, 50);
    display.print("reconnect...");
    
    // Animated dots for connecting
    static int dotAnimation = 0;
    dotAnimation = (dotAnimation + 1) % 4;
    for (int i = 0; i < dotAnimation; i++) {
      display.fillCircle(95 + (i * 6), 50, 1, SSD1306_WHITE);
    }
  }
  
  display.display();
}

void displayModeMessage(const char* message, const char* submessage) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Draw a border for emphasis
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(1, 1, 126, 62, SSD1306_WHITE);
  
  // Main message
  int messageLen = strlen(message);
  int x = (128 - (messageLen * 6)) / 2; // Center text (6 pixels per character)
  display.setCursor(x, 20);
  display.print(message);
  
  // Submessage
  if (submessage != nullptr) {
    int submessageLen = strlen(submessage);
    int subX = (128 - (submessageLen * 6)) / 2;
    display.setCursor(subX, 35);
    display.print(submessage);
  }
  
  display.display();
  delay(2000); // Show the message for 2 seconds
}

// Read battery voltage and calculate percentage
void readBatteryVoltage() {
  // Take multiple samples for stable reading
  long adcSum = 0;
  for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    adcSum += analogRead(BATTERY_ADC_PIN);
    delay(1); // Small delay between samples
  }
  
  // Calculate average ADC reading
  int avgADC = adcSum / BATTERY_SAMPLE_COUNT;
  
  // Convert ADC reading to voltage
  // ESP32-C3 ADC reference is 3.3V with 12-bit resolution (0-4095)
  float adcVoltage = (avgADC / 4095.0) * 3.3;
  
  // Apply voltage divider ratio to get actual battery voltage
  batteryVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO;
  
  // Calculate battery percentage
  batteryPercentage = map(constrain(batteryVoltage * 100, BATTERY_MIN_VOLTAGE * 100, BATTERY_MAX_VOLTAGE * 100), 
                         BATTERY_MIN_VOLTAGE * 100, BATTERY_MAX_VOLTAGE * 100, 0, 100);
  
  // Ensure percentage is within bounds
  batteryPercentage = constrain(batteryPercentage, 0, 100);
}

// Menu button handling with debouncing
void checkMenuButton() {
  unsigned long currentTime = millis();
  
  // Read current button state
  bool menuButtonState = digitalRead(MENU_BUTTON_PIN);
  
  // Menu Button Logic
  if (menuButtonState != menuButtonLastState) {
    menuButtonLastChange = currentTime;
    menuButtonProcessed = false;
    Serial.print("Menu button state changed to: ");
    Serial.println(menuButtonState ? "HIGH" : "LOW");
  }
  
  if (!menuButtonProcessed && (currentTime - menuButtonLastChange) > BUTTON_DEBOUNCE_DELAY) {
    if (menuButtonState == LOW && menuButtonLastState == HIGH) {
      // Button press detected (falling edge after debounce)
      Serial.println("MENU BUTTON PRESSED!");
      menuButtonProcessed = true;
      lastMenuActivity = currentTime;
      
      // Toggle between menu modes
      switch (currentMenu) {
        case MENU_NORMAL:
          currentMenu = MENU_BATTERY;
          Serial.println("Entered BATTERY menu");
          displayBatteryPanel();
          break;
        case MENU_BATTERY:
          currentMenu = MENU_SYSTEM;
          Serial.println("Entered SYSTEM menu");
          displaySystemInfoPanel();
          break;
        case MENU_SYSTEM:
          currentMenu = MENU_NORMAL;
          Serial.println("Returned to NORMAL mode");
          displayModeMessage("Normal Mode", "Auto-cycling panels");
          break;
      }
    }
  }
  menuButtonLastState = menuButtonState;
}

// Handle menu navigation and timeouts
void handleMenuNavigation() {
  unsigned long currentTime = millis();
  
  // Check for menu timeout (return to normal mode after inactivity)
  if (currentMenu != MENU_NORMAL && (currentTime - lastMenuActivity) > MENU_TIMEOUT) {
    Serial.println("Menu timeout - returning to normal mode");
    currentMenu = MENU_NORMAL;
    displayModeMessage("Timeout", "Returning to normal");
    lastDisplayChange = currentTime; // Reset display timer
  }
  
  // Update menu displays if in menu mode
  static unsigned long lastMenuUpdate = 0;
  if (currentMenu != MENU_NORMAL && (currentTime - lastMenuUpdate) > 1000) {
    switch (currentMenu) {
      case MENU_BATTERY:
        displayBatteryPanel();
        break;
      case MENU_SYSTEM:
        displaySystemInfoPanel();
        break;
      default:
        break;
    }
    lastMenuUpdate = currentTime;
  }
}

// Display battery information panel
void displayBatteryPanel() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Title
  display.setTextSize(1);
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.fillRect(1, 1, 126, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(40, 3);
  display.print("Battery");
  display.setTextColor(SSD1306_WHITE);
  
  // Battery voltage
  display.setTextSize(1);
  display.setCursor(5, 18);
  display.print("Voltage: ");
  display.print(batteryVoltage, 2);
  display.print("V");
  
  // Battery percentage
  display.setCursor(5, 30);
  display.print("Level: ");
  display.print(batteryPercentage);
  display.print("%");
  
  // Battery bar indicator
  display.drawRect(5, 42, 100, 10, SSD1306_WHITE);
  int barWidth = map(batteryPercentage, 0, 100, 0, 98);
  display.fillRect(6, 43, barWidth, 8, SSD1306_WHITE);
  
  // Battery status text
  display.setCursor(5, 55);
  if (batteryPercentage > 75) {
    display.print("Status: Full");
  } else if (batteryPercentage > 50) {
    display.print("Status: Good");
  } else if (batteryPercentage > 25) {
    display.print("Status: Low");
  } else {
    display.print("Status: Critical");
  }
  
  display.display();
}

// Display system information panel
void displaySystemInfoPanel() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Title
  display.setTextSize(1);
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.fillRect(1, 1, 126, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(45, 3);
  display.print("System");
  display.setTextColor(SSD1306_WHITE);
  
  // System information
  display.setCursor(2, 16);
  display.print("Uptime: ");
  unsigned long uptime = millis() / 1000;
  display.print(uptime / 3600);
  display.print("h ");
  display.print((uptime % 3600) / 60);
  display.print("m");
  
  display.setCursor(2, 26);
  display.print("Free Heap: ");
  display.print(ESP.getFreeHeap());
  
  display.setCursor(2, 36);
  display.print("CPU: ESP32-C3");
  
  display.setCursor(2, 46);
  display.print("Flash: ");
  display.print(ESP.getFlashChipSize() / 1024 / 1024);
  display.print("MB");
  
  display.setCursor(2, 56);
  display.print("Memes: ");
  display.print(TOTAL_MEMES);
  display.print(" loaded");
  
  display.display();
}

// Reset button handling with debouncing
void checkResetButton() {
  unsigned long currentTime = millis();
  
  // Read current button state
  bool resetButtonState = digitalRead(RESET_BUTTON_PIN);
  
  // Reset Button Logic
  if (resetButtonState != resetButtonLastState) {
    resetButtonLastChange = currentTime;
    resetButtonProcessed = false;
    Serial.print("Reset button state changed to: ");
    Serial.println(resetButtonState ? "HIGH" : "LOW");
  }
  
  if (!resetButtonProcessed && (currentTime - resetButtonLastChange) > BUTTON_DEBOUNCE_DELAY) {
    if (resetButtonState == LOW && resetButtonLastState == HIGH) {
      // Button press detected (falling edge after debounce)
      Serial.println("RESET BUTTON PRESSED!");
      resetButtonProcessed = true;
      
      // Immediately cycle to the next panel
      cyclePanels();
      Serial.println("Panel changed by reset button");
    }
  }
  resetButtonLastState = resetButtonState;
}

// Onboard button handling - currently just for testing/boot mode
void checkOnboardButton() {
  unsigned long currentTime = millis();
  
  // Read current button state
  bool onboardButtonState = digitalRead(ONBOARD_BUTTON_PIN);
  
  // Onboard Button Logic
  if (onboardButtonState != onboardButtonLastState) {
    onboardButtonLastChange = currentTime;
    onboardButtonProcessed = false;
    Serial.print("Onboard button state changed to: ");
    Serial.println(onboardButtonState ? "HIGH" : "LOW");
  }
  
  if (!onboardButtonProcessed && (currentTime - onboardButtonLastChange) > BUTTON_DEBOUNCE_DELAY) {
    if (onboardButtonState == LOW && onboardButtonLastState == HIGH) {
      // Button press detected (falling edge after debounce)
      Serial.println("ONBOARD BUTTON PRESSED!");
      onboardButtonProcessed = true;
      
      // For testing: Immediately show system info panel
      displaySystemInfoPanel();
      Serial.println("Displayed system information panel");
    }
  }
  onboardButtonLastState = onboardButtonState;
}

// LED control - flash LED briefly
void flashLED() {
  digitalWrite(ONBOARD_LED_PIN, HIGH);
  delay(LED_FLASH_DURATION);
  digitalWrite(ONBOARD_LED_PIN, LOW);
}

// LED control - update LED state based on conditions
void updateLED() {
  // Example: Flash LED quickly if battery is low
  if (batteryPercentage < 20) {
    // Flash LED rapidly (5 times per second)
    const int flashInterval = 200; // 200ms on/off
    static unsigned long lastFlashTime = 0;
    static bool ledState = false;
    
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= flashInterval) {
      ledState = !ledState;
      digitalWrite(ONBOARD_LED_PIN, ledState ? HIGH : LOW);
      lastFlashTime = currentTime;
    }
  } else {
    // Ensure LED is off if battery is not low
    digitalWrite(ONBOARD_LED_PIN, LOW);
  }
}