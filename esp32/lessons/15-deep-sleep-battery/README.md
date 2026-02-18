# Lesson 15: Deep Sleep & Battery

**Difficulty:** Advanced ![Advanced](https://img.shields.io/badge/level-advanced-red)

**Estimated Time:** 60-90 minutes

---

## What You'll Learn

- The different power modes of the ESP32-S3 and their current consumption
- How to enter deep sleep and wake up using a timer
- How to wake from deep sleep using a GPIO pin (external button)
- How RTC memory preserves data across deep sleep cycles
- How to detect and report the wake-up reason
- How to calculate expected battery life for real-world deployments
- How to track boot count across sleep cycles

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Built-in LED on pin 21 |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |
| Push button (optional) | 1 | For GPIO wake-up demo |
| LiPo battery (optional) | 1 | 3.7V, for real-world battery testing |

The lesson works without any extra hardware (timer wake-up only). A push button allows you to test GPIO wake-up, and a LiPo battery lets you measure real power consumption.

---

## Background: ESP32-S3 Power Modes

### Why Power Matters

An ESP32-S3 running WiFi draws about **120-240 mA**. A typical 500 mAh LiPo battery would last roughly 2-4 hours. For a battery-powered sensor that reports data once per minute, that is unacceptable. Deep sleep changes the equation dramatically.

### Power Mode Comparison

| Mode | CPU | WiFi/BT | RAM | RTC | Current | Description |
|------|-----|---------|-----|-----|---------|-------------|
| Active | Running | Available | Retained | Running | 80-240 mA | Normal operation |
| Modem Sleep | Running | Off | Retained | Running | 20-30 mA | CPU runs, radio off |
| Light Sleep | Paused | Off | Retained | Running | 0.8-2 mA | CPU frozen, fast wake |
| Deep Sleep | Off | Off | Lost | Running | 10-50 uA | Only RTC domain powered |
| Hibernation | Off | Off | Lost | Minimal | 5-10 uA | Minimal RTC, slowest wake |

### Deep Sleep in Detail

In deep sleep, almost everything is powered off:

```
┌──────────────────────────────────────────────┐
│  ESP32-S3 Chip                               │
│                                              │
│  ┌─────────────┐  ┌─────────────┐            │
│  │  Main CPU   │  │  WiFi/BT    │            │
│  │   (OFF)     │  │   (OFF)     │            │
│  └─────────────┘  └─────────────┘            │
│                                              │
│  ┌─────────────┐  ┌─────────────┐            │
│  │  Main SRAM  │  │  Peripherals│            │
│  │   (OFF)     │  │   (OFF)     │            │
│  └─────────────┘  └─────────────┘            │
│                                              │
│  ┌──────────────────────────────────────┐    │
│  │  RTC Domain (POWERED)                │    │
│  │  ┌──────────┐  ┌──────────────────┐  │    │
│  │  │ RTC Timer│  │ RTC Memory (8KB) │  │    │
│  │  │  (wake)  │  │ (data preserved) │  │    │
│  │  └──────────┘  └──────────────────┘  │    │
│  │  ┌──────────┐  ┌──────────────────┐  │    │
│  │  │ RTC GPIO │  │ RTC Controller   │  │    │
│  │  │  (wake)  │  │  (manages wake)  │  │    │
│  │  └──────────┘  └──────────────────┘  │    │
│  └──────────────────────────────────────┘    │
└──────────────────────────────────────────────┘
```

Only the RTC (Real-Time Clock) domain stays powered. This domain includes:
- **RTC timer** — can be programmed to wake the chip after a set duration
- **RTC GPIO** — selected GPIO pins can trigger a wake-up
- **RTC memory** — 8 KB of SRAM that retains data across deep sleep cycles
- **RTC controller** — manages the wake-up process

When the chip wakes up, it goes through a full reset. The `setup()` function runs from the beginning, just like a power-on. The only data that survives is what you stored in RTC memory.

### Wake-Up Sources

The ESP32-S3 supports several wake-up sources that can be combined:

| Source | Function | Use Case |
|--------|----------|----------|
| Timer | `esp_sleep_enable_timer_wakeup(us)` | Periodic sensor readings |
| External (ext0) | `esp_sleep_enable_ext0_wakeup(pin, level)` | Button press, motion sensor |
| External (ext1) | `esp_sleep_enable_ext1_wakeup(mask, mode)` | Multiple pin sources |
| Touch pad | `esp_sleep_enable_touchpad_wakeup()` | Capacitive touch activation |
| ULP coprocessor | `esp_sleep_enable_ulp_wakeup()` | Ultra-low-power monitoring |

### RTC Memory

Variables declared with `RTC_DATA_ATTR` are stored in RTC memory and survive deep sleep:

```cpp
RTC_DATA_ATTR int bootCount = 0;  // Persists across sleep cycles
int normalVar = 0;                 // Reset to 0 on every boot
```

This is essential for tracking state across sleep cycles without using flash memory (which has limited write endurance).

### Calculating Battery Life

The formula for battery life estimation:

```
Battery Life (hours) = Battery Capacity (mAh) / Average Current (mA)
```

For a duty-cycled application:

```
Average Current = (Active Time * Active Current + Sleep Time * Sleep Current)
                  / (Active Time + Sleep Time)
```

**Example:** A sensor that wakes every 60 seconds, runs for 2 seconds at 100 mA, then sleeps at 0.01 mA:

```
Average Current = (2 * 100 + 58 * 0.01) / 60 = 3.34 mA
Battery Life    = 500 mAh / 3.34 mA = 149 hours = 6.2 days
```

Compare this to always-on: `500 / 100 = 5 hours`. Deep sleep extends battery life by 30x in this example.

---

## Circuit Diagram (Optional Button for GPIO Wake)

If you want to test GPIO wake-up, connect a button to pin D0 (GPIO 1):

```
    XIAO ESP32-S3
    ┌──────────────┐
    │              │
    │          D0 ─┤─── ┌──┐ ─── GND
    │    (GPIO 1)  │    │  │
    │              │    └──┘
    │          GND ┤    Button
    └──────────────┘

  The internal pull-up resistor is enabled in software.
  When the button is pressed, D0 goes LOW, triggering wake-up.
```

If you do not have a button, the sketch still works using timer-based wake-up only.

---

## Step-by-Step Instructions

### 1. Upload the Sketch

1. Select **XIAO_ESP32S3** as the board.
2. Select `/dev/cu.usbmodem101` as the port.
3. Click **Upload**.

### 2. Open the Serial Monitor

Set baud rate to **115200**. On the first boot you will see:

```
=== Lesson 15: Deep Sleep & Battery ===

Boot count: 1
Wake-up reason: Power-on / Reset

--- System Info ---
  CPU Frequency: 240 MHz
  Free Heap:     285432 bytes
  Uptime:        0 sec

LED blink to show we are awake...

Configuring wake-up sources:
  Timer: 30 seconds
  GPIO:  Pin 1 (LOW to wake)

Entering deep sleep in 5 seconds...
  5... 4... 3... 2... 1...
Going to sleep now. Goodnight!
```

### 3. Wait for Timer Wake-Up

After 30 seconds, the board wakes and you see:

```
Boot count: 2
Wake-up reason: Timer

--- System Info ---
  CPU Frequency: 240 MHz
  Free Heap:     285432 bytes
  Uptime:        0 sec
```

Notice the boot count incremented (stored in RTC memory) but uptime resets to 0 (deep sleep is a full reboot).

### 4. Test GPIO Wake-Up (If Button Connected)

Press the button while the board is in deep sleep. You should see:

```
Boot count: 3
Wake-up reason: External (ext0) - GPIO pin
```

### 5. Observe RTC Data Persistence

The sketch stores both the boot count and a cumulative "total awake time" in RTC memory. Each boot, it adds its active time to the running total, demonstrating data persistence across sleep cycles.

---

## The Code

### RTC Memory Variables

Three variables are stored in RTC memory:
- `bootCount` — incremented on every wake-up
- `totalAwakeTimeMs` — cumulative time spent awake across all cycles
- `lastSleepDuration` — the configured sleep duration from the last cycle

### Wake-Up Reason Detection

The `getWakeupReason()` function calls `esp_sleep_get_wakeup_cause()` and returns a human-readable string. This is useful for logging and for executing different logic depending on how the device was awakened.

### Countdown Before Sleep

The sketch provides a 5-second countdown before entering deep sleep. This gives you time to read the serial output and also demonstrates that the board is genuinely awake and running code.

### Sleep Configuration

Both timer and GPIO wake-up sources are configured before calling `esp_deep_sleep_start()`. The ESP32 will wake on whichever trigger fires first.

---

## How It Works

1. On power-on or wake-up, the ESP32-S3 boots and runs `setup()` from the beginning.
2. The RTC memory variables retain their values from before sleep, so `bootCount` increments.
3. The sketch prints diagnostic information, blinks the LED, and starts a countdown.
4. Before sleeping, it configures both wake-up sources: a 30-second timer and GPIO pin 1.
5. `esp_deep_sleep_start()` powers down the CPU, SRAM, WiFi, and all peripherals except the RTC domain.
6. The RTC timer counts down. When it reaches zero (or the GPIO is triggered), the chip resets and the cycle repeats.

---

## Try It Out

- [ ] Boot count increments with each wake-up cycle (check serial monitor)
- [ ] Wake-up reason correctly identifies "Timer" after 30 seconds
- [ ] Wake-up reason correctly identifies "External (ext0)" when button is pressed
- [ ] RTC data (boot count, total awake time) persists across sleep cycles
- [ ] The LED blinks briefly on each wake-up to provide visual confirmation
- [ ] Power consumption drops dramatically during deep sleep (verify with multimeter if available)

---

## Challenges

1. **WiFi sensor with deep sleep** — Combine this lesson with Lesson 11 (MQTT). Wake every 60 seconds, connect to WiFi, publish a sensor reading, then go back to sleep. Track how many successful publishes you have made using RTC memory.
2. **Adaptive sleep duration** — Store a "sleep multiplier" in RTC memory. If the GPIO button is pressed, halve the sleep duration (more frequent wake-ups). If not pressed for 5 cycles, double the sleep duration (less frequent, more power saving). Print the current schedule on each boot.
3. **Battery voltage monitor** — The XIAO ESP32-S3 can read battery voltage on the ADC. Read the voltage on each wake-up, publish it via serial, and if it drops below 3.3V, increase the sleep duration to conserve remaining power.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Board never wakes up | Check that the timer duration is correct (microseconds, not milliseconds). 30 seconds = 30000000 microseconds. |
| Boot count always shows 1 | The RTC_DATA_ATTR variable may not be linking correctly. Ensure it is declared at global scope, not inside a function. |
| GPIO wake-up does not trigger | Not all GPIOs support RTC wake-up. Use an RTC-capable GPIO (GPIO 0-21 on ESP32-S3). Pin 1 (D0) is used in this sketch. |
| Serial output is garbled on wake | The serial port reinitializes on each boot. Add `delay(1000)` after `Serial.begin()` to allow the USB connection to stabilize. |
| Cannot upload new sketch | If the board is sleeping, press the Reset button (or power cycle) and quickly start the upload while it is awake during the 5-second countdown. |
| Higher current than expected | Other peripherals (camera, SD card, LED) may still draw current. Call their deinit/power-down functions before sleeping. |

---

## What's Next

In [Lesson 16: OTA Updates](../16-ota-updates/), you will learn how to update your ESP32's firmware wirelessly over WiFi — no USB cable required after the initial upload.
