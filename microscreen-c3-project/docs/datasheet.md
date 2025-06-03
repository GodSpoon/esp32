Product Introduction:
The ESP32 C3 SuperMini is an loT mini development boardbased on the EspressifESP32-C3WiFi/
Bluetoot hdual-mode chip. The ESP32-C3 is a 32-bit RISC-V CPU that contains the FPU(floating
point unit)for32-bit single-precision operationswith powerfulcomputing power. It hasexcellent F
performance andsupportsIEEE 802.11b /g/n WiFiand Bluetooth 5 (LE)protocols. Theboard comes
with an external antenna toenhance signal strength for wireless applications. It also hasasmall and
delicate formfactorcombined with asingle-sided surface mountdesign. It is equippedwitha
wealth ofinterfaces,with 11 digital I/ OS that can be used as PWM pins and 4 analog I/ OSthat can
be usedas ADC pins. It supports four serial interfaces: UART, I2C and SPI.The oard alsohas a
small reset button and a boot loader modebutton.

Combinedwith theabove features, theESP32C3SuperMini is positioned asahigh-performance,low-
power,cost-effective iot minidevelopment board forlow-power iotapplications and wireless
wearable applications.

Productparameter:
1.Powerful CPU:ESP32-C3, 32-bit RISC-V single-core processor,runningup to 160MHz
.WiFi: 80 2.11b/g/n protocol, 2.4GhHz, support Stationmode,SoftAP mode,SoftAP+Stationmode,
hybridmode

3. Bluetooth: Bluetooth5.
4.Ultra-low powerconsumption: deepsleep power consumption ofabout43uA
5.Rich boardresources:400KB SRAM, 384KBROM built-in 4Mflash.
6.Chipmodel:ESP32C3FN
7.Ultra-smallsize: As small as thethumb (22.52x18mm) classic shape,suitable forwearablesand
smallprojects
8.Reliablesecurity features:Encryptionhardwareaccelerators thatsupportAES- 12 8/256,hashing,
RSA,HMAC,digitalsignatures,and securestartup
9.Richinterface:1x12C,1xSPI,2xUART,11xGPIO(PWM),4xADC
0.Single-sidedcomponents,surface mountdesign
11 Onboard^ LED bluelight: GPIO8 pin



## Hardware setup

```
You need to prepare the following:
1x ESP32 C3 SuperMini
1 x Computer
1xUSB Type-C data cable
Some USBcables can only supplypower,nottransmitdata.
Make sure yourUSB cablecan transferdata.
```
External power supply:
If external power supply is required, just connect the + level of the external power supply to the
position of 5V.GND connects to the negative terminal. (Support 3.3 - 6V power supply). Remember
thatwhen connecting the external power supply, you cannot access USB. USB and external power
supplycan only choose one.

###### When welding, please be careful not to short-circuit the positive

###### and negativeelectrodes,otherwise itwill burn the battery and equipment.

## WIFI antenna


## Software setup

```
Step 1. Download and install the latestversion of IDEbased on your operating system.
```
Step 2. Start the IDE application
Step 3. Add the ESP32 board package to the IDE
Navigateto File -› Preferences, then fill inthe "Additional BoardsManager URL" using the
followingURL:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

```
Navigate to Tools > Board > Boards Manager... Enter the keyword "esp32" in the search box, select
the latest version of esp32 and install it.
```

```
Navigate to "Tools" > Port, then select the serial port name of the ESP32 C3 SuperMini you are
connecting to. This could be COM3 or later (COM1 and COM2 are usually reserved for
hardwareserial ports)
```
#### Flashing LED

```
Copy the Following code in the the IDE
// define led according to pin diagramint
led = 8;
void setup() {
// initialize digital pin led as an output
pinMode(led,OUTPUT);
}
void loop() {
digitalWrite(led,HIGH); // turntheLED on
delay(1000); // wait for asecond
digitalWrite(led, LOW); // turn theLED off
delay(1000); // wait for a second
}
After uploading, you will see the LED flashing on the board with a 1-second delay between each
flashing.
```
Navigate to Tools > Development board > ESP32 and select ESP32C3 Dev Module. The list of
boards is a bit long and you need to scroll to the bottom to get to it.


# FAQ

```
Com port cannot be recognized on IDE
Enter the download mode:Method 1: Press andhold BOOTto power on.Method
2: Press and holddown the BOOTbutton oftheESP32C3,
press the RESETbutton,release the RESET button, andthen release theBOOTbutton. Then the ESP32C3 will
enterdownloadmode. (Each connectionneedsto re-enter thedownload mode, sometimes press once, theport
instability willbe disconnected, you can judge bythe port identification sound)
The programwill notrun afterupload
fter the upload succeeds, youneed to press the Reset button toexecute theupload.
ESP32C3 SuperMini serial port cannotprint
Set the USB CDC OnBoot on the toolbar toEnabled
```
## WiFi function

Connect the ESP32C3SuperMini to your computer using a USB Type-C datacable
Scan WiFinetworks (StationMode)
We will use the ESP32C3SueprMini to scan the availableWiFinetworksaroundit.Here, the boardwill beconfigured ins
tation (STA) mode

1. Copy and pastethefollowingcode into the IDE
#include "WiFi.h"
void setup()
{
Serial.begin(115200);
// Set WiFi to station mode and disconnect from an AP if it was previouslyconnected
WiFi.mode(WIFI_STA);
WiFi.disconnect();
delay(100);
Serial.println("Setup done");
}
void loop()
{
Serial.println("scan start");
//WiFi.scanNetworks will return thenumberof networks found
int n=WiFi.scanNetworks();
Serial.println("scandone");
if (n ==0){
Serial.println("no networks found");
}else{
Serial.print(n);
Serial.println("networks found");
for (int i= 0; i< n; ++i) {
//PrintSSIDandRSSI for each networkfound
Serial.print(i+1);
Serial.print(":");
Serial.print(WiFi.SSID(i));
Serial.print("(");
Serial.print(WiFi.RSSI(i));Serial.print(")");
Serial.println((WiFi.encryptionType(i) ==WIFI_AUTH_OPEN)?"":"*");
delay(10);}
}
Serial.println("");
// Waita bitbeforescanningagain
delay(5000);}


2. Upload the code and turn on the serial monitor to start scanning the WiFinetwork

#### Connect to WiFi network

1. Copy and paste the following code into the IDE
    #include <WiFi.h>
    const char* ssid = "your-ssid"; //your WiFi Name
    const char* password = "your-password"; //your WiFi
    passwordvoidsetup()
    {
    Serial.begin(115200);
    delay(10);
    // We start by connecting to aWiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while(WiFi.status() !=WL_CONNECTED){
    delay(500);
    Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFiconnected");
    Serial.println("IPaddress:");
    Serial.println(WiFi.localIP());
    }
    voidloop()
    {
    }
2. Upload the code and turn on the serial monitor to check whether the development board
isconnected to the WiF| network


#### WiFi hotspot

```
In this example, we will use the ESP32C3SuperMini as a WiFi access point that otherdevices
canconnect to. This is similar to the WiFi hotspot function on your phone.
```
1. Copy andpaste thefollowing code into the IDE
    #include "WiFi.h"
    void setup()
    {
    Serial.begin(115200);
    WiFi.softAP("ESP_AP", "123456789");
    }
    void loop()
    {
    Serial.print("HostName:");
    Serial.println(WiFi.softAPgetHostname());
    Serial.print("Host IP:");
    Serial.println(WiFi.softAPIP());
    Serial.print("HostIPV6:");
    Serial.println(WiFi.softAPIPv6());
    Serial.print("HostSSID:");
    Serial.println(WiFi.SSID());
    Serial.print("Host BroadcastIP:");
    Serial.println(WiFi.softAPBroadcastIP());
    Serial.print("Host macAddress:");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("Number ofHost
       Connections:");
    Serial.println(WiFi.softAPgetStationNum());
    Serial.print("Host NetworkID:");
    Serial.println(WiFi.softAPNetworkID());
    Serial.print("HostStatus:");
    Serial.println(WiFi.status());delay(1000);
    }
2. Upload the code and turn on the serial monitor to check for more details aboutthe WiFi access
point


```
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
int scanTime = 5;//In seconds
BLEScan*pBLEScan;
classMyAdvertisedDeviceCallbacks:publicBLEAdvertisedDeviceCallbacks {voidonResult(BLEAdvertisedDevice
advertisedDevice) {Serial.printf("AdvertisedDevice:%s \n",advertisedDevice.toString().c_str());
}
};
void setup(){
Serial.begin(115200);
Serial.println("Scanning...");
BLEDevice::init("");
pBLEScan = BLEDevice::getScan();//create newscan
pBLEScan->setAdvertisedDeviceCallbacks(newMyAdvertisedDeviceCallbacks());
pBLEScan->setActiveScan(true);//activescan uses morepower, butget resultsfaster
pBLEScan->setInterval(100);
pBLEScan->setWindow(99); // less or equal setInterval value
}
voidloop() {
// putyour maincodehere, to runrepeatedly:
BLEScanResults foundDevices =pBLEScan->start(scanTime,false);
Serial.print("Devices found:");
Serial.println(foundDevices.getCount());
Serial.println("Scandone!");
pBLEScan->clearResults(); // deleteresultsfromBLEScanbuffer to release memory
delay(2000);
}
```
2. Upload the code and turn on the serial monitor to start scanning Bluetooth devices

#### Bluetooth function

Connect the ESP32C3SuperMini to your computer via a USB Type-Ccable

##### Scan Bluetooth

Wewill use the ESP32C3SueprMini to scan for available Bluetooth devicesaround it

1. Copy andpaste the following code into the IDE


### As a Bluetooth server

In this example, we will use the ESP32C3SuperMini as the Bluetooth server.Here we will use asmartp
hone to search the ESP32C3SuperMini board and send a string todisplay onthe serialmonitor

1. Copy and paste the following code into the IDE

```
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
// See the following for generating UUIDs:
/ https://www.uuidgenerator.net/
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
class MyCallbacks: public BLECharacteristicCallbacks {
void onWrite(BLECharacteristic *pCharacteristic) {
std::string value = pCharacteristic->getValue();
if (value.length() > 0) {
Serial.println("*********");
Serial.print("New value: ");
for (int i = 0; i < value.length(); i++)
Serial.print(value[i]);
Serial.println();
Serial.println("*********");
}
}
};
void setup() {
Serial.begin(115200);
BLEDevice::init("MyESP32");
BLEServer *pServer = BLEDevice::createServer();
BLEService *pService = pServer->createService(SERVICE_UUID);
BLECharacteristic *pCharacteristic = pService->createCharacteristic(
CHARACTERISTIC_UUID,
BLECharacteristic::PROPERTY_READ |
BLECharacteristic::PROPERTY_WRITE
);
pCharacteristic->setCallbacks(new MyCallbacks());
pCharacteristic->setValue("Hello World");
pService->start();
BLEAdvertising *pAdvertising = pServer->getAdvertising();
pAdvertising->start();
}
void loop() {
// put your main code here, to run repeatedly:
delay(20 00 );
}
```

2. Upload the code and open the serial monitor
3. Download and install the LightBlue app on yoursmartphone
4. Turn on the Bluetooth of the phone,place the phone near the ESP32C3SuperMini, scanthe device
and connect the MyESP32 device
    5. Open the LightBlue application and click the Bonded TAB
    6. Click CONNECT next toMyESP
7. Click at the bottom where Readable, Writable are displayed
8. Under the Data Format drop-down menu, select UTF-8 string


9. Type "Hello" under "WRITTEN VALUES" and click "WRITE"

```
:) You will see the text string"Hello" output on the serial monitor of the IDE
```

## ChatGPT

```
We can use the ESP32 Supermini to do some applications in ChatGPT.
For example, we canconfigureour own ChatGPT Q&A page using the SuperMini.
In this page, youcan enter your question, theESP32C3SuperMini will record your
question, and usingthe API callmethod provided byOpenAl,using HTTP Client to
send a requestcommand, get theanswer ofChatGPT and print it in theserialport.
```
```
The steps are asfollows:
ConnecttheESP32C3SuperMini to the network
Buildingembedded webpages
Submitquestions viathe built-in web page
Get answers fromChatGPT
If you areinterested, youcansearch forrelatedmaterials.
```
# Pin use

The ESP32C3SuperMini has various interfaces. There are 11 digital I/ OS that can be used asPWM

pins and 4 analog inputs that can be used as ADC pins. It supports four serial communicationinterface

s such as UART, I2C, SPI and 12S. This article will help you understand these interfaces andimpleme

nt them in your next project!

About pin A0A5, GPIOOGPIO10 (010), and the beginning of D, here toexplain, the default

motherboard only GPIO beginning is 010, 20, 21, A0~A5 pin is amapping problem, in order to

facilitate the user to tell the function of this pin is analog pin or digitalpin. When theArduino selects

the development board type and selects the ESP32C3 Dev Module, youcan referenceitspin map. The

```
pin map is shown below:
```

```
Upload the code to the board, and the on-board LED will light up every second.
// define led according to pin diagram
int led = 8;void setup() {
//initialize digital pin led as an output
pinMode(led,OUTPUT);
}
voidloop(){digitalWrite(led, HIGH); // turn theLED on
delay(1000); //wait for asecond
digitalWrite(led, LOW);//turn theLEDoff
delay(1000); // wait for asecond
}
```
##### Digital PWM

```
Upload the following code to see the on-board LED gradually dim.
int ledPin = 8; // LED connected to digital pin 10
void setup() {
// declaring LED pin as output
pinMode(ledPin,OUTPUT);
}
void loop() {
// fade in from min to max inincrements of 5points:
for (int fadeValue = 0 ;fadeValue <= 255;fadeValue += 5){
// sets thevalue (range from 0 to255):
analogWrite(ledPin, fadeValue);
// wait for 30milliseconds to see thedimmingeffect
delay(30);
}
//fade out from max tomin in increments of 5points:
for (intfadeValue =255 ; fadeValue >= 0;fadeValue-= 5){
//sets thevalue (range from 0 to255):
analogWrite(ledPin,fadeValue);
//wait for 30milliseconds to see thedimmingeffect
delay(30);
}
}
```
##### Analog pin

Connect the potentiometer to pin A5 and upload the following code to control the flashing
interval of the LED by turning the potentiometer knob.

##### Digital pin


## Serial port

Hardware serial port, there are two hardware serial ports on the board:
USB serial port
ART serial port
By default, USB serial is enabled, which means you can connect the development board to a PC via
USB Type-C and turn on the serial monitor on the Arduino IDE to see the data sent via serial.

```
However, if you want to use ART as a serial port, you will need to connect pin 20 as a TX pin
and pin 21 as an RX pin using a USB serial adapter.
```
```
Also, you need to set USB CDC On Boot to disabled from the Arduino IDE.
```
```
const int sensorPin = A5;
const int ledPin = 8;
void setup() {
pinMode(sensorPin, INPUT); // declare the sensorPin as anINPUT
pinMode(ledPin,OUTPUT); // declare theledPin as anOUTPUT
}
void loop() {
// read the value from the sensor:
intsensorValue =analogRead(sensorPin);
// turn theledPinon
digitalWrite(ledPin, HIGH);
//stop the program for<sensorValue>milliseconds:
delay(sensorValue);
// turnthe ledPinoff:
digitalWrite(ledPin,LOW);
// stopthe program for for<sensorValue>milliseconds:
delay(sensorValue);
}
```

##### Software serial port

```
If you want to use more serial ports, you need to use the SoftwareSeriallibrary tocreate soft
serial ports
```
##### I2C

```
Connection of ESP32 C3 Supermini and 0.96 OLED
```
1. Open Arduino IDE and navigate to Sketch -› Include Library-›Manage Libraries...
2. Search for u8g2 and install it
3. Upload the code to display the text string on the OLED display


