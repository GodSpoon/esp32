#ifndef USB_HOST_H
#define USB_HOST_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class USBHostManager {
private:
    bool usbMounted = false;
    String mountPoint = "/usb";
    
public:
    bool initialize();
    bool isMounted();
    String listFiles(String path = "/");
    bool uploadFile(String filename, uint8_t* data, size_t length);
    bool downloadFile(String filename, uint8_t** data, size_t* length);
    bool deleteFile(String filename);
    bool createDirectory(String dirname);
    uint64_t getTotalSpace();
    uint64_t getFreeSpace();
    String getDeviceInfo();
    
    // Event callbacks
    void onUSBConnect();
    void onUSBDisconnect();
    void onFileTransferProgress(int percentage);
};

#endif // USB_HOST_H
