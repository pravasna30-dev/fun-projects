# ESP32-S3 (Seeed Studio XIAO ESP32-S3 Sense)

A beginner-friendly guide to getting started with the Seeed Studio XIAO ESP32-S3 Sense board, featuring a comprehensive 20-lesson curriculum from your first blink to building a smart doorbell.

---

## Board Overview

| Spec | Details |
|------|---------|
| Chip | ESP32-S3 (Dual Core @ 240MHz) |
| Wi-Fi | 2.4GHz 802.11 b/g/n |
| Bluetooth | BT 5.0 (LE) |
| Flash | 8MB |
| PSRAM | 8MB |
| USB | Native USB-Serial/JTAG (USB-C) |
| Camera | OV2640 (on Sense expansion board) |
| Microphone | PDM digital mic (on Sense expansion board) |
| SD Card | MicroSD slot (on Sense expansion board) |

### Pinout

```
         USB-C
        ┌─────┐
   D0  ─┤1  14├─ D10
   D1  ─┤2  13├─ D9
   D2  ─┤3  12├─ D8
   D3  ─┤4  11├─ D7
   D4  ─┤5  10├─ D6
   D5  ─┤6   9├─ D5
  GND  ─┤7   8├─ 3V3
        └─────┘
```

> Refer to the [official pinout diagram](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#hardware-overview) for full details.

---

## Setup & Installation

### Prerequisites

- **macOS** (this guide is written for macOS; Linux/Windows steps are similar)
- **USB-C cable** (data-capable, not charge-only)
- **Seeed Studio XIAO ESP32-S3 Sense** board

### Step 1: Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Step 2: Install Python 3

```bash
brew install python
python3 --version
```

### Step 3: Install Arduino IDE

**Option A:** Download from [arduino.cc/en/software](https://www.arduino.cc/en/software)

**Option B:** Via Homebrew:

```bash
brew install --cask arduino-ide
```

### Step 4: Add ESP32 Board Support

1. Open **Arduino IDE**
2. Go to **Arduino IDE > Settings** (`Cmd + ,`)
3. In **Additional Board Manager URLs**, add:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

4. Go to **Tools > Board > Boards Manager**, search **esp32**, install **esp32 by Espressif Systems**

### Step 5: Select the Board

1. Plug in your XIAO ESP32-S3 via USB-C
2. **Tools > Board > esp32** → select `XIAO_ESP32S3`
3. **Tools > Port** → select `/dev/cu.usbmodem101`

### Step 6: Install esptool (Optional)

```bash
python3 -m venv ~/esp-tools
source ~/esp-tools/bin/activate
pip install esptool pyserial
```

**Useful commands:**

```bash
esptool --port /dev/cu.usbmodem101 chip-id       # Identify board
esptool --port /dev/cu.usbmodem101 flash-id       # Flash info
esptool --port /dev/cu.usbmodem101 read-flash 0 0x800000 backup.bin  # Backup firmware
```

---

## Lesson Plan

20 hands-on projects progressing from your first LED blink to building a complete IoT smart doorbell. Each lesson builds on the previous ones.

### Beginner (Lessons 1-6)

Core fundamentals — mostly no extra hardware needed.

| # | Lesson | What You'll Learn | Extra Hardware |
|---|--------|-------------------|----------------|
| 1 | [Blink LED](lessons/01-blink-led/) | GPIO output, setup/loop, uploading sketches | None |
| 2 | [Serial Communication](lessons/02-serial-communication/) | Serial.print, reading input, debugging | None |
| 3 | [Wi-Fi Scanner](lessons/03-wifi-scanner/) | Wi-Fi library, scanning networks, RSSI | None |
| 4 | [Web Server + LED Control](lessons/04-web-server-led/) | HTTP server, HTML, browser control | None |
| 5 | [Button & Interrupts](lessons/05-button-and-interrupts/) | GPIO input, pull-ups, hardware interrupts | Push button |
| 6 | [PWM & RGB LED](lessons/06-pwm-and-rgb-led/) | Analog output, PWM, color mixing | RGB LED + resistors |

### Intermediate (Lessons 7-12)

Sensors, displays, connectivity, and the camera.

| # | Lesson | What You'll Learn | Extra Hardware |
|---|--------|-------------------|----------------|
| 7 | [Temperature Sensor](lessons/07-temperature-sensor/) | I2C/OneWire, reading DHT22, data formatting | DHT22 sensor |
| 8 | [OLED Display](lessons/08-oled-display/) | I2C display, graphics library, animations | SSD1306 OLED |
| 9 | [Data Logger (SD Card)](lessons/09-data-logger-sd-card/) | File system, CSV logging, timestamps | MicroSD card |
| 10 | [Bluetooth LE Basics](lessons/10-bluetooth-le-basics/) | BLE server, GATT, characteristics, phone app | None (phone app) |
| 11 | [MQTT IoT Dashboard](lessons/11-mqtt-iot-dashboard/) | MQTT pub/sub, cloud broker, real-time data | None |
| 12 | [Camera Capture](lessons/12-camera-capture/) | OV2640 camera, JPEG capture, serving images | None (Sense board) |

### Advanced (Lessons 13-17)

Combining multiple features into real projects.

| # | Lesson | What You'll Learn | Extra Hardware |
|---|--------|-------------------|----------------|
| 13 | [Camera Live Stream](lessons/13-camera-stream/) | MJPEG streaming, multi-client, frame rate | None (Sense board) |
| 14 | [Audio Recorder](lessons/14-audio-recorder/) | PDM microphone, I2S, WAV files, SD storage | MicroSD card |
| 15 | [Deep Sleep & Battery](lessons/15-deep-sleep-battery/) | Power modes, wake sources, battery optimization | LiPo battery (optional) |
| 16 | [OTA Updates](lessons/16-ota-updates/) | Over-the-air firmware, ArduinoOTA, mDNS | None |
| 17 | [Weather Station](lessons/17-weather-station/) | Multi-sensor dashboard, OLED + web + SD logging | DHT22 + OLED + SD |

### Expert (Lessons 18-20)

Full integrated systems and advanced techniques.

| # | Lesson | What You'll Learn | Extra Hardware |
|---|--------|-------------------|----------------|
| 18 | [Motion Detection Camera](lessons/18-motion-detection/) | Frame differencing, alerts, image saving | MicroSD card |
| 19 | [TinyML Gesture Recognition](lessons/19-tinyml-gesture/) | TensorFlow Lite Micro, IMU, ML on device | MPU6050 IMU |
| 20 | [Smart Doorbell (Capstone)](lessons/20-smart-doorbell/) | Full IoT product: camera + audio + BLE + web | Push button |

---

## Shopping List

Everything you need for all 20 lessons:

| Item | Used In Lessons | Approx. Cost |
|------|----------------|-------------|
| Seeed Studio XIAO ESP32-S3 Sense | All | ~$15 |
| USB-C data cable | All | ~$5 |
| Breadboard + jumper wires | 5-8, 17, 19-20 | ~$8 |
| Push button (x2) | 5, 20 | ~$1 |
| RGB LED (common cathode) | 6 | ~$1 |
| 220 ohm resistors (x5) | 5-6 | ~$1 |
| 10K ohm resistor | 7 | ~$1 |
| DHT22 temperature/humidity sensor | 7, 17 | ~$5 |
| SSD1306 128x64 OLED (I2C) | 8, 17 | ~$5 |
| MicroSD card (FAT32, 4-32GB) | 9, 14, 17-18, 20 | ~$5 |
| MPU6050 accelerometer/gyroscope | 19 | ~$3 |
| LiPo battery 3.7V (optional) | 15 | ~$5 |
| **Total** | | **~$55** |

> You can start with just the board and USB cable for lessons 1-4, 10-13, 15-16.

---

## Troubleshooting

### Board not detected / no port showing up

- Try a different USB-C cable (must be a **data cable**, not charge-only)
- Try a different USB port
- Unplug and replug the board
- Hold **BOOT** while plugging in USB to enter bootloader mode

### Upload fails

- Correct board (**XIAO_ESP32S3**) and port selected?
- Close other serial programs (Serial Monitor, screen, miniterm)
- Hold **BOOT** → press **RESET** → release **BOOT** → try uploading

### Serial Monitor shows garbage

- Baud rate must match your code (`115200` in all lessons)
- Press **RESET** on the board

### "Permission denied" on serial port (Linux)

```bash
sudo usermod -a -G dialout $USER
# Then log out and back in
```

---

## Useful Resources

- [Seeed Studio XIAO ESP32-S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [ESP32-S3 Datasheet (Espressif)](https://www.espressif.com/en/products/socs/esp32-s3)
- [Arduino ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [Seeed Studio Forum](https://forum.seeedstudio.com/)

---

## File Structure

```
esp32/
├── README.md                            # This file (setup + lesson plan)
├── lessons/
│   ├── 01-blink-led/                    # Beginner
│   ├── 02-serial-communication/
│   ├── 03-wifi-scanner/
│   ├── 04-web-server-led/
│   ├── 05-button-and-interrupts/
│   ├── 06-pwm-and-rgb-led/
│   ├── 07-temperature-sensor/           # Intermediate
│   ├── 08-oled-display/
│   ├── 09-data-logger-sd-card/
│   ├── 10-bluetooth-le-basics/
│   ├── 11-mqtt-iot-dashboard/
│   ├── 12-camera-capture/
│   ├── 13-camera-stream/                # Advanced
│   ├── 14-audio-recorder/
│   ├── 15-deep-sleep-battery/
│   ├── 16-ota-updates/
│   ├── 17-weather-station/
│   ├── 18-motion-detection/             # Expert
│   ├── 19-tinyml-gesture/
│   └── 20-smart-doorbell/
└── examples/
```
