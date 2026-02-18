# Lesson 7: Temperature Sensor (DHT22)

**Difficulty:** Intermediate

**Estimated Time:** 60-75 minutes

---

## What You'll Learn

- How digital temperature sensors work internally
- The difference between I2C and OneWire communication protocols
- How to install and use Arduino libraries from the Library Manager
- How to read temperature and humidity from a DHT22 sensor
- How to calculate the heat index (feels-like temperature)
- How to format sensor data for clear serial output

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Main microcontroller |
| DHT22 (AM2302) sensor | 1 | Digital temp/humidity sensor, 3 or 4 pins |
| 10K ohm resistor | 1 | Pull-up resistor for the data line |
| Breadboard | 1 | For prototyping |
| Jumper wires | 4+ | Male-to-male |
| USB-C cable | 1 | For programming and power |

## Circuit Diagram

```
    XIAO ESP32-S3                    DHT22 Sensor
   ┌─────────────┐                 ┌──────────────┐
   │            3V3├────────────────┤ VCC (pin 1)  │
   │               │      ┌────────┤ DATA (pin 2) │
   │             D1├──────┤        │ NC   (pin 3)  │
   │               │    [10KΩ]     │ GND  (pin 4)  │
   │            3V3├──────┘        └──────┬────────┘
   │            GND├───────────────────────┘
   └─────────────┘

   DHT22 Pinout (looking at the front, grid side):
   ┌────────────────┐
   │    DHT22       │
   │  ┌──────────┐  │
   │  │ ▓▓▓▓▓▓▓▓ │  │
   │  └──────────┘  │
   └─┬──┬──┬──┬─────┘
     1  2  3  4
    VCC DAT NC GND

   The 10K pull-up resistor connects between VCC (3V3) and the DATA pin.
   This ensures the data line has a defined HIGH state when the sensor
   isn't actively pulling it LOW.
```

> **Note:** Some DHT22 modules come on a small PCB with the pull-up resistor already built in. If yours has three pins labeled VCC, DATA, GND, you can skip the external 10K resistor.

## Step-by-Step Instructions

### Step 1: Install the DHT Library

The DHT22 uses a proprietary single-wire protocol (not the same as Dallas OneWire). We need a library to handle the timing-sensitive communication.

1. Open Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...** (or press Cmd+Shift+I).
3. In the search box, type **"DHT sensor library"**.
4. Find **"DHT sensor library" by Adafruit** — click **Install**.
5. When prompted, also install the dependency **"Adafruit Unified Sensor"** — click **Install All**.

You should now see both libraries listed as installed.

### Step 2: Wire the DHT22

1. Place the DHT22 on your breadboard.
2. Connect pin 1 (VCC) to the 3V3 rail.
3. Connect pin 4 (GND) to the GND rail.
4. Connect pin 2 (DATA) to **D1** on the XIAO.
5. Place the 10K resistor between pin 1 (VCC/3V3) and pin 2 (DATA). This is the pull-up resistor — it's essential for reliable communication.
6. Pin 3 (NC) is not connected — leave it empty.

### Step 3: Verify Your Wiring

Before uploading code, double-check:
- VCC goes to 3.3V (NOT 5V — the XIAO ESP32-S3 is a 3.3V board)
- The pull-up resistor bridges VCC and DATA
- GND is connected

### Step 4: Upload the Sketch

1. Open `temperature-sensor.ino` in Arduino IDE.
2. Select **Tools > Board > XIAO_ESP32S3**.
3. Select **Tools > Port > /dev/cu.usbmodem101**.
4. Click Upload.
5. Open the Serial Monitor at **115200 baud**.

## The Code

### Library Includes and Setup

```cpp
#include "DHT.h"
#define DHTPIN D1
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
```

We include the Adafruit DHT library, define which pin the sensor is on, and specify the sensor type (DHT22 as opposed to DHT11, which is less accurate). The `DHT` object handles all the timing-critical communication.

### Reading the Sensor

```cpp
float humidity = dht.readHumidity();
float tempC = dht.readTemperature();       // Celsius
float tempF = dht.readTemperature(true);   // Fahrenheit
```

Each call initiates a full read cycle. The DHT22 needs about 2 seconds between reads — reading faster will return stale data or NaN.

### Heat Index Calculation

```cpp
float heatIndexC = dht.computeHeatIndex(tempC, humidity, false);
```

The heat index (or "feels like" temperature) accounts for how humidity affects the human body's ability to cool itself through evaporation. At high humidity, sweat doesn't evaporate efficiently, so it feels hotter than the actual temperature.

### Error Handling

```cpp
if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
  Serial.println("Failed to read from DHT sensor!");
  return;
}
```

The sensor can return NaN (Not a Number) if the wiring is wrong, the pull-up resistor is missing, or the timing is off. Always check for this before using the values.

## How It Works

### How Digital Temperature Sensors Work

The DHT22 contains two sensors in one package:

1. **Thermistor** — A resistor whose resistance changes with temperature. The DHT22's internal circuitry measures this resistance and converts it to a digital value.
2. **Capacitive humidity sensor** — A thin polymer film that absorbs water molecules, changing its electrical capacitance. The chip measures this capacitance and converts it to a relative humidity percentage.

The built-in microcontroller reads both sensors, applies calibration, and sends the data digitally when requested.

### The DHT Communication Protocol

The DHT22 uses a proprietary single-wire protocol:

1. The microcontroller pulls the data line LOW for at least 1 ms (the "start" signal).
2. The microcontroller releases the line; the pull-up resistor pulls it HIGH.
3. The DHT22 responds by pulling the line LOW for 80 microseconds, then HIGH for 80 microseconds.
4. The DHT22 sends 40 bits of data (16 bits humidity + 16 bits temperature + 8 bits checksum).
5. Each bit is encoded as a LOW pulse followed by a HIGH pulse. The duration of the HIGH pulse determines whether it's a 0 (26-28 us) or 1 (70 us).

This is why the library needs precise timing and why the pull-up resistor is critical.

### I2C vs. OneWire vs. DHT Protocol

| Feature | I2C | OneWire (Dallas) | DHT Protocol |
|---------|-----|-------------------|-------------|
| Wires needed | 2 (SDA + SCL) | 1 (data) | 1 (data) |
| Multiple devices | Yes (addresses) | Yes (ROM codes) | No (1 sensor per pin) |
| Speed | 100-400 kHz typical | ~16 kbps | ~5 kbps |
| Pull-up needed | Yes (on both lines) | Yes (4.7K) | Yes (10K) |
| Example sensors | BME280, SHT31 | DS18B20 | DHT22, DHT11 |

### DHT22 vs. DHT11

| Specification | DHT22 | DHT11 |
|---------------|-------|-------|
| Temperature range | -40 to 80°C | 0 to 50°C |
| Temperature accuracy | ±0.5°C | ±2°C |
| Humidity range | 0-100% | 20-80% |
| Humidity accuracy | ±2-5% | ±5% |
| Read interval | 2 seconds | 1 second |
| Price | ~$3-5 | ~$1-2 |

## Try It Out

1. After uploading, open the Serial Monitor at **115200 baud**.
2. You should see readings like:
   ```
   ===== Weather Station =====
   Temperature:  23.4 °C  |  74.1 °F
   Humidity:     45.2 %
   Heat Index:   23.1 °C  |  73.6 °F
   ============================
   ```
3. Breathe on the sensor — you should see humidity spike quickly.
4. Place your finger near (not on) the sensor — temperature should rise slightly.
5. Readings update every 2 seconds.

## Challenges

1. **Min/Max Tracker:** Keep track of the highest and lowest temperature and humidity readings since boot and display them alongside the current values.
2. **Comfort Zone Alert:** Define a "comfortable" range (e.g., 20-25°C and 40-60% humidity) and print a warning when conditions fall outside it. If you completed Lesson 6, make the RGB LED change color based on comfort level — green for comfortable, yellow for borderline, red for extreme.
3. **Running Average:** Implement a rolling average over the last 10 readings to smooth out sensor noise. Compare the raw and averaged values in your serial output.

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| "Failed to read from DHT sensor!" | Missing pull-up resistor | Add a 10K resistor between DATA and VCC |
| NaN readings | Wrong pin definition | Verify `DHTPIN` matches your wiring |
| Readings seem stuck | Reading too fast | Ensure at least 2 seconds between reads |
| Temperature reads -999 | Sensor not powered | Check VCC/GND connections |
| Humidity always reads 0% | Wrong sensor type | Verify `DHTTYPE` is `DHT22`, not `DHT11` |
| Library not found | Not installed | Re-do Step 1 — install both DHT and Unified Sensor libraries |
| Compilation error about Adafruit_Sensor | Missing dependency | Install "Adafruit Unified Sensor" from Library Manager |

## What's Next

Now that we can read sensor data, let's display it on a screen! In the next lesson, we'll connect an **OLED display** and learn about I2C communication in depth.

[Lesson 8: OLED Display ->](../08-oled-display/)
