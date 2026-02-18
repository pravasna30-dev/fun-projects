# Lesson 8: OLED Display (SSD1306)

**Difficulty:** Intermediate

**Estimated Time:** 60-90 minutes

---

## What You'll Learn

- How I2C (Inter-Integrated Circuit) communication works
- How to wire and control an SSD1306 128x64 OLED display
- How to install and use the Adafruit GFX and SSD1306 libraries
- How to draw text, shapes (lines, rectangles, circles), and simple animations
- How to build a real-time "dashboard" showing uptime and free memory
- How I2C addressing works and how to scan for devices

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Main microcontroller |
| SSD1306 OLED display (128x64, I2C) | 1 | 4-pin I2C version (VCC, GND, SCL, SDA) |
| Breadboard | 1 | For prototyping |
| Jumper wires | 4 | Male-to-female or male-to-male |
| USB-C cable | 1 | For programming and power |

## Circuit Diagram

```
    XIAO ESP32-S3                SSD1306 OLED (I2C)
   ┌─────────────┐             ┌──────────────────┐
   │            3V3├────────────┤ VCC              │
   │            GND├────────────┤ GND              │
   │         D5/SCL├────────────┤ SCL              │
   │         D4/SDA├────────────┤ SDA              │
   └─────────────┘             └──────────────────┘

   I2C Bus Topology:
   ┌──────┐     SDA ══════════════════ SDA ┌────────┐
   │ XIAO │     SCL ══════════════════ SCL │  OLED  │
   │ ESP32│     3V3 ══════════════════ VCC │SSD1306 │
   │  S3  │     GND ══════════════════ GND │128x64  │
   └──────┘                                └────────┘

   Note: Most SSD1306 modules have built-in pull-up resistors
   on the SDA and SCL lines. No external pull-ups needed.
```

> **Note:** The XIAO ESP32-S3 uses D4 as SDA and D5 as SCL by default for I2C. Some OLED modules label pins differently — make sure VCC goes to 3V3 and GND goes to GND.

## Step-by-Step Instructions

### Step 1: Install Required Libraries

You need two libraries from Adafruit:

1. Open Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...**
3. Search for **"Adafruit SSD1306"** — install it.
4. When prompted for dependencies, click **Install All** (this will install Adafruit GFX and Adafruit BusIO too).
5. Verify installation: Go to **Sketch > Include Library** and confirm you see both "Adafruit SSD1306" and "Adafruit GFX Library" listed.

**What these libraries do:**
- **Adafruit GFX** — A graphics "engine" that provides drawing functions (text, lines, shapes, bitmaps). It works with many different display types.
- **Adafruit SSD1306** — The hardware-specific driver that knows how to talk to the SSD1306 chip over I2C (or SPI). It translates GFX commands into the right bytes.

### Step 2: Wire the OLED

1. Connect the OLED's VCC to the XIAO's 3V3.
2. Connect GND to GND.
3. Connect the OLED's SCL to **D5** on the XIAO.
4. Connect the OLED's SDA to **D4** on the XIAO.

That's it — I2C only needs two data wires plus power.

### Step 3: Find the I2C Address

Most SSD1306 OLEDs use address **0x3C**, but some use **0x3D**. The sketch includes an I2C scan at startup that prints all detected devices — use this to verify.

### Step 4: Upload and Test

1. Open `oled-display.ino` in Arduino IDE.
2. Select **Tools > Board > XIAO_ESP32S3**.
3. Select **Tools > Port > /dev/cu.usbmodem101**.
4. Upload and open Serial Monitor at **115200 baud**.
5. You should see the I2C scan results, then the display should light up.

## The Code

### Display Initialization

```cpp
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
```

The constructor takes the pixel dimensions, a reference to the I2C bus (`Wire`), and the reset pin (-1 means no hardware reset pin). The address 0x3C is passed during `display.begin()`.

### Drawing Text

```cpp
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.println("Hello, World!");
display.display();
```

Key concept: All drawing functions write to an internal buffer (1024 bytes for 128x64 at 1-bit-per-pixel). Nothing appears on screen until you call `display.display()`, which sends the entire buffer over I2C. This avoids flickering — you compose the full frame first, then send it all at once.

### Drawing Shapes

The GFX library provides primitive shapes:
- `drawLine(x0, y0, x1, y1, color)` — a line between two points
- `drawRect(x, y, w, h, color)` — rectangle outline
- `fillRect(x, y, w, h, color)` — filled rectangle
- `drawCircle(x, y, r, color)` — circle outline
- `fillCircle(x, y, r, color)` — filled circle
- `drawTriangle(x0, y0, x1, y1, x2, y2, color)` — triangle outline

### The Dashboard

The sketch ends with a continuously updating dashboard that shows:
- Uptime in HH:MM:SS format
- Free heap memory (in bytes)
- A progress bar that fills over 60 seconds, then resets
- A small bouncing dot animation

## How It Works

### I2C Communication Protocol

I2C (pronounced "I-squared-C" or "I-two-C") is a two-wire serial protocol invented by Philips:

- **SDA (Serial Data):** Carries data in both directions
- **SCL (Serial Clock):** Clock signal generated by the master (the ESP32)

**How a transaction works:**
1. The master sends a **START condition** (SDA goes LOW while SCL is HIGH).
2. The master sends the **7-bit device address** plus a read/write bit.
3. The addressed device sends an **ACK** (acknowledgement) by pulling SDA LOW.
4. Data bytes are exchanged, each followed by an ACK.
5. The master sends a **STOP condition** (SDA goes HIGH while SCL is HIGH).

**I2C Addressing:**
Each device on the bus has a unique 7-bit address (0x00-0x7F). The SSD1306 typically uses 0x3C or 0x3D. You can have up to 127 devices on a single I2C bus — each with its own address. This is what makes I2C powerful for sensors: one bus, many devices, only two wires.

### How OLED Displays Work

OLED stands for **Organic Light-Emitting Diode**. Unlike LCDs, which need a backlight, each OLED pixel emits its own light:

- **True blacks:** Pixels that are "off" emit zero light, giving infinite contrast ratio.
- **No backlight needed:** Lower power consumption for dark content.
- **Wide viewing angles:** Light is emitted in all directions.
- **Fast response time:** Pixels switch in microseconds.

The SSD1306 is a **monochrome** OLED controller — each pixel is either ON (white/blue) or OFF (black). The display memory is organized as 8 pages of 128 columns, where each byte controls 8 vertical pixels. The total framebuffer is 128 x 64 / 8 = **1024 bytes**.

### Framebuffer Architecture

```
Page 0: [col0][col1][col2]...[col127]   <- each byte = 8 vertical pixels
Page 1: [col0][col1][col2]...[col127]
...
Page 7: [col0][col1][col2]...[col127]
```

When you call `display.display()`, the library sends all 1024 bytes over I2C. At the standard 400 kHz I2C speed, this takes about 20ms — fast enough for smooth animations at ~50 FPS.

## Try It Out

1. After uploading, the display should show a startup sequence:
   - "Hello XIAO!" text
   - Shape demos (rectangles, circles, lines)
   - A bouncing ball animation
   - Then the live dashboard
2. Watch the Serial Monitor for I2C scan results and debug info.
3. The dashboard updates every 100ms, showing real-time uptime and memory.
4. The progress bar fills over 60 seconds, then resets.

## Challenges

1. **Temperature Dashboard:** Combine this lesson with Lesson 7 (DHT22) — display temperature and humidity on the OLED instead of (or in addition to) the serial monitor. Draw a bar graph that shows the last 128 readings.
2. **Custom Bitmap:** Use an online tool like [image2cpp](https://javl.github.io/image2cpp/) to convert a small image to a byte array, then display it using `drawBitmap()`. Try making a splash screen logo for your project.
3. **Scrolling Text Marquee:** Implement a scrolling text banner that smoothly moves a long message across the screen pixel by pixel. Use `getTextBounds()` to calculate the total width.

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Display stays blank | Wrong I2C address | Check Serial Monitor for scan results; try 0x3D instead of 0x3C |
| Display stays blank | No power | Verify VCC/GND connections |
| Garbled display | Wrong screen dimensions | Confirm your display is 128x64, not 128x32 (adjust `SCREEN_HEIGHT` if needed) |
| "SSD1306 allocation failed" | Not enough memory | This shouldn't happen on ESP32-S3 with 8MB PSRAM; check that the library is current |
| I2C scan finds nothing | SDA/SCL swapped | Swap the D4 and D5 wires |
| Library not found | Not installed | Re-install Adafruit SSD1306 and Adafruit GFX from Library Manager |
| Flickering display | Updating too fast without clearDisplay | Always call clearDisplay() before redrawing, then display() once at the end |

## What's Next

With sensor reading and display skills under your belt, the next lesson will teach you how to **log data to an SD card** — creating a real data logger that saves readings even when the power is off.

[Lesson 9: Data Logger (SD Card) ->](../09-data-logger-sd-card/)
