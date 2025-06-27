#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>

// Global variables
BLEHIDDevice* hid;
BLECharacteristic* input;
BLEServer* server;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Server callbacks to handle connection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

// Function to send key press and release
void sendKey(uint8_t modifier, uint8_t keycode) {
  if (!deviceConnected) return;
  
  // Send key press
  uint8_t keyReport[8] = {0};
  keyReport[0] = modifier;  // Modifier byte
  keyReport[2] = keycode;   // First key
  
  input->setValue(keyReport, sizeof(keyReport));
  input->notify();
  delay(10);
  
  // Send key release
  memset(keyReport, 0, sizeof(keyReport));
  input->setValue(keyReport, sizeof(keyReport));
  input->notify();
}

// Function to send multiple keys
void sendKeys(uint8_t modifier, uint8_t key1, uint8_t key2 = 0, uint8_t key3 = 0) {
  if (!deviceConnected) return;
  
  uint8_t keyReport[8] = {0};
  keyReport[0] = modifier;
  keyReport[2] = key1;
  keyReport[3] = key2;
  keyReport[4] = key3;
  
  input->setValue(keyReport, sizeof(keyReport));
  input->notify();
  delay(10);
  
  // Release keys
  memset(keyReport, 0, sizeof(keyReport));
  input->setValue(keyReport, sizeof(keyReport));
  input->notify();
}

// Function to send string
void sendString(const char* str) {
  if (!deviceConnected) return;
  
  for (int i = 0; str[i] != '\0'; i++) {
    uint8_t keycode = 0;
    uint8_t modifier = 0;
    
    char c = str[i];
    
    // Convert character to HID keycode
    if (c >= 'a' && c <= 'z') {
      keycode = c - 'a' + 0x04;  // a=0x04, b=0x05, etc.
    }
    else if (c >= 'A' && c <= 'Z') {
      keycode = c - 'A' + 0x04;  // A=0x04, B=0x05, etc.
      modifier = 0x02;           // Left Shift
    }
    else if (c >= '1' && c <= '9') {
      keycode = c - '1' + 0x1E;  // 1=0x1E, 2=0x1F, etc.
    }
    else if (c == '0') {
      keycode = 0x27;            // 0=0x27
    }
    else if (c == ' ') {
      keycode = 0x2C;            // Space=0x2C
    }
    else if (c == '\n') {
      keycode = 0x28;            // Enter=0x28
    }
    else if (c == '\t') {
      keycode = 0x2B;            // Tab=0x2B
    }
    else if (c == '.') {
      keycode = 0x37;            // Period=0x37
    }
    else if (c == ',') {
      keycode = 0x36;            // Comma=0x36
    }
    else if (c == '!') {
      keycode = 0x1E;            // 1 key
      modifier = 0x02;           // Left Shift
    }
    else if (c == '@') {
      keycode = 0x1F;            // 2 key
      modifier = 0x02;           // Left Shift
    }
    else if (c == '#') {
      keycode = 0x20;            // 3 key
      modifier = 0x02;           // Left Shift
    }
    else if (c == '$') {
      keycode = 0x21;            // 4 key
      modifier = 0x02;           // Left Shift
    }
    else if (c == '%') {
      keycode = 0x22;            // 5 key
      modifier = 0x02;           // Left Shift
    }
    else {
      continue;  // Skip unsupported characters
    }
    
    sendKey(modifier, keycode);
    delay(50);  // Small delay between characters
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE HID Keyboard...");
  
  // Initialize BLE
  BLEDevice::init("ESP32 HID Keyboard");
  Serial.println("BLE initialized successfully");
  
  // Create BLE server
  server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());
  
  // Create HID device
  hid = new BLEHIDDevice(server);
  input = hid->inputReport(1); // Report ID 1
  
  // Set device information
  hid->manufacturer()->setValue("Custom Electronics");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x01);
  
  // HID Report Map for standard keyboard
  const uint8_t reportMap[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224)
    0x29, 0xE7,       //   Usage Maximum (231)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Var, Abs) ; Modifier byte
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Const) ; Reserved byte
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data, Ary, Abs)
    0xC0              // End Collection
  };
  
  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->startServices();
  
  // Setup advertising
  BLEAdvertising* advertising = server->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->start();
  
  hid->setBatteryLevel(100);
  
  Serial.println("ESP32 HID Keyboard started!");
  Serial.println("Waiting for connection...");
}

void loop() {
  // Handle connection state changes
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    Serial.println("Device connected - starting demo");
    delay(1000);
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give stack time to get ready
    server->startAdvertising(); // Restart advertising
    Serial.println("Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }
  
  // Demo sequence when connected
  if (deviceConnected) {
    static unsigned long lastAction = 0;
    static int demoStep = 0;
    
    if (millis() - lastAction > 5000) {  // Every 5 seconds
      lastAction = millis();
      
      switch (demoStep % 6) {
        case 0:
          Serial.println("Sending: Hello World!");
          sendString("Hello World!");
          break;
          
        case 1:
          Serial.println("Sending: ESP32 BLE Keyboard");
          sendString("ESP32 BLE Keyboard");
          break;
          
        case 2:
          Serial.println("Sending: 12345");
          sendString("12345");
          break;
          
        case 3:
          Serial.println("Sending: Ctrl+A (Select All)");
          sendKey(0x01, 0x04);  // Ctrl + A
          break;
          
        case 4:
          Serial.println("Sending: Tab key");
          sendKey(0x00, 0x2B);  // Tab
          break;
          
        case 5:
          Serial.println("Sending: Enter key");
          sendKey(0x00, 0x28);  // Enter
          break;
      }
      
      demoStep++;
    }
  }
  
  delay(100);
}

// Common HID Keycodes Reference:
// 0x04-0x1D: a-z
// 0x1E-0x27: 1-9, 0
// 0x28: Enter
// 0x29: Escape
// 0x2A: Backspace
// 0x2B: Tab
// 0x2C: Space
// 0x2D: - and _
// 0x2E: = and +
// 0x2F: [ and {
// 0x30: ] and }
// 0x31: \ and |
// 0x33: ; and :
// 0x34: ' and "
// 0x35: ` and ~
// 0x36: , and <
// 0x37: . and >
// 0x38: / and ?
// 0x39: Caps Lock
// 0x3A-0x45: F1-F12
// 0x4F-0x52: Right, Left, Down, Up arrows

// Modifier Keys:
// 0x01: Left Ctrl
// 0x02: Left Shift  
// 0x04: Left Alt
// 0x08: Left GUI (Windows key)
// 0x10: Right Ctrl
// 0x20: Right Shift
// 0x40: Right Alt
// 0x80: Right GUI
