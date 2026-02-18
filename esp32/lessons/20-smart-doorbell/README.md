# Lesson 20: Smart Doorbell (Capstone Project)

![Difficulty: Expert](https://img.shields.io/badge/Difficulty-Expert-red)

**Estimated Time: 2-3 hours**

---

## What You'll Learn

- Designing a complete IoT product from concept to prototype
- Combining camera, microphone, BLE, WiFi, SD card, and web server
- State machine design for managing complex device behavior
- Handling concurrent tasks on a microcontroller
- Building a full-featured web dashboard with image history
- BLE notifications to a connected phone
- Power and deployment considerations for real-world use
- Thinking like a product engineer, not just a hobbyist

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| XIAO ESP32S3 Sense | 1 | Main board with Sense expansion (camera + mic + SD) |
| Push button | 1 | Momentary normally-open (the doorbell trigger) |
| MicroSD card | 1 | Formatted FAT32 |
| Breadboard | 1 | For the button |
| Jumper wires | 2 | For the button connection |
| USB-C cable | 1 | For programming and power |
| WiFi network | 1 | 2.4 GHz |

## Circuit Diagram

```
                        +-----+
                        | USB |
                   +----+-----+----+
                   |  XIAO ESP32S3 |
                   |    (top view)  |
              D0 --|1            14|-- 5V
  Button---->D1 --|2            13|-- GND ----+
              D2 --|3            12|-- 3V3    |
              D3 --|4            11|-- D10    |
              D4 --|5            10|-- D9     |
              D5 --|6             9|-- D8     |
              D6 --|7             8|-- D7     |
                   +---------------+          |
                                              |
                                              |
    Push Button:                              |
    +-------+                                 |
    |  BTN  |                                 |
    +---+---+                                 |
        |                                     |
        +-----> D1 (GPIO 2)                   |
        |                                     |
        +-------------------------------------+
              (Button connects D1 to GND)

    The button uses the internal pull-up resistor (INPUT_PULLUP).
    Pressing the button pulls D1 to GND (active LOW).

    On-board components (no wiring needed):
    - OV2640 Camera (front of Sense board)
    - PDM Microphone (CLK=GPIO 42, DATA=GPIO 41)
    - MicroSD card slot (side of Sense board)
    - Built-in LED (GPIO 21)
```

---

## Step-by-Step Instructions

### Step 1: Think Like a Product Designer

Before we write any code, let us think about what a smart doorbell needs to do. This is how real product development works -- you start with requirements, not code.

**Functional Requirements:**
1. Detect when someone presses the doorbell button
2. Capture a photo of the visitor
3. Optionally record a short audio clip
4. Save the photo and audio to local storage (SD card)
5. Notify the homeowner (via BLE to phone)
6. Provide a web dashboard to review ring history
7. Allow live camera viewing on demand

**Non-Functional Requirements:**
- Must respond quickly (visitor should not wait more than 1 second)
- Must handle multiple rings in succession
- Must not lose data on power failure
- Should work even when WiFi is down (local storage + BLE)

### Step 2: Design the State Machine

A state machine is the right pattern for this kind of event-driven system. Our doorbell has four states:

```
                    +-------+
         +--------->| IDLE  |<---------+
         |          +---+---+          |
         |              |              |
         |         [Button Press]      |
         |              |              |
         |              v              |
         |        +-----------+        |
         |        | CAPTURING |        |
         |        | (photo +  |        |
         |        |  audio)   |        |
         |        +-----+-----+        |
         |              |              |
         |         [Capture Done]      |
         |              |              |
         |              v              |
         |       +------------+        |
         |       | NOTIFYING  |        |
         |       | (BLE + save|        |
         |       |  to SD)    |        |
         |       +-----+------+        |
         |              |              |
         |         [Notify Done]       |
         |              |              |
         |              v              |
         |       +-------------+       |
         +----- | COOLDOWN     |-------+
                | (3 sec wait) |
                +-------------+
```

This prevents rapid double-rings from causing problems and ensures each step completes before the next begins.

### Step 3: Prepare the Hardware

1. Insert the MicroSD card into the Sense expansion board
2. Connect the push button between pin D1 and GND (no external resistor needed -- we use INPUT_PULLUP)
3. Make sure the camera lens is unobstructed
4. Point the camera toward where a visitor would stand

### Step 4: Install Required Libraries

Open Arduino IDE and install via **Sketch > Include Library > Manage Libraries**:

1. **ESP32 BLE Arduino** (included with ESP32 board package)
2. **SD** (included with ESP32 board package)

All other libraries (WiFi, WebServer, esp_camera) are part of the ESP32 board support.

### Step 5: Upload and Configure

1. Open `smart-doorbell.ino`
2. Replace `YOUR_SSID` and `YOUR_PASSWORD` with your WiFi credentials
3. Select board **XIAO_ESP32S3** and port `/dev/cu.usbmodem101`
4. Set **PSRAM: "OPI PSRAM"** in the Tools menu
5. Upload the sketch
6. Open Serial Monitor at 115200 baud

### Step 6: Test Each Subsystem

1. **Button** -- Press the button; Serial Monitor should show "DOORBELL PRESSED!"
2. **Camera** -- After pressing the button, a photo should be saved to `/doorbell/` on the SD card
3. **Audio** -- A short PDM recording is captured and saved alongside the photo
4. **BLE** -- Open a BLE scanner app on your phone (like "nRF Connect") and look for "XIAO-Doorbell"
5. **Web Dashboard** -- Navigate to the IP address shown in Serial Monitor

### Step 7: Connect via BLE

1. Install the "nRF Connect" app on your phone (iOS or Android)
2. Scan for BLE devices and find "XIAO-Doorbell"
3. Connect to it
4. Subscribe to notifications on the "Ring Event" characteristic
5. Press the doorbell button -- you should receive a BLE notification with the ring details

### Step 8: Explore the Web Dashboard

The web dashboard provides:
- **Live view** -- Tap "View Live Camera" to see the camera feed
- **Ring history** -- A list of all doorbell presses with timestamps and photo thumbnails
- **Photo viewer** -- Tap any ring event to see the full-resolution photo
- **System status** -- SD card usage, WiFi signal, uptime, total rings

---

## The Code

### State Machine Implementation

The `loop()` function implements the state machine with a `switch` statement:

```cpp
switch (currentState) {
  case STATE_IDLE:
    // Check for button press
    break;
  case STATE_CAPTURING:
    // Take photo and record audio
    break;
  case STATE_NOTIFYING:
    // Save to SD, send BLE notification
    break;
  case STATE_COOLDOWN:
    // Wait before accepting next ring
    break;
}
```

Each state performs its work and transitions to the next. This is clean, predictable, and easy to debug.

### Camera Capture

The camera is initialized in JPEG mode at SVGA resolution (800x600). When the doorbell is pressed, a frame is captured and saved to the SD card with a sequential filename like `/doorbell/ring_0042.jpg`.

### PDM Microphone Recording

The onboard PDM (Pulse Density Modulation) microphone captures a short audio clip (about 2 seconds) when the doorbell rings. The raw audio data is saved as a `.raw` file alongside the photo. PDM microphones output a 1-bit stream at high frequency, which is decimated into 16-bit PCM samples by the ESP32's I2S peripheral.

### BLE Notifications

The ESP32-S3 advertises as "XIAO-Doorbell" with a custom BLE service. When a ring event occurs, it sends a notification containing the ring number and timestamp. A phone app connected via BLE receives this notification instantly -- much faster than a WiFi-based push notification.

### Web Dashboard

The web server provides multiple endpoints:
- `/` -- The main HTML dashboard
- `/photo?id=N` -- Serves a specific captured photo
- `/stream` -- Live MJPEG camera stream
- `/status` -- JSON status data for AJAX updates

---

## How It Works

### Managing Concurrent Systems

This project has five subsystems running simultaneously:

| Subsystem | Protocol | Priority | Notes |
|-----------|----------|----------|-------|
| Button | GPIO | Highest | Must respond immediately |
| Camera | DVP | High | Large data, uses PSRAM |
| Microphone | I2S/PDM | Medium | Time-critical during recording |
| BLE | Bluetooth | Medium | Background advertising |
| WiFi/Web | TCP/IP | Lower | Can tolerate slight delays |

The state machine naturally serializes the critical operations (capture, save, notify) while BLE and WiFi run in the background via the ESP32's dual-core architecture.

### Dual-Core Advantage

The ESP32-S3 has two Xtensa LX7 cores:
- **Core 0** -- Runs the WiFi and BLE stacks (managed by the ESP-IDF framework)
- **Core 1** -- Runs your Arduino `loop()` function

This means WiFi and BLE continue working even while your code is busy capturing photos or recording audio.

### SD Card File Management

Each ring event creates two files:
```
/doorbell/ring_0001.jpg    (photo)
/doorbell/ring_0001.raw    (audio)
```

A `log.csv` file tracks all events:
```
id,timestamp_ms,filename
1,45230,ring_0001
2,87450,ring_0002
```

### Power Considerations

For a real deployment, you would need to think about power:
- **USB power** -- The simplest option; a 5V USB adapter provides continuous power
- **Battery** -- The XIAO supports LiPo batteries, but continuous WiFi + camera would drain a battery in hours
- **Deep sleep** -- You could put the ESP32 in deep sleep and wake on the button press GPIO, dramatically reducing power consumption between rings

### Enclosure Ideas

To make this a real product, consider:
- A 3D-printed enclosure with a camera window and button recess
- A waterproof case for outdoor mounting
- An angled bracket to position the camera at face height
- A small speaker for "please wait" audio feedback (use I2S DAC output)

---

## Try It Out

1. Upload the sketch and confirm all subsystems initialize in Serial Monitor
2. Press the doorbell button and verify:
   - Serial Monitor shows "DOORBELL PRESSED!" and the full capture sequence
   - The built-in LED flashes during capture
   - A JPEG file appears on the SD card
3. Open the web dashboard on your computer or phone
4. Press the button again and watch the ring appear in the dashboard
5. Click on a ring event to view the captured photo
6. Try the "Live Camera" feature to see the real-time feed
7. Connect with a BLE scanner app and verify notifications arrive when the button is pressed
8. Press the button several times in rapid succession -- verify the cooldown prevents double-captures

---

## Challenges

1. **Two-way audio** -- Add a speaker (via I2S DAC) and implement a simple intercom feature. When the visitor presses the bell, the homeowner can speak through the web dashboard and the audio plays through the speaker. This requires WebSocket audio streaming.

2. **Face detection** -- Use the ESP32-S3's built-in support for the ESP-WHO face detection framework. Instead of capturing every ring, only save photos where a face is detected. This reduces false triggers from wind blowing the button or animals.

3. **MQTT integration** -- Connect the doorbell to an MQTT broker (like Mosquitto) and integrate with Home Assistant or Node-RED. This allows you to trigger automations (turn on porch light, send push notification, start recording on an NVR) when the doorbell rings.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Camera init fails | Ensure PSRAM is enabled in Tools menu ("OPI PSRAM"). Make sure the Sense expansion board is firmly seated. |
| Button not responding | Check wiring: one leg to D1, other to GND. Verify with a multimeter that the button closes the circuit. Ensure INPUT_PULLUP is configured. |
| SD card write fails | Format as FAT32. Try a different card. Ensure the card is not full. Check that the Sense board is properly connected. |
| BLE not advertising | Check that the BLE initialization did not fail in Serial Monitor. Some ESP32-S3 boards need a BLE antenna connected. |
| Web dashboard not loading | Verify the IP address from Serial Monitor. Ensure your device is on the same WiFi network. Check that port 80 is not blocked. |
| Photos are dark | The camera auto-exposure needs warm-up frames. The sketch discards the first few frames, but low-light conditions may need longer exposure times. |
| Audio quality is poor | PDM microphone quality depends on distance. The built-in mic is close to the board; external noise and vibrations will be picked up. |
| Device crashes during capture | This can happen if PSRAM is not enabled or is full. Monitor free heap via Serial output. Reduce JPEG quality if needed. |

---

## Congratulations!

You have completed all 20 lessons in the ESP32 Learning Path! Starting from a blinking LED, you have built your way up to a complete IoT product that combines:

- Digital and analog I/O (Lessons 1-4)
- Serial communication and protocols (Lessons 5-6)
- Sensors and displays (Lessons 7-8)
- Data storage with SD cards (Lesson 9)
- WiFi and web servers (Lessons 10-11)
- Bluetooth Low Energy (Lesson 12)
- Camera and image processing (Lessons 13-14)
- Audio with the PDM microphone (Lesson 15)
- Over-the-Air updates (Lesson 16)
- Multi-peripheral integration (Lesson 17)
- Computer vision for motion detection (Lesson 18)
- Machine learning on microcontrollers (Lesson 19)
- And finally, a full product prototype (Lesson 20)

You now have a solid foundation in embedded systems and IoT development. Here is where to go from here:

### Next Steps

**Deepen Your ESP32 Knowledge:**
- **ESP-IDF** -- Move beyond Arduino to Espressif's native development framework. It gives you full control over FreeRTOS tasks, memory management, and peripheral drivers. Start at [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/).
- **FreeRTOS** -- Learn the real-time operating system that runs under the hood of every ESP32 project. Understand tasks, queues, semaphores, and mutexes for professional-grade multitasking.
- **ESP-WHO and ESP-SR** -- Espressif's frameworks for face detection/recognition and speech recognition on the ESP32-S3.

**Level Up Your Hardware Skills:**
- **Custom PCB Design** -- Learn KiCad or Eagle to design your own circuit boards. Turn your breadboard prototypes into professional products.
- **3D Printing** -- Design enclosures for your projects using Fusion 360 or OpenSCAD.
- **Power Management** -- Learn about voltage regulators, battery charging circuits, and solar panels for off-grid IoT.

**Explore Advanced IoT Platforms:**
- **MQTT and Home Assistant** -- Build a complete smart home system with standardized protocols.
- **AWS IoT / Google Cloud IoT** -- Connect your devices to cloud platforms for data analytics and remote management.
- **Edge Impulse** -- Take your TinyML skills to the next level with a professional development platform.

**Build Real Projects:**
- Plant watering system with soil moisture sensors
- Home security system with multiple cameras
- Weather station network with mesh networking (ESP-NOW)
- Wearable fitness tracker with BLE and IMU
- Automated pet feeder with scheduling and camera

The skills you have learned in these 20 lessons are the same ones used by professional embedded engineers building real products. The difference between a hobbyist project and a commercial product is iteration, testing, and attention to edge cases -- and you now have the tools to do all three.

Keep building. Keep learning. Keep making things that did not exist before.
