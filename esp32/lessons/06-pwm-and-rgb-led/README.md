# Lesson 6: PWM & RGB LED

**Difficulty:** Beginner

**Estimated Time:** 45-60 minutes

---

## What You'll Learn

- What Pulse Width Modulation (PWM) is and how it works
- The relationship between duty cycle, frequency, and resolution on the ESP32
- How to use `ledcWrite()` to control LED brightness
- How RGB color mixing works with a common-cathode RGB LED
- How to create smooth rainbow fade effects
- How to accept serial commands for custom color control

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Main microcontroller |
| Common-cathode RGB LED | 1 | 4-pin, longest pin is cathode (GND) |
| 220 ohm resistors | 3 | One per color channel |
| Breadboard | 1 | For prototyping |
| Jumper wires | 5+ | Male-to-male |
| USB-C cable | 1 | For programming and power |

## Circuit Diagram

```
    XIAO ESP32-S3              RGB LED (Common Cathode)
   ┌─────────────┐
   │             D1├───[220Ω]───┤ Red    (pin 1)
   │             D2├───[220Ω]───┤ Green  (pin 3)
   │             D3├───[220Ω]───┤ Blue   (pin 4)
   │            GND├────────────┤ GND    (pin 2 - longest)
   └─────────────┘

   RGB LED Pin Layout (flat side facing you):
   ┌──────────┐
   │  R G B   │
   │  | | |   │
   └──┤─┤─┤───┘
      1 2 3 4
      R C G B
        │
       GND (longest pin)
```

> **Note:** A common-cathode RGB LED shares a ground pin. The three shorter pins control Red, Green, and Blue. Make sure you identify the longest pin (cathode/GND) correctly.

## Step-by-Step Instructions

### Step 1: Understand PWM Before Wiring

Before we touch hardware, let's understand what PWM actually does. A digital pin can only be HIGH (3.3V) or LOW (0V) — it cannot output, say, 1.65V. PWM fakes an analog voltage by rapidly switching between HIGH and LOW. The percentage of time the signal is HIGH is called the **duty cycle**.

- **0% duty cycle** = always LOW = LED off
- **50% duty cycle** = HIGH half the time = LED at half brightness
- **100% duty cycle** = always HIGH = LED at full brightness

### Step 2: Wire the RGB LED

1. Place the RGB LED on your breadboard.
2. Identify the longest pin — this is the common cathode (GND).
3. Connect the longest pin to the GND rail on your breadboard.
4. Connect a 220-ohm resistor from each of the three color pins to separate rows.
5. Wire from each resistor to the XIAO:
   - Red pin -> D1
   - Green pin -> D2
   - Blue pin -> D3
6. Connect the GND rail to the XIAO's GND pin.

### Step 3: Install the Board Support (if not already done)

1. Open Arduino IDE.
2. Go to **Tools > Board > Boards Manager**.
3. Search for "esp32" and install the **esp32 by Espressif Systems** package.
4. Select **Tools > Board > XIAO_ESP32S3**.
5. Select **Tools > Port > /dev/cu.usbmodem101**.

### Step 4: Upload the Sketch

1. Open the file `pwm-rgb-led.ino` in Arduino IDE.
2. Click the Upload button (right arrow icon).
3. Wait for "Done uploading" to appear.
4. Open the Serial Monitor at **115200 baud**.

## The Code

The sketch is organized into several sections:

### Pin and Channel Setup

```cpp
const int RED_PIN   = D1;
const int GREEN_PIN = D2;
const int BLUE_PIN  = D3;
```

We define which GPIO pins connect to each color leg of the RGB LED. On the XIAO ESP32-S3, `D1`, `D2`, and `D3` map to specific GPIO numbers, but using the `Dx` aliases keeps the code portable and readable.

### PWM Configuration

```cpp
const int PWM_FREQ       = 5000;  // 5 kHz
const int PWM_RESOLUTION = 8;     // 8-bit = 0-255
```

The ESP32-S3 has a dedicated LED Control (LEDC) peripheral. Unlike basic Arduinos that use `analogWrite()`, the ESP32 lets you configure:

- **Frequency**: How fast the signal toggles. 5 kHz is fast enough that you won't see flickering.
- **Resolution**: How many brightness steps you get. 8-bit gives 256 levels (0-255). The ESP32 supports up to 16-bit (65,536 levels), but 8-bit is standard for RGB work.

### Rainbow Fade Logic

The rainbow effect works by cycling a hue value from 0 to 360 degrees around the color wheel, then converting that hue to RGB values. This uses a simplified HSV-to-RGB conversion where saturation and value are always at maximum.

### Serial Command Parsing

The sketch reads serial input looking for comma-separated RGB values like `255,0,128`. It uses `parseInt()` to extract each number and applies the color immediately.

## How It Works

### PWM on the ESP32-S3

The ESP32-S3 has 8 LEDC channels (0-7), each of which can be independently configured with its own frequency and resolution. When you call `ledcAttach(pin, frequency, resolution)`, the hardware generates the PWM signal — the CPU doesn't have to bit-bang the signal, so it's perfectly stable even when your code is busy doing other things.

**Frequency vs. Resolution Tradeoff:**

The ESP32-S3 runs its LEDC timer from an 80 MHz clock. The maximum resolution at a given frequency is:

```
max_resolution_bits = log2(80,000,000 / frequency)
```

| Frequency | Max Resolution | Brightness Levels |
|-----------|---------------|-------------------|
| 1 kHz     | 16-bit        | 65,536            |
| 5 kHz     | ~13-bit       | 8,192             |
| 40 kHz    | ~11-bit       | 2,048             |
| 1 MHz     | ~6-bit        | 64                |

For LEDs, 5 kHz at 8-bit resolution is a great default — no visible flicker and 256 brightness steps per channel (16.7 million total colors).

### Color Mixing

An RGB LED contains three tiny LEDs (red, green, blue) in one package. By mixing different brightness levels:

- Red (255) + Green (255) + Blue (0) = **Yellow**
- Red (0) + Green (255) + Blue (255) = **Cyan**
- Red (255) + Green (0) + Blue (255) = **Magenta**
- Red (255) + Green (255) + Blue (255) = **White**

### HSV Color Model

Rather than manually picking RGB values, the code uses HSV (Hue, Saturation, Value) to sweep through the rainbow. Hue is an angle on the color wheel (0-360 degrees), making it trivial to create smooth color transitions by incrementing the hue.

## Try It Out

1. After uploading, the LED should start cycling through rainbow colors smoothly.
2. Open the Serial Monitor at **115200 baud**.
3. You should see the current hue angle and RGB values being printed.
4. Type `255,0,0` and press Enter — the LED should turn solid red.
5. Type `0,255,0` — solid green.
6. Type `0,0,255` — solid blue.
7. Type `rainbow` to resume the rainbow cycle.

## Challenges

1. **Breathing Effect:** Modify the code to make a single color "breathe" — smoothly fade from off to full brightness and back, using a sine wave for a natural-looking curve.
2. **Custom Color Sequences:** Add serial commands to define a sequence of colors and transition times (e.g., `seq:255,0,0,1000;0,255,0,500;0,0,255,2000`) that loops automatically.
3. **Music Reactive (Advanced):** If you have the PDM mic working from the Sense board, use it to change LED colors based on sound amplitude.

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| LED doesn't light up | Wrong pin connections | Check wiring matches the diagram; verify the longest pin goes to GND |
| Only one color works | Resistor not connected on other channels | Verify all three resistors are properly seated |
| Colors look wrong | Common-anode LED instead of common-cathode | If using common-anode, connect the long pin to 3V3 instead of GND and invert the PWM values (255 - value) |
| LED flickers | PWM frequency too low | Increase `PWM_FREQ` to 5000 or higher |
| Serial commands don't work | Wrong baud rate | Set Serial Monitor to 115200 baud |
| Upload fails | Wrong board/port selected | Verify Tools > Board is XIAO_ESP32S3 and Port is /dev/cu.usbmodem101 |

## What's Next

In the next lesson, we'll read real-world data from a **temperature and humidity sensor** using the DHT22 and learn about digital communication protocols.

[Lesson 7: Temperature Sensor ->](../07-temperature-sensor/)
