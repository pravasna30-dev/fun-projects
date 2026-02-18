# Lesson 3: Wi-Fi Scanner

**Difficulty:** Beginner

**Estimated Time:** 20-30 minutes

---

## What You'll Learn

- How to use the ESP32-S3's built-in Wi-Fi hardware
- What SSID, RSSI, channels, and encryption types mean
- How to scan for nearby Wi-Fi networks using `WiFi.scanNetworks()`
- How to read and interpret signal strength values
- The difference between Wi-Fi Station mode and Access Point mode
- How to sort and format data for display

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Has built-in Wi-Fi antenna |
| USB-C cable | 1 | Data-capable |
| Computer with Arduino IDE | 1 | |

No extra hardware required. The ESP32-S3 has a built-in Wi-Fi radio and antenna.

---

## Step-by-Step Instructions

### 1. Open and Upload the Sketch

1. Open `wifi-scanner.ino` in the Arduino IDE.
2. Verify your board (**XIAO_ESP32S3**) and port (`/dev/cu.usbmodem101`) are selected.
3. Click **Upload** and wait for "Done uploading."

### 2. Open the Serial Monitor

1. Open the Serial Monitor at **115200** baud.
2. You should see the welcome banner followed by "Scanning..."
3. After 1-3 seconds, a table of nearby Wi-Fi networks will appear.

### 3. Read the Results

Look at the table output. Each row represents a Wi-Fi network with:
- **#** — Row number (sorted by signal strength).
- **RSSI** — Signal strength in dBm (closer to 0 = stronger).
- **Signal** — A visual bar representing signal strength.
- **Ch** — The Wi-Fi channel (1-13 for 2.4 GHz).
- **Encryption** — The security type (WPA2, WPA3, Open, etc.).
- **SSID** — The network name.

### 4. Watch It Repeat

The scan repeats automatically every 10 seconds. Try moving the board closer to or farther from your router and watch the RSSI values change.

---

## The Code

### Including the WiFi Library

```cpp
#include <WiFi.h>
```

This line imports the Wi-Fi library that comes with the ESP32 board package. It provides all the functions for scanning, connecting, and managing Wi-Fi.

### Setting the Wi-Fi Mode

```cpp
WiFi.mode(WIFI_STA);
WiFi.disconnect();
```

We set the ESP32 to **Station (STA) mode**, which means it acts as a client (like your phone or laptop). We call `disconnect()` to make sure we're not connected to any network — we only want to scan, not join.

### Performing the Scan

```cpp
int networkCount = WiFi.scanNetworks();
```

This single function call tells the ESP32 to listen on all Wi-Fi channels and collect information about every network it can hear. The function **blocks** (pauses your code) for 1-3 seconds while the scan runs, then returns the number of networks found.

### Accessing Network Details

```cpp
String ssid = WiFi.SSID(idx);          // Network name
int32_t rssi = WiFi.RSSI(idx);         // Signal strength
int channel = WiFi.channel(idx);       // Wi-Fi channel
wifi_auth_mode_t enc = WiFi.encryptionType(idx);  // Security type
```

After scanning, you access each network's details by index. The index goes from 0 to `networkCount - 1`.

### Sorting by Signal Strength

The code uses a simple bubble sort to order the results by RSSI (strongest signal first). While not the most efficient sorting algorithm, it works perfectly well for the small number of networks we're sorting (usually under 30).

### Cleaning Up

```cpp
WiFi.scanDelete();
```

Scan results are stored in memory. After displaying them, we call `scanDelete()` to free that memory. This is important on a microcontroller where RAM is limited.

---

## How It Works

### What is Wi-Fi?

Wi-Fi is a wireless networking technology that uses radio waves to provide internet connectivity. Your home router broadcasts a signal, and devices like phones, laptops, and ESP32 boards can detect and connect to it.

### SSID (Service Set Identifier)

The **SSID** is simply the name of a Wi-Fi network — it's what you see when you look at the list of available networks on your phone. Some networks hide their SSID; these show up as "(hidden)" in our scanner.

### RSSI (Received Signal Strength Indicator)

**RSSI** measures how strong the Wi-Fi signal is at your location, measured in **dBm** (decibels relative to one milliwatt). Key points:

- RSSI is always a **negative number** (for Wi-Fi).
- **Closer to 0 = stronger signal** (e.g., -30 dBm is excellent).
- **More negative = weaker signal** (e.g., -85 dBm is barely usable).
- The scale is **logarithmic**, not linear. A change of -10 dBm means roughly half the signal power.

| RSSI Range | Quality |
|------------|---------|
| -30 to -50 dBm | Excellent |
| -51 to -60 dBm | Good |
| -61 to -70 dBm | Fair |
| -71 to -80 dBm | Weak |
| -81 to -90 dBm | Very weak |
| Below -90 dBm | Unusable |

### Wi-Fi Channels

Wi-Fi networks operate on specific **channels** — think of them as lanes on a highway. In the 2.4 GHz band (which the ESP32-S3 uses for scanning), there are channels 1 through 13. If too many networks are on the same channel, they interfere with each other and performance drops.

### Encryption Types

Encryption protects the data traveling over the wireless connection:

| Type | Security Level | Notes |
|------|---------------|-------|
| Open | None | No password. Anyone can join and see your traffic. |
| WEP | Very Low | Broken encryption from the 1990s. Avoid. |
| WPA | Low | Older standard, has known vulnerabilities. |
| WPA2 | Good | The current widely-used standard since 2004. |
| WPA3 | Excellent | The newest standard (2018). Best security. |
| WPA2-Enterprise | Good | Uses a login server. Common in offices and universities. |

### Station Mode vs. Access Point Mode

The ESP32-S3 can operate in two Wi-Fi modes:

- **Station (STA):** The board connects to an existing network, like your phone connects to your home Wi-Fi. This is what we use for scanning and in most projects.
- **Access Point (AP):** The board creates its own Wi-Fi network that other devices can connect to. Useful for creating a configuration portal.

---

## Try It Out

1. Upload the sketch and open the Serial Monitor.
2. Verify that you can see your home Wi-Fi network in the list.
3. Check the RSSI value for your network. Is it in the "Excellent" or "Good" range?
4. Look at the encryption column. Hopefully your home network shows WPA2 or WPA3.
5. Try walking to a different room with the board connected to your laptop. Watch how RSSI values change in subsequent scans.
6. Count how many networks your board can detect. In an apartment building, you might see dozens.

---

## Challenges

1. **Add 5 GHz detection.** The ESP32-S3 supports 2.4 GHz only for scanning, but research which ESP32 variants support 5 GHz and add a note to the output indicating the frequency band.

2. **Track signal changes over time.** Modify the sketch to store the RSSI of a specific network (your home Wi-Fi) across multiple scans and print the average, minimum, and maximum values. This creates a simple signal strength monitor.

3. **Find the best Wi-Fi channel.** Analyze the scan results to count how many networks are on each channel. Print a recommendation for which channel would have the least congestion if you were setting up a new router.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **"No networks found"** | Make sure you're not in a Faraday cage (metal enclosure). Try moving closer to a router. Check that `WiFi.mode(WIFI_STA)` is in your code. |
| **Scan returns negative number** | A negative return from `scanNetworks()` indicates an error. Try resetting the board. Make sure no other code is using the Wi-Fi radio. |
| **Very few networks detected** | The ESP32-S3 scans 2.4 GHz only. Many modern networks are 5 GHz only and won't appear. Also, some hidden networks won't show their SSID. |
| **RSSI values seem wrong** | RSSI values are approximations. They can fluctuate by 5-10 dBm between scans. This is normal. |
| **Board resets during scan** | This can happen if power is insufficient. Make sure you're using a good USB port (not a hub). |

---

## What's Next

Now that you can see Wi-Fi networks, the next lesson takes it further: you'll actually **connect** to a network and create a **web server** that lets you control the LED from your phone's browser.

[Next: Lesson 4 - Web Server + LED Control](../04-web-server-led/)
