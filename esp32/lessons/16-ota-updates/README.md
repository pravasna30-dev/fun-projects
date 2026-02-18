# Lesson 16: OTA (Over-the-Air) Updates

![Difficulty: Advanced](https://img.shields.io/badge/Difficulty-Advanced-orange)

**Estimated Time: 45-60 minutes**

---

## What You'll Learn

- How Over-the-Air (OTA) firmware updates work on ESP32
- Using the ArduinoOTA library for wireless firmware deployment
- mDNS (multicast DNS) for discovering devices on a local network
- Password-protecting OTA updates for security
- Running a simple device info web server alongside OTA
- Why OTA is essential for deployed IoT devices

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| XIAO ESP32S3 Sense | 1 | Main board |
| USB-C cable | 1 | For initial upload only |
| WiFi network | 1 | 2.4 GHz network with known SSID/password |

> **No extra wiring required!** This lesson uses only the onboard LED and WiFi.

## Circuit Diagram

```
No external circuit needed.

   +------------------+
   | XIAO ESP32S3     |
   |                  |
   |   [Built-in LED] |  <-- GPIO 21 (status indicator)
   |                  |
   |   [WiFi Antenna] |  <-- Connects to your network
   |                  |
   +---[USB-C]--------+
        |
        First upload via USB, then OTA!
```

---

## Step-by-Step Instructions

### Step 1: Understand Why OTA Matters

Imagine you have an ESP32 mounted inside a weather station on your roof, or embedded in a wall-mounted smart display. Every time you want to update the firmware, you would need to:

1. Physically access the device
2. Disconnect it from its mounting
3. Plug in a USB cable
4. Upload the new code
5. Reinstall everything

OTA updates solve this problem by letting you push new firmware over WiFi. Once your device has OTA capability, you never need to plug in a USB cable again (unless something goes very wrong).

### Step 2: How OTA Works

The OTA process follows these steps:

1. **Discovery** -- Your ESP32 advertises itself on the network using mDNS (like Bonjour on macOS). The Arduino IDE discovers it automatically.
2. **Authentication** -- If configured, the IDE sends a password hash to the ESP32.
3. **Transfer** -- The new firmware binary is sent over TCP to the ESP32.
4. **Flash** -- The ESP32 writes the new firmware to a secondary partition in flash memory.
5. **Reboot** -- The ESP32 reboots into the new firmware.

The ESP32-S3 uses a dual-partition scheme: while running from partition A, the new firmware is written to partition B. On reboot, it switches to partition B. If the new firmware fails, it can roll back to partition A.

### Step 3: Configure Arduino IDE

1. Open Arduino IDE
2. Select board: **XIAO_ESP32S3**
3. Select port: `/dev/cu.usbmodem101` (for the first upload)
4. Make sure you have the ArduinoOTA library (included with the ESP32 board package)

### Step 4: Upload the Initial Firmware via USB

1. Open `ota-updates.ino`
2. Replace `YOUR_SSID` and `YOUR_PASSWORD` with your WiFi credentials
3. Upload via USB -- this is the **last time** you need the cable!
4. Open Serial Monitor at 115200 baud
5. Note the IP address printed in the console

### Step 5: Verify OTA is Running

1. Open a web browser and navigate to `http://xiao-esp32s3.local` (or use the IP address)
2. You should see the device info page showing IP, uptime, firmware version, and free heap memory
3. In Arduino IDE, go to **Tools > Port** -- you should see a new network port labeled `xiao-esp32s3`

### Step 6: Upload Over the Air

1. In Arduino IDE, select the network port under **Tools > Port** (it will show as `xiao-esp32s3 at [IP_ADDRESS]`)
2. Make a small change to your code (e.g., update `FIRMWARE_VERSION`)
3. Click Upload
4. When prompted for a password, enter `xiao-ota-2024`
5. Watch the progress in the IDE -- the built-in LED will also blink during upload
6. The device reboots automatically with the new firmware

---

## The Code

The sketch is organized into several logical sections:

### WiFi and OTA Setup

```cpp
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
```

We include three libraries: WiFi for network connectivity, ArduinoOTA for over-the-air updates, and WebServer to serve our device info page.

### OTA Configuration

```cpp
ArduinoOTA.setHostname("xiao-esp32s3");
ArduinoOTA.setPassword("xiao-ota-2024");
```

The hostname is used for mDNS discovery -- this is what appears in the Arduino IDE port list. The password provides basic security so not just anyone on your network can flash your device.

### Progress Callbacks

```cpp
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
});
```

ArduinoOTA provides several callback hooks: `onStart`, `onEnd`, `onProgress`, and `onError`. These let you provide visual feedback (like blinking an LED) during the update process.

### Web Server for Device Info

The web server runs on port 80 and shows a simple HTML page with device diagnostics. This is useful for monitoring deployed devices and confirming firmware versions after OTA updates.

---

## How It Works

### mDNS (Multicast DNS)

mDNS allows devices to discover each other on a local network without a central DNS server. When you configure a hostname like `xiao-esp32s3`, the device responds to `xiao-esp32s3.local`. This is the same technology that lets you access your printer at `myprinter.local` on macOS.

### Partition Table

The ESP32-S3 flash is divided into partitions:

```
+------------------+
| Bootloader       |
+------------------+
| Partition Table  |
+------------------+
| OTA Data         |  <-- Tracks which partition to boot from
+------------------+
| App Partition 0  |  <-- Currently running firmware
+------------------+
| App Partition 1  |  <-- New firmware written here during OTA
+------------------+
| SPIFFS/LittleFS  |  <-- File storage (if configured)
+------------------+
```

### Security Considerations

OTA updates are convenient but introduce security risks:

- **Password protection** -- Always set an OTA password. Without it, anyone on your network can flash arbitrary firmware to your device.
- **Network segmentation** -- Consider putting IoT devices on a separate VLAN.
- **HTTPS OTA** -- For production, use encrypted OTA with signed firmware images. ArduinoOTA uses basic password authentication, which is fine for development but not for production.
- **Rollback** -- The ESP32 supports automatic rollback if new firmware crashes on boot.

---

## Try It Out

1. Upload the sketch via USB and confirm the Serial Monitor shows the IP address
2. Open `http://xiao-esp32s3.local` in your browser -- you should see the device info page
3. Note the firmware version displayed
4. Change `FIRMWARE_VERSION` in the code to `"1.1.0"`
5. Select the network port in Arduino IDE and upload via OTA
6. Refresh the web page -- the firmware version should now show `1.1.0`
7. Check that the uptime has reset to 0 (confirming a reboot happened)

---

## Challenges

1. **Add a restart button** -- Add a `/restart` endpoint to the web server that reboots the device remotely. Include a confirmation step so you do not accidentally trigger it.

2. **Firmware update log** -- Store the last 5 update timestamps and previous firmware versions in EEPROM/Preferences so you can track your update history on the web dashboard.

3. **Auto-update check** -- Modify the sketch to periodically check a URL on your local network for a new firmware binary and automatically apply it (look into `httpUpdate` for HTTP-based OTA).

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Network port not showing in IDE | Make sure the board and your computer are on the same network. Try restarting the IDE. On macOS, mDNS should work immediately; on Windows, you may need Bonjour installed. |
| OTA upload fails with "No Answer" | Check that the password matches. Ensure no firewall is blocking UDP port 3232 and TCP port 3232. |
| Upload starts but fails partway | The firmware binary may be too large. Check that your partition scheme supports OTA (needs two app partitions). |
| Device unreachable after OTA | The new firmware may have a bug preventing WiFi connection. You will need to flash via USB to recover. Always keep OTA code in your sketch! |
| `xiao-esp32s3.local` not resolving | Try using the IP address directly. mDNS can be unreliable on some networks, especially across VLANs. |

---

## What's Next

In [Lesson 17: Weather Station](../17-weather-station/), we will build a complete weather monitoring system that combines multiple sensors (DHT22 + OLED + SD card) with a WiFi web dashboard -- a capstone project bringing together skills from several earlier lessons.
