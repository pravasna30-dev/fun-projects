/*
 * Lesson 10: Bluetooth LE Basics
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Creates a BLE (Bluetooth Low Energy) server that exposes:
 *   1. LED State characteristic  (READ)   — reports whether the LED is on/off
 *   2. LED Control characteristic (WRITE)  — accepts 0 or 1 to toggle the LED
 *   3. Counter characteristic (READ+NOTIFY) — sends an incrementing counter every second
 *
 * No extra hardware needed — uses the built-in LED on GPIO 21 and the
 * ESP32-S3's built-in BLE radio.
 *
 * To interact, use a BLE scanner app on your phone:
 *   - nRF Connect (Nordic Semiconductor) — Android & iOS
 *   - LightBlue (Punch Through) — iOS & Android
 *
 * Usage:
 *   1. Upload this sketch.
 *   2. Open your BLE scanner app and scan for "XIAO-ESP32S3".
 *   3. Connect to the device.
 *   4. Find the service (UUID: 4fafc201-...) and explore the characteristics.
 *   5. Write 0x01 to LED Control to turn on the LED, 0x00 to turn off.
 *   6. Subscribe to Counter notifications to see the counter increment.
 */

// ---------------------------------------------------------------------------
// Library Includes
// ---------------------------------------------------------------------------
// The ESP32 Arduino core includes a full BLE stack. These headers
// provide the classes we need to create a GATT server.
#include <BLEDevice.h>        // Top-level BLE initialization
#include <BLEServer.h>        // GATT server functionality
#include <BLEUtils.h>         // Utility classes
#include <BLE2902.h>          // CCCD descriptor (enables notifications)

// ---------------------------------------------------------------------------
// Pin Definitions
// ---------------------------------------------------------------------------
// The XIAO ESP32-S3 has a built-in LED on GPIO 21.
// Note: This pin is shared with the SD card chip select.
// If you have an SD card inserted, the LED won't work.
#define LED_PIN 21

// ---------------------------------------------------------------------------
// BLE UUIDs
// ---------------------------------------------------------------------------
// UUIDs uniquely identify services and characteristics. We use custom
// 128-bit UUIDs since our LED service isn't a standard Bluetooth service.
//
// You can generate your own UUIDs at: https://www.uuidgenerator.net/
// The specific values don't matter — they just need to be unique.

// Service UUID — groups all our characteristics under one "service"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// Characteristic UUIDs — each identifies one data point
#define LED_STATE_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"   // READ
#define LED_CONTROL_UUID    "cba1d466-344c-4be3-ab3f-189f80dd7518"   // WRITE
#define COUNTER_UUID        "1a2b3c4d-5e6f-7a8b-9c0d-1e2f3a4b5c6d"  // READ + NOTIFY

// ---------------------------------------------------------------------------
// Global Variables
// ---------------------------------------------------------------------------
BLEServer         *pServer       = NULL;    // The GATT server instance
BLECharacteristic *pLedState     = NULL;    // LED state (read-only)
BLECharacteristic *pLedControl   = NULL;    // LED control (write)
BLECharacteristic *pCounter      = NULL;    // Counter (read + notify)

bool deviceConnected    = false;  // Is a client currently connected?
bool oldDeviceConnected = false;  // Previous connection state (for edge detection)
uint32_t counterValue   = 0;     // The notification counter
bool ledState           = false;  // Current LED state (false = off, true = on)

unsigned long lastCounterUpdate = 0;  // Timestamp of last counter notification

// ---------------------------------------------------------------------------
// BLE Server Callbacks
// ---------------------------------------------------------------------------
// These callbacks fire when a client connects to or disconnects from
// the BLE server. We use them to track connection state and manage
// advertising (re-start advertising after a disconnect so new clients
// can find the device).
class MyServerCallbacks : public BLEServerCallbacks {
  // Called when a client successfully connects
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client connected!");
  }

  // Called when a client disconnects
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected.");
  }
};

// ---------------------------------------------------------------------------
// LED Control Characteristic Callback
// ---------------------------------------------------------------------------
// This callback fires whenever a BLE client writes a value to the
// LED Control characteristic. We read the written value and use it
// to turn the LED on or off.
class LedControlCallback : public BLECharacteristicCallbacks {
  // Called when a client writes to this characteristic
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Get the written value as a string (raw bytes)
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
      // Read the first byte as the LED command
      uint8_t command = value[0];

      if (command == 1 || command == '1') {
        // Turn LED ON
        // Note: The XIAO ESP32-S3 built-in LED is active LOW on some
        // boards and active HIGH on others. GPIO 21 is typically active HIGH.
        ledState = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("BLE -> LED ON");
      } else {
        // Turn LED OFF
        ledState = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("BLE -> LED OFF");
      }

      // Update the LED State characteristic so clients can read it
      uint8_t stateValue = ledState ? 1 : 0;
      pLedState->setValue(&stateValue, 1);
    }
  }
};

// ---------------------------------------------------------------------------
// Function: setupBLE
// Initializes the entire BLE stack: device, server, service, and
// characteristics. This is called once from setup().
// ---------------------------------------------------------------------------
void setupBLE() {
  // Step 1: Initialize the BLE device with a name.
  // This name appears in scan results on the phone.
  // Keep it short — the advertisement packet has limited space (31 bytes).
  BLEDevice::init("XIAO-ESP32S3");

  // Step 2: Create a GATT server.
  // A "server" in BLE terminology is the device that holds data
  // (characteristics) that clients can read/write.
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Step 3: Create a service.
  // A service groups related characteristics together. Think of it
  // like a class — the service is the class, characteristics are its fields.
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // -----------------------------------------------------------------------
  // Step 4a: Create the LED State characteristic (READ-only)
  // -----------------------------------------------------------------------
  // This characteristic reports the current LED state.
  // Clients can read it at any time to check if the LED is on or off.
  pLedState = pService->createCharacteristic(
    LED_STATE_UUID,
    BLECharacteristic::PROPERTY_READ   // Client can read this value
  );
  // Set the initial value to 0 (LED off)
  uint8_t initialState = 0;
  pLedState->setValue(&initialState, 1);

  // -----------------------------------------------------------------------
  // Step 4b: Create the LED Control characteristic (WRITE-only)
  // -----------------------------------------------------------------------
  // Clients write to this characteristic to turn the LED on/off.
  // The LedControlCallback handles the write event.
  pLedControl = pService->createCharacteristic(
    LED_CONTROL_UUID,
    BLECharacteristic::PROPERTY_WRITE  // Client can write to this value
  );
  pLedControl->setCallbacks(new LedControlCallback());

  // -----------------------------------------------------------------------
  // Step 4c: Create the Counter characteristic (READ + NOTIFY)
  // -----------------------------------------------------------------------
  // This characteristic demonstrates notifications — the server pushes
  // updated values to the client without being asked.
  //
  // NOTIFY means:
  //   - The server can send new values at any time
  //   - The client must "subscribe" by writing to the CCCD descriptor
  //   - No acknowledgement from the client (unlike INDICATE)
  pCounter = pService->createCharacteristic(
    COUNTER_UUID,
    BLECharacteristic::PROPERTY_READ |     // Client can read current value
    BLECharacteristic::PROPERTY_NOTIFY     // Server can push updates
  );

  // Add the Client Characteristic Configuration Descriptor (CCCD).
  // This is a special descriptor (UUID 0x2902) that the client writes to
  // in order to enable or disable notifications. Without this descriptor,
  // the client has no way to subscribe.
  pCounter->addDescriptor(new BLE2902());

  // Set initial counter value
  uint8_t initialCounter = 0;
  pCounter->setValue(&initialCounter, 1);

  // Step 5: Start the service.
  // The service must be started before advertising, otherwise clients
  // won't see any characteristics.
  pService->start();

  // Step 6: Configure and start advertising.
  // Advertising is how BLE devices announce their presence. The ESP32
  // broadcasts a small packet containing the device name and service UUID.
  // Scanner apps on phones pick up these packets.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // Include service UUID in advertisement
  pAdvertising->setScanResponse(true);         // Allow scan response for more data
  // These settings help iOS devices find the ESP32 more reliably:
  pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE server started. Advertising as \"XIAO-ESP32S3\"");
  Serial.println("Waiting for connections...");
}

// ---------------------------------------------------------------------------
// setup() - Runs once at startup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==========================================");
  Serial.println("  Lesson 10: Bluetooth LE Basics");
  Serial.println("  XIAO ESP32-S3 Sense");
  Serial.println("==========================================");
  Serial.println();

  // Configure the built-in LED as an output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off

  // Initialize the BLE stack and start advertising
  setupBLE();

  Serial.println();
  Serial.println("BLE Characteristics:");
  Serial.println("  LED State   (READ):         " LED_STATE_UUID);
  Serial.println("  LED Control (WRITE):        " LED_CONTROL_UUID);
  Serial.println("  Counter     (READ+NOTIFY):  " COUNTER_UUID);
  Serial.println();
}

// ---------------------------------------------------------------------------
// loop() - Runs repeatedly after setup()
// ---------------------------------------------------------------------------
void loop() {
  // --- Handle notifications when a client is connected ---
  if (deviceConnected) {
    // Send counter notification every 1 second
    if (millis() - lastCounterUpdate >= 1000) {
      lastCounterUpdate = millis();
      counterValue++;

      // Set the characteristic value.
      // We send it as a 4-byte uint32 so the counter can go above 255.
      pCounter->setValue(counterValue);

      // Notify all subscribed clients of the new value.
      // This triggers the GATT notification mechanism — the BLE stack
      // sends a small packet to each subscribed client containing the
      // new value. No acknowledgement is expected.
      pCounter->notify();

      // Print to serial for debugging
      Serial.printf("Counter: %u (notified)\n", counterValue);
    }
  }

  // --- Handle reconnection ---
  // When a client disconnects, we need to restart advertising
  // so that new clients can discover and connect to us.
  if (!deviceConnected && oldDeviceConnected) {
    // Give the BLE stack a moment to clean up
    delay(500);

    // Restart advertising
    pServer->startAdvertising();
    Serial.println("Advertising restarted. Waiting for connections...");

    oldDeviceConnected = deviceConnected;
  }

  // --- Detect new connection ---
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    // Reset counter on new connection for clarity
    counterValue = 0;
    Serial.println("Counter reset for new connection.");
  }

  // Small delay to prevent busy-looping
  delay(10);
}
