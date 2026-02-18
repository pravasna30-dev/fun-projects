# Lesson 17: Weather Station

![Difficulty: Advanced](https://img.shields.io/badge/Difficulty-Advanced-orange)

**Estimated Time: 90-120 minutes**

---

## What You'll Learn

- Combining multiple peripherals in a single project (DHT22 + OLED + SD card + WiFi)
- Non-blocking timing with `millis()` to manage multiple tasks
- Logging sensor data as CSV to an SD card
- Building a live-updating web dashboard with HTML and JavaScript
- Tracking min/max values over time
- System design for multi-peripheral projects

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| XIAO ESP32S3 Sense | 1 | Main board with Sense expansion |
| DHT22 sensor | 1 | Temperature and humidity sensor (also called AM2302) |
| SSD1306 OLED display | 1 | 128x64, I2C interface |
| MicroSD card | 1 | Formatted FAT32 (inserted in Sense expansion board) |
| Breadboard | 1 | For connecting components |
| Jumper wires | 5-6 | Male-to-male or male-to-female |
| 10K resistor | 1 | Pull-up for DHT22 data line (some modules have built-in) |

## Circuit Diagram

```
                        +-----+
                        | USB |
                   +----+-----+----+
                   |  XIAO ESP32S3 |
                   |    (top view)  |
              D0 --|1            14|-- 5V
     DHT22-->D1 --|2            13|-- GND
              D2 --|3            12|-- 3V3
              D3 --|4            11|-- D10
   OLED SDA->D4 --|5            10|-- D9
   OLED SCL->D5 --|6             9|-- D8
              D6 --|7             8|-- D7
                   +---------------+

    DHT22 Sensor:          SSD1306 OLED:
    +----------+           +-----------+
    | 1  2  3  |           | GND VCC SDA SCL |
    +----------+           +-----------+
      |  |  |                |   |   |   |
     VCC |  GND             GND 3V3  D4  D5
         |
         D1 (with 10K pull-up to 3V3)

    DHT22 Wiring:                OLED Wiring:
    Pin 1 (VCC)  -> 3V3         GND -> GND
    Pin 2 (DATA) -> D1          VCC -> 3V3
    Pin 3 (NC)   -> N/C         SDA -> D4 (GPIO 6)
    Pin 4 (GND)  -> GND         SCL -> D5 (GPIO 7)

    SD Card: Built into the Sense expansion board (no wiring needed)

    Pull-up Resistor:
    3V3 ----[10K]---- D1 (DHT22 data line)
    (Skip if your DHT22 module has a built-in pull-up)
```

---

## Step-by-Step Instructions

### Step 1: Install Required Libraries

Open Arduino IDE and install these libraries via **Sketch > Include Library > Manage Libraries**:

1. **DHT sensor library** by Adafruit (also install the prompted **Adafruit Unified Sensor** dependency)
2. **Adafruit SSD1306** (also install the prompted **Adafruit GFX Library** dependency)
3. **SD** (included with ESP32 board package)
4. **WiFi** and **WebServer** (included with ESP32 board package)

### Step 2: Prepare the Hardware

1. Insert the MicroSD card into the Sense expansion board slot
2. Connect the DHT22 sensor to pin D1 with a 10K pull-up resistor to 3V3
3. Connect the SSD1306 OLED via I2C (SDA to D4, SCL to D5)
4. Double-check all power connections (3V3, not 5V, for the OLED)

### Step 3: Understand the Software Architecture

This project runs four concurrent tasks, all managed with `millis()`:

| Task | Interval | Description |
|------|----------|-------------|
| Read DHT22 | 2 seconds | Read temperature and humidity |
| Update OLED | 1 second | Refresh the display |
| Log to SD | 30 seconds | Append a CSV row |
| Web server | Continuous | Handle HTTP requests |

Using `millis()` instead of `delay()` is critical here. If we used `delay(2000)` for the DHT22, the web server would be unresponsive for 2 seconds at a time!

### Step 4: Upload and Configure

1. Open `weather-station.ino`
2. Replace `YOUR_SSID` and `YOUR_PASSWORD` with your WiFi credentials
3. Select board **XIAO_ESP32S3** and port `/dev/cu.usbmodem101`
4. Upload the sketch
5. Open Serial Monitor at 115200 baud

### Step 5: Verify Each Subsystem

1. **Serial output** -- You should see temperature and humidity readings every 2 seconds
2. **OLED display** -- Should show current temp/humidity and min/max values
3. **SD card** -- After 30 seconds, check that a `weather.csv` file is being created
4. **Web dashboard** -- Navigate to the IP address shown in Serial Monitor

---

## The Code

### Multi-Peripheral Initialization

The `setup()` function initializes each peripheral in sequence, with error checking at each step. If the OLED or SD card fails, the system continues running with reduced functionality rather than halting completely. This is a resilient design pattern important in real-world IoT.

### Non-Blocking Timing with millis()

```cpp
if (millis() - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = millis();
    readDHT();
}
```

Each task has its own timer variable and interval. The `loop()` function checks each timer on every iteration and only executes the task when enough time has elapsed. This allows all tasks to run "simultaneously."

### CSV Data Logging

Data is logged in a standard CSV format:

```
timestamp,temperature,humidity
12345,23.5,45.2
14345,23.6,44.8
```

This format can be opened directly in Excel or Google Sheets for analysis and charting.

### Web Dashboard

The web dashboard uses JavaScript's `fetch()` API to request data from a `/data` JSON endpoint every 2 seconds. This approach (AJAX) updates the page without a full reload, creating a smooth live-updating experience. A simple bar chart is drawn using HTML `<div>` elements with dynamic widths.

---

## How It Works

### Managing Multiple Peripherals

When multiple devices share the same microcontroller, you need to consider:

- **I2C bus sharing** -- The OLED uses I2C. If you added more I2C devices, they need unique addresses.
- **Pin conflicts** -- Make sure no two peripherals share the same GPIO pin.
- **Timing conflicts** -- The DHT22 uses a precise timing protocol on its data pin. Reading it while the SD card is writing could cause issues, but with `millis()` scheduling, these operations are naturally separated.
- **Memory** -- Each library uses RAM. The ESP32-S3 has plenty (8MB PSRAM), but be mindful of string allocations in the web server.

### Why millis() Instead of delay()

`delay()` blocks the entire processor. During a `delay(2000)`:
- The web server cannot respond to requests
- Button presses are missed
- Other sensors cannot be read

`millis()` returns the number of milliseconds since boot. By comparing the current time to the last time a task ran, you can schedule tasks without blocking.

### CSV Logging Best Practices

- Always flush the file after writing to prevent data loss on power failure
- Include timestamps so you can correlate data with events
- Keep the format simple so it is easy to parse with any tool
- Consider file rotation (new file each day) for long-running stations

---

## Try It Out

1. Upload the sketch and confirm all four subsystems initialize (check Serial Monitor)
2. Breathe on the DHT22 sensor -- you should see humidity spike on both the OLED and web dashboard
3. Let it run for a few minutes, then remove the SD card and open `weather.csv` on your computer
4. Open the web dashboard on your phone (same WiFi network) -- it should work from any device
5. Watch the min/max values on the OLED update as conditions change

---

## Challenges

1. **Add a daily summary** -- At midnight (or after 24 hours of uptime), write a summary line to the SD card with the day's average, min, and max temperature/humidity.

2. **Chart history on the web dashboard** -- Store the last 100 readings in memory and draw a line chart using an HTML5 `<canvas>` element or the lightweight Chart.js library.

3. **Add email or push notifications** -- When temperature exceeds a configurable threshold, send an alert. Look into IFTTT webhooks or a simple SMTP library for ESP32.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| DHT22 returns NaN | Check wiring. Ensure the 10K pull-up resistor is connected between the data pin and 3V3. DHT22 needs at least 2 seconds between reads. |
| OLED shows nothing | Verify I2C address (try 0x3C or 0x3D). Check SDA/SCL wiring. Run an I2C scanner sketch. |
| SD card initialization fails | Ensure the card is FAT32 formatted. Try a different card. The Sense expansion board must be properly seated on the XIAO. |
| Web page does not load | Confirm the IP address from Serial Monitor. Ensure your computer is on the same WiFi network. |
| Readings are unstable | Keep the DHT22 away from heat sources. Add a 100nF capacitor between VCC and GND of the DHT22 for power smoothing. |
| Sketch too large to upload | Make sure you selected the correct partition scheme. The default should work for this project. |

---

## What's Next

In [Lesson 18: Motion Detection Camera](../18-motion-detection/), we will use the onboard OV2640 camera to build a motion detection system that captures photos when movement is detected -- combining image processing with SD card storage and a web interface.
