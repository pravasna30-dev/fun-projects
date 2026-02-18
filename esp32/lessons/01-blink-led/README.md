# Lesson 1: Blink LED

**Difficulty:** Beginner

**Estimated Time:** 20-30 minutes

---

## What You'll Learn

- How to open the Arduino IDE and set up your board for the first time
- What `setup()` and `loop()` do in every Arduino sketch
- How to configure a GPIO pin as an output using `pinMode()`
- How to turn an LED on and off using `digitalWrite()`
- How to use `delay()` to control timing
- How to upload a sketch to the XIAO ESP32-S3

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | The board has a built-in LED on pin 21 |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |

No breadboard, no wires, no external components. Just the board and a cable.

---

## Step-by-Step Instructions

### 1. Install the Arduino IDE

If you haven't already, download and install the Arduino IDE from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software). Version 2.x is recommended.

### 2. Add ESP32-S3 Board Support

1. Open Arduino IDE.
2. Go to **File > Preferences** (or **Arduino IDE > Settings** on macOS).
3. In the **Additional Boards Manager URLs** field, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Click **OK**.
5. Go to **Tools > Board > Boards Manager**.
6. Search for **esp32** and install the package by **Espressif Systems**.

### 3. Connect Your Board

1. Plug the XIAO ESP32-S3 into your computer using the USB-C cable.
2. Go to **Tools > Board** and select **XIAO_ESP32S3**.
3. Go to **Tools > Port** and select `/dev/cu.usbmodem101` (the exact name may vary slightly).

> **Tip:** If you don't see a port, try a different USB-C cable. Some cables are charge-only and don't carry data.

### 4. Create a New Sketch

1. Go to **File > New Sketch** (or press Cmd+N / Ctrl+N).
2. Delete any existing code in the editor.
3. Copy and paste the code from `blink-led.ino` into the editor.
4. Save the file (**File > Save**) with a meaningful name.

### 5. Upload the Sketch

1. Click the **Upload** button (the right-arrow icon in the top-left toolbar).
2. Wait for the IDE to compile the code. You'll see "Compiling sketch..." in the output panel.
3. The IDE will then upload the compiled code to your board. You'll see "Uploading..." followed by progress dots.
4. When you see **"Done uploading"**, the sketch is running on your board.

### 6. Open the Serial Monitor

1. Click the **Serial Monitor** button (the magnifying glass icon in the top-right).
2. Set the baud rate to **115200** (dropdown in the bottom-right of the Serial Monitor).
3. You should see messages like `LED is ON` and `LED is OFF` appearing every second.

---

## The Code

### Constants

```cpp
const int LED_PIN = 21;
const int BLINK_DELAY = 1000;
```

We define the LED pin number and the delay time as named constants. This makes the code easier to read and modify. If you ever want to change the blink speed, you only need to change it in one place.

### setup()

```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
}
```

- `Serial.begin(115200)` opens a communication channel between the board and your computer at 115200 baud (bits per second). This lets us send text messages to the Serial Monitor.
- `delay(1000)` gives the serial connection a moment to establish.
- `pinMode(LED_PIN, OUTPUT)` tells the board that pin 21 will be used to **send** voltage out (to drive the LED), not to read voltage in.

### loop()

```cpp
void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED is ON");
  delay(BLINK_DELAY);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED is OFF");
  delay(BLINK_DELAY);
}
```

- `digitalWrite(LED_PIN, HIGH)` sets pin 21 to 3.3 volts, turning the LED on.
- `digitalWrite(LED_PIN, LOW)` sets pin 21 to 0 volts, turning the LED off.
- `delay(BLINK_DELAY)` pauses the program for 1000 milliseconds (1 second).
- `Serial.println()` sends a line of text to the Serial Monitor so you can confirm the LED is doing what you expect.

---

## How It Works

### GPIO (General Purpose Input/Output)

The ESP32-S3 has many **GPIO pins** — these are the metal contacts on the edges of the board. Each pin can be configured as either an **input** (to read sensors or buttons) or an **output** (to drive LEDs, motors, etc.).

In this lesson, we use pin 21 as an output to control the built-in LED.

### HIGH and LOW

In digital electronics, there are only two states:

- **HIGH** = the pin outputs 3.3 volts (on the ESP32-S3). This is like turning a switch ON.
- **LOW** = the pin outputs 0 volts (ground). This is like turning a switch OFF.

When pin 21 goes HIGH, current flows through the LED and it lights up. When it goes LOW, no current flows and the LED turns off.

### The setup/loop Pattern

Every Arduino sketch has two required functions:

1. **`setup()`** runs once, right when the board powers on. Use it for one-time configuration.
2. **`loop()`** runs over and over, forever, after `setup()` finishes. This is where your main program logic goes.

The board automatically calls `setup()` first, then calls `loop()` repeatedly. You never need to call these functions yourself.

### What is a "Sketch"?

In the Arduino world, a program is called a **sketch**. It is a `.ino` file that contains C/C++ code. The Arduino IDE compiles this code and uploads the resulting binary to your board.

---

## Try It Out

1. After uploading, look at your XIAO ESP32-S3 board. You should see the built-in LED blinking on and off every second.
2. Open the Serial Monitor (set baud to 115200). You should see alternating `LED is ON` and `LED is OFF` messages.
3. Press the small **Reset** button on the board. The sketch restarts from `setup()` and you should see the startup banner again in the Serial Monitor.

---

## Challenges

1. **Change the blink speed.** Modify `BLINK_DELAY` to 250 (quarter second) or 2000 (two seconds). Upload again and observe the difference.

2. **Create an SOS pattern.** In Morse code, SOS is `... --- ...` (three short blinks, three long blinks, three short blinks). Modify the `loop()` function to blink this pattern, then pause before repeating.

3. **Asymmetric blinking.** Make the LED stay ON for 200ms and OFF for 800ms. This creates a brief "heartbeat" flash effect.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **No port appears in Tools > Port** | Try a different USB-C cable. Many cables are charge-only. You need a data cable. |
| **Upload fails with "Failed to connect"** | Hold the **BOOT** button on the board, click Upload, then release BOOT after you see "Connecting..." in the output. |
| **Serial Monitor shows garbage characters** | Make sure the baud rate in the Serial Monitor matches the code: **115200**. |
| **LED doesn't blink but upload succeeded** | Double-check that `LED_PIN` is set to `21`. On some board variants, the pin number may differ. |
| **"Board not found" error** | Make sure you installed the ESP32 board package (step 2) and selected **XIAO_ESP32S3** under Tools > Board. |

---

## What's Next

Now that you can control an LED and upload code, the next lesson introduces **Serial Communication** — the primary way to send data between your board and computer, and the most important debugging tool you'll use in every future project.

[Next: Lesson 2 - Serial Communication](../02-serial-communication/)
