# Lesson 11: MQTT IoT Dashboard

**Difficulty:** Intermediate ![Intermediate](https://img.shields.io/badge/level-intermediate-yellow)

**Estimated Time:** 60-90 minutes

---

## What You'll Learn

- What the MQTT protocol is and why it is the backbone of IoT communication
- The publish/subscribe messaging model and how it differs from HTTP request/response
- How MQTT topics, QoS levels, and retained messages work
- How to connect to a public MQTT broker using the PubSubClient library
- How to publish sensor-like data (uptime, free heap memory, simulated temperature)
- How to subscribe to a topic and use incoming messages to control the built-in LED
- How to use MQTT Explorer or similar tools to build a live dashboard

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Built-in LED on pin 21 |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |
| WiFi network | 1 | 2.4 GHz network with internet access |

No extra hardware is needed. We use the free public MQTT broker at `broker.hivemq.com` and the board's built-in LED.

---

## Background: What Is MQTT?

**MQTT** (Message Queuing Telemetry Transport) is a lightweight messaging protocol designed for constrained devices and unreliable networks. It was invented in 1999 by Andy Stanford-Clark (IBM) and Arlen Nipper for monitoring oil pipelines over satellite links. Today it is the dominant protocol for IoT.

### Publish/Subscribe Model

Unlike HTTP where a client sends a request and a server sends a response, MQTT uses a **publish/subscribe** (pub/sub) model with three actors:

```
  Publisher ──publish──▶ BROKER ──deliver──▶ Subscriber
  (sensor)              (server)             (dashboard)
```

- **Publisher** — sends a message to a **topic** (like a channel name).
- **Subscriber** — registers interest in one or more topics.
- **Broker** — the central server that receives all messages and routes them to the right subscribers.

Publishers and subscribers never talk directly to each other. They only talk to the broker. This decoupling is what makes MQTT so scalable.

### Topics

Topics are UTF-8 strings with a hierarchical structure using `/` as a separator:

```
home/living-room/temperature
home/kitchen/humidity
office/server-room/cpu-load
```

Subscribers can use wildcards:
- `+` matches a single level: `home/+/temperature` matches `home/kitchen/temperature`
- `#` matches all remaining levels: `home/#` matches everything under `home/`

### QoS Levels

MQTT defines three Quality of Service levels:

| QoS | Name | Guarantee | Use Case |
|-----|------|-----------|----------|
| 0 | At most once | Fire and forget, may be lost | Frequent sensor readings |
| 1 | At least once | Delivered, but may be duplicated | Important alerts |
| 2 | Exactly once | Delivered exactly once, slowest | Billing, critical commands |

For most IoT sensor data, QoS 0 is fine. For commands (like turning on an LED), QoS 1 is a good choice.

### Retained Messages

When a message is published with the **retain** flag set, the broker stores it. Any new subscriber to that topic immediately receives the last retained message. This is useful for status topics (e.g., device online/offline).

---

## Step-by-Step Instructions

### 1. Install the PubSubClient Library

1. Open Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries**.
3. Search for **PubSubClient** by Nick O'Leary.
4. Click **Install**.

### 2. Install MQTT Explorer (Optional but Recommended)

MQTT Explorer is a free desktop tool that lets you browse all topics on a broker in real time. Download it from [mqtt-explorer.com](http://mqtt-explorer.com/).

1. Open MQTT Explorer.
2. Create a connection to `broker.hivemq.com` on port `1883`.
3. Click **Connect**. You will see a live tree of topics from users around the world.

### 3. Configure Your WiFi Credentials

Open `mqtt-dashboard.ino` and replace the placeholders near the top:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

### 4. Choose a Unique Device ID

The sketch uses a device ID based on the ESP32's MAC address to avoid collisions on the public broker. No changes needed, but note the topic prefix printed to the serial monitor.

### 5. Upload the Sketch

1. Select **XIAO_ESP32S3** as the board.
2. Select `/dev/cu.usbmodem101` as the port.
3. Click **Upload**.

### 6. Open the Serial Monitor

Set the baud rate to **115200**. You should see:

```
Connecting to WiFi...
WiFi connected! IP: 192.168.1.42
Connecting to MQTT broker...
MQTT connected!
Device ID: esp32_AABBCC
Publishing to: esp32_AABBCC/sensors/temperature
Publishing to: esp32_AABBCC/sensors/uptime
Publishing to: esp32_AABBCC/sensors/heap
Subscribed to: esp32_AABBCC/led/control
```

### 7. Watch Data in MQTT Explorer

In MQTT Explorer, look for your device's topic tree. You will see values updating every 5 seconds.

### 8. Control the LED

In MQTT Explorer, publish a message to your device's `led/control` topic:
- Publish `ON` to turn the LED on.
- Publish `OFF` to turn the LED off.

---

## The Code

### Library Includes and Configuration

The sketch includes `WiFi.h` for network connectivity and `PubSubClient.h` for the MQTT client. The broker address is set to `broker.hivemq.com` on the standard unencrypted port 1883.

### Device ID Generation

The function `getDeviceId()` reads the ESP32's MAC address and creates a unique string like `esp32_AABBCC`. This ensures your topics do not collide with other users on the public broker.

### WiFi Connection

The `setupWiFi()` function connects to your network and prints the assigned IP address. It retries in a loop until connected.

### MQTT Callback

The `mqttCallback()` function is called whenever a message arrives on a subscribed topic. It checks the payload for "ON" or "OFF" and controls the LED accordingly.

### Reconnect Logic

The `reconnect()` function handles MQTT disconnections. It attempts to reconnect and re-subscribes to the LED control topic each time. This is important because subscriptions are lost when the connection drops.

### Publishing Data

In `loop()`, every 5 seconds the sketch publishes three values:
- **temperature** — a simulated reading using `random()` (25.0-35.0 degrees)
- **uptime** — seconds since boot using `millis()`
- **heap** — free heap memory in bytes using `ESP.getFreeHeap()`

---

## How It Works

1. The ESP32 connects to WiFi, then to the MQTT broker over TCP port 1883.
2. It subscribes to `<deviceId>/led/control` so it can receive commands.
3. Every 5 seconds, it publishes three sensor readings to separate sub-topics.
4. The broker forwards those messages to anyone subscribed (like MQTT Explorer).
5. When you publish a message to the LED control topic, the broker forwards it to the ESP32, which turns the LED on or off.

The entire flow is asynchronous. The ESP32 does not need to know who is listening. The dashboard does not need to know the ESP32's IP address. The broker handles all routing.

---

## Try It Out

- [ ] Serial monitor shows "MQTT connected!" and prints published values every 5 seconds
- [ ] MQTT Explorer displays live topic tree with temperature, uptime, and heap values
- [ ] Publishing "ON" to the led/control topic turns on the built-in LED
- [ ] Publishing "OFF" turns the LED off
- [ ] Unplugging WiFi causes reconnection messages, and the sketch recovers automatically

---

## Challenges

1. **Add a light sensor topic** — If you have a photoresistor from earlier lessons, publish real light readings instead of (or in addition to) simulated temperature.
2. **Build a two-way thermostat** — Subscribe to a `setpoint` topic. When the simulated temperature exceeds the setpoint, publish an "ALERT" message to a new topic and blink the LED.
3. **Implement QoS 1** — Change the publish and subscribe calls to use QoS 1. Observe in MQTT Explorer how the behavior differs (hint: use the PubSubClient's return values to confirm delivery).

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "WiFi connected" but MQTT fails | Ensure your network allows outbound connections on port 1883. Some corporate/school networks block this. Try a phone hotspot. |
| Messages not appearing in MQTT Explorer | Double-check the topic name. MQTT topics are case-sensitive. Copy the exact topic from the serial monitor. |
| LED does not respond | Make sure you publish to the exact topic shown in the serial monitor. Payload must be exactly `ON` or `OFF` (uppercase). |
| Frequent disconnections | The public broker may be busy. Add a longer keepalive interval or try `test.mosquitto.org` as an alternative broker. |
| Compile error about PubSubClient | Make sure you installed the library by Nick O'Leary, not a similarly named one. |

---

## What's Next

In [Lesson 12: Camera Capture](../12-camera-capture/), you will initialize the OV2640 camera on the Sense expansion board and serve captured JPEG images through a web browser.
