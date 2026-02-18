# Lesson 10: Bluetooth LE Basics

**Difficulty:** Intermediate

**Estimated Time:** 75-90 minutes

---

## What You'll Learn

- The difference between Bluetooth Classic and Bluetooth Low Energy (BLE)
- What GATT (Generic Attribute Profile) is and how it structures data
- How BLE services and characteristics work
- What UUIDs are and how they identify BLE components
- How to create a BLE server on the ESP32-S3
- How to expose readable, writable, and notifying characteristics
- How to connect from a smartphone app to control the board wirelessly

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Main microcontroller |
| USB-C cable | 1 | For programming and power |
| Smartphone | 1 | iOS or Android, with BLE support |
| BLE Scanner App | 1 | **nRF Connect** (Nordic) or **LightBlue** (Punch Through) — both free |

> **No extra hardware needed!** This lesson uses only the built-in LED on GPIO 21 and the ESP32-S3's built-in BLE radio.

## Circuit Diagram

```
   No external wiring required!

   ┌─────────────────────────────────────┐
   │   XIAO ESP32-S3 Sense              │
   │                                     │
   │   Built-in LED ────── GPIO 21       │
   │                                     │
   │   Built-in BLE Radio                │
   │   ┌───────────────┐                 │
   │   │  2.4 GHz      │                 │
   │   │  Antenna       │                 │
   │   │  (on PCB)      │                 │
   │   └───────────────┘                 │
   │                                     │
   └─────────────────────────────────────┘

              ))) BLE Signal (((

   ┌─────────────────────────────────────┐
   │   Smartphone                        │
   │   ┌───────────────────────────┐     │
   │   │  nRF Connect / LightBlue │     │
   │   │  ┌─────────────────────┐ │     │
   │   │  │ LED Service         │ │     │
   │   │  │  - LED State (read) │ │     │
   │   │  │  - LED Ctrl (write) │ │     │
   │   │  │  - Counter (notify) │ │     │
   │   │  └─────────────────────┘ │     │
   │   └───────────────────────────┘     │
   └─────────────────────────────────────┘
```

## Step-by-Step Instructions

### Step 1: Install the BLE Scanner App

Before writing any code, install a BLE scanner on your phone:

**Android:** Install [nRF Connect](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) from the Play Store.

**iOS:** Install [nRF Connect](https://apps.apple.com/app/nrf-connect-for-mobile/id1054362403) or [LightBlue](https://apps.apple.com/app/lightblue/id557428110) from the App Store.

These apps let you scan for nearby BLE devices, connect to them, read/write characteristics, and subscribe to notifications — essential tools for BLE development.

### Step 2: Understand the BLE Architecture (Read First!)

Before uploading code, take a few minutes to understand the BLE concepts below. BLE has more conceptual overhead than previous lessons, but it's not complicated once the vocabulary clicks.

### Step 3: No Library Installation Needed

The ESP32 Arduino core includes the BLE library (`BLEDevice.h`, `BLEServer.h`, etc.) out of the box. No additional libraries to install.

### Step 4: Upload the Sketch

1. Open `ble-basics.ino` in Arduino IDE.
2. Select **Tools > Board > XIAO_ESP32S3**.
3. Select **Tools > Port > /dev/cu.usbmodem101**.
4. Upload (this may take a bit longer — the BLE library is large).
5. Open Serial Monitor at **115200 baud**.

### Step 5: Connect from Your Phone

1. Open nRF Connect (or LightBlue) on your phone.
2. Tap **Scan** to find nearby BLE devices.
3. Look for a device named **"XIAO-ESP32S3"** and tap **Connect**.
4. You should see a service with UUID starting with `4fafc201...`.
5. Expand it to see three characteristics.

### Step 6: Interact with Characteristics

**Read the LED state:**
- Find the "LED State" characteristic (UUID ending in `beb5...`).
- Tap the read button (down arrow). You should see `0` (LED off).

**Toggle the LED:**
- Find the "LED Control" characteristic (UUID ending in `cba1...`).
- Tap the write button (up arrow).
- Write the value `1` (as a byte) to turn the LED ON.
- Write `0` to turn it OFF.
- Read the LED State characteristic again to confirm it changed.

**Subscribe to the counter:**
- Find the "Counter" characteristic (UUID ending in `1a2b...`).
- Tap the notification button (three down arrows).
- You should see the counter incrementing every second.

## The Code

### BLE Server Setup

```cpp
BLEDevice::init("XIAO-ESP32S3");
BLEServer *pServer = BLEDevice::createServer();
```

`BLEDevice::init()` starts the BLE stack and sets the device name that appears during scanning. `createServer()` creates a GATT server — this is what makes the ESP32 a peripheral that other devices can connect to.

### Service and Characteristics

```cpp
BLEService *pService = pServer->createService(SERVICE_UUID);

BLECharacteristic *pLedState = pService->createCharacteristic(
    LED_STATE_UUID, BLECharacteristic::PROPERTY_READ);

BLECharacteristic *pLedControl = pService->createCharacteristic(
    LED_CONTROL_UUID, BLECharacteristic::PROPERTY_WRITE);

BLECharacteristic *pCounter = pService->createCharacteristic(
    COUNTER_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
```

Each characteristic is created with specific **properties** that define what clients can do:
- **READ** — Client can request the current value
- **WRITE** — Client can send a new value
- **NOTIFY** — Server can push updates to the client without being asked

### Callbacks

```cpp
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { ... }
  void onDisconnect(BLEServer* pServer) { ... }
};

class LedControlCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) { ... }
};
```

Callbacks are functions that get called automatically when specific events happen. The server callbacks fire on connection/disconnection. The characteristic callback fires when a client writes a new value — we use this to toggle the LED.

### Advertising

```cpp
BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
pAdvertising->addServiceUUID(SERVICE_UUID);
pAdvertising->start();
```

Advertising is how BLE devices announce their presence. The ESP32 broadcasts a small packet (31 bytes max) containing its name and service UUIDs. Scanner apps pick up these advertisements and display the device.

## How It Works

### BLE vs. Classic Bluetooth

| Feature | Bluetooth Classic | Bluetooth Low Energy (BLE) |
|---------|------------------|---------------------------|
| Power consumption | Higher | Very low (coin cell for years) |
| Data rate | 1-3 Mbps | ~1 Mbps (but lower throughput) |
| Range | ~10-100m | ~10-100m (similar) |
| Connection time | 100+ ms | ~6 ms |
| Use cases | Audio, file transfer | Sensors, wearables, IoT |
| Pairing | Usually required | Optional |
| Concurrent connections | 7 | Depends on implementation |

BLE was designed from the ground up for IoT — devices that send small amounts of data infrequently and need to run on batteries for months or years.

### GATT (Generic Attribute Profile)

GATT defines how BLE devices structure and exchange data. Think of it as a database schema:

```
BLE Device
 └── GATT Server
      └── Service (UUID: 4fafc201-...)
           ├── Characteristic: LED State  (READ)
           │    └── Value: 0 or 1
           ├── Characteristic: LED Control (WRITE)
           │    └── Value: 0 or 1
           └── Characteristic: Counter (READ + NOTIFY)
                ├── Value: 0, 1, 2, 3...
                └── Descriptor: CCCD (Client Characteristic Configuration)
```

**Hierarchy:**
1. **Server** — The device that holds the data (our ESP32).
2. **Service** — A collection of related characteristics (like a class in programming). Identified by a UUID.
3. **Characteristic** — A single data point with defined properties. Identified by a UUID.
4. **Descriptor** — Metadata about a characteristic (e.g., the CCCD descriptor enables/disables notifications).

### UUIDs (Universally Unique Identifiers)

UUIDs are 128-bit numbers used to identify services and characteristics. There are two types:

- **Standard UUIDs** — Defined by the Bluetooth SIG for common use cases. For example, `0x180F` is the Battery Service. These are 16-bit and well-known.
- **Custom UUIDs** — 128-bit random numbers for your own services. We use these because our LED service isn't a standard Bluetooth service.

Our UUIDs:
```
Service:     4fafc201-1fb5-459e-8fcc-c5c9c331914b
LED State:   beb5483e-36e1-4688-b7f5-ea07361b26a8
LED Control: cba1d466-344c-4be3-ab3f-189f80dd7518
Counter:     1a2b3c4d-5e6f-7a8b-9c0d-1e2f3a4b5c6d
```

### Notifications vs. Polling

Without notifications, a client would have to repeatedly read a characteristic to check for changes (polling). This wastes power and bandwidth. With notifications:

1. The client subscribes once (writes to the CCCD descriptor).
2. The server pushes new values whenever they change.
3. The client receives updates instantly without asking.

This is much more efficient — the radio stays asleep until there's actually new data.

### Advertising

When not connected, a BLE device broadcasts advertisement packets at regular intervals (typically every 100-1000ms). These packets contain:

- Device name (or part of it)
- Service UUIDs (so scanners know what the device offers)
- Flags (discoverable, connectable, etc.)
- Optional: manufacturer data, TX power level

The total advertisement payload is limited to **31 bytes**, so not much data fits. The important thing is the service UUID — it lets scanner apps filter for relevant devices.

## Try It Out

1. Upload the sketch and open Serial Monitor.
2. You should see:
   ```
   BLE server started. Advertising as "XIAO-ESP32S3"
   Waiting for connections...
   ```
3. Open nRF Connect on your phone and scan.
4. Connect to "XIAO-ESP32S3". The serial monitor should print:
   ```
   Client connected!
   ```
5. Explore the service and characteristics.
6. Write `1` to the LED Control characteristic — the built-in LED should turn on.
7. Write `0` — it should turn off.
8. Subscribe to the Counter characteristic — watch it increment every second.
9. Disconnect and watch the serial monitor; the device will start advertising again.

## Challenges

1. **LED Brightness Control:** Instead of just on/off, modify the LED Control characteristic to accept brightness values (0-255) and use PWM (from Lesson 6) to control the LED brightness via BLE. Update the LED State characteristic to report the current brightness level.
2. **Sensor Broadcasting:** If you completed Lessons 7-8, add a new service that broadcasts temperature and humidity data via BLE notifications. Use nRF Connect to graph the values in real time.
3. **Bidirectional Communication:** Add a "Message" characteristic that accepts a text string from the phone (up to 20 bytes) and displays it on the Serial Monitor. Add another characteristic that lets the ESP32 send text messages back to the phone.

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Device not found in scan | Not advertising | Check serial monitor for "Advertising" message; ensure BLE is initialized |
| Device not found in scan | Phone BLE off | Enable Bluetooth in phone settings |
| Connection drops immediately | ESP32 BLE stack issue | Reset the board (press the reset button); some phones need a longer connection interval |
| Can't write to characteristic | Wrong data format | In nRF Connect, make sure you're writing a byte value (not text); use hex format `0x01` |
| LED doesn't toggle | GPIO 21 conflict with SD card | If you have an SD card inserted, remove it — GPIO 21 is shared |
| Counter notifications don't arrive | Not subscribed | Tap the three-arrow icon on the Counter characteristic to enable notifications |
| Compile errors about BLE | Wrong board selected | Ensure board is XIAO_ESP32S3; the BLE library is board-specific |
| Upload takes very long | BLE library is large | This is normal — the BLE stack adds significant code size; first compile takes longer |
| Phone can't reconnect after disconnect | Cached connection | On your phone, "forget" the device in Bluetooth settings and try scanning again |

## What's Next

Congratulations — you've completed the fundamentals! You now know how to:
- Control outputs with PWM (Lesson 6)
- Read sensors (Lesson 7)
- Display data on an OLED (Lesson 8)
- Log data to persistent storage (Lesson 9)
- Communicate wirelessly via BLE (Lesson 10)

These building blocks combine into real projects. Some ideas for where to go next:
- **WiFi & Web Server** — Serve a webpage from the ESP32 to monitor sensors from any browser
- **Camera Projects** — Use the built-in OV2640 camera to capture images
- **Deep Sleep & Power Management** — Build battery-powered projects that last months
- **MQTT & Cloud IoT** — Send sensor data to cloud platforms like AWS IoT or Home Assistant
