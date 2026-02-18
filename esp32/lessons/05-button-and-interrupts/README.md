# Lesson 5: Button & Interrupts

**Difficulty:** Beginner-Intermediate

**Estimated Time:** 30-40 minutes

---

## What You'll Learn

- How to read a physical push button using `digitalRead()`
- What pull-up and pull-down resistors are and why they matter
- How to use the ESP32's internal pull-up resistor with `INPUT_PULLUP`
- What hardware interrupts are and why they're better than polling
- How to attach an interrupt with `attachInterrupt()`
- What the `volatile` keyword does and when to use it
- How to debounce a button in software to avoid false triggers

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Built-in LED on pin 21 |
| USB-C cable | 1 | Data-capable |
| Push button (momentary) | 1 | Any standard tactile push button |
| Breadboard | 1 | For making connections |
| Jumper wires | 2 | Male-to-male |
| Computer with Arduino IDE | 1 | |

---

## Circuit Diagram

```
                XIAO ESP32-S3 Sense
               +-------------------+
               |                   |
               |  D1 (GPIO 2)  o---+----+
               |                   |    |
               |           GND o---+--+ |
               |                   |  | |
               +-------------------+  | |
                                      | |
                                      | |
            Push Button               | |
            +---------+               | |
            |         |               | |
            |  ( O )  |               | |
            |         |               | |
            +----+----+               | |
                 |    |               | |
                 |    +---------------+ |
                 |         (to GND)     |
                 +-----------------------+
                           (to D1)


    Breadboard View:
    ================

    Row 1:  [XIAO D1] ---wire---> [Button Pin 1]
    Row 2:  [XIAO GND] ---wire--> [Button Pin 2]

    That's it! Just two wires.

    How the button sits on a breadboard:

         Column A    Column B
           |            |
    Row 5  +--[BUTTON]--+    <-- button bridges the center gap
           |            |

    Wire from D1  --> Row 5, Column A
    Wire from GND --> Row 5, Column B
```

> **Note:** No external resistor is needed. We use the ESP32's built-in pull-up resistor (enabled in software with `INPUT_PULLUP`).

---

## Step-by-Step Instructions

### 1. Wire the Button

1. Place the push button on the breadboard so it bridges the center gap.
2. Connect one side of the button to **D1 (GPIO 2)** on the XIAO using a jumper wire.
3. Connect the other side of the button to **GND** on the XIAO using a jumper wire.
4. That's it — just two wires. No resistor needed.

### 2. Upload the Sketch

1. Open `button-interrupts.ino` in the Arduino IDE.
2. Select **XIAO_ESP32S3** and port `/dev/cu.usbmodem101`.
3. Click **Upload**.

### 3. Open the Serial Monitor

1. Open the Serial Monitor at **115200** baud.
2. You should see the startup banner and wiring instructions.

### 4. Press the Button

1. Press the button once. You should see:
   ```
   Button press #1 | LED is now: ON
   ```
2. Press it again:
   ```
   Button press #2 | LED is now: OFF
   ```
3. Each press toggles the LED and increments the counter.
4. Try pressing rapidly — the debouncing code should prevent double-counts.

---

## The Code

### Configuring the Button Pin

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

This does two things:
1. Sets the pin as an **input** (reads voltage rather than outputting it).
2. Activates the **internal pull-up resistor**, which holds the pin at HIGH (3.3V) when nothing is connected. When the button is pressed, it connects the pin to GND, pulling it LOW.

### Attaching the Interrupt

```cpp
attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
```

This tells the hardware: "When pin D1 goes from HIGH to LOW (FALLING edge), immediately call `handleButtonInterrupt()`." The function runs instantly, no matter what the main `loop()` is doing.

### The Interrupt Service Routine (ISR)

```cpp
void IRAM_ATTR handleButtonInterrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
    buttonPressed = true;
    lastPressTime = currentTime;
  }
}
```

- `IRAM_ATTR` places this function in the ESP32's fast internal RAM so it executes as quickly as possible.
- The debounce check ignores triggers that happen within 200ms of the last valid press.
- It only sets a flag — the actual LED toggling happens in `loop()`.

### The volatile Keyword

```cpp
volatile bool buttonPressed = false;
volatile unsigned long lastPressTime = 0;
```

`volatile` tells the compiler: "Don't optimize this variable. Always read it from memory." Without `volatile`, the compiler might cache the variable's value in a CPU register, and the main code would never see the change made by the interrupt.

### Handling the Press in loop()

```cpp
if (buttonPressed) {
  buttonPressed = false;
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  pressCount++;
  Serial.print("Button press #");
  Serial.println(pressCount);
}
```

The main loop checks the flag, resets it, toggles the LED, and prints the result. This separation keeps the ISR fast and lets us safely use `Serial.print()`.

---

## How It Works

### Pull-Up vs. Pull-Down Resistors

When a pin is configured as an input, it needs to be in a **known state** at all times. Without a resistor, a disconnected input pin "floats" — it picks up random electrical noise and gives unpredictable readings.

**Pull-up resistor:** Connects the pin to 3.3V through a resistor. The pin reads HIGH by default. Pressing the button (which connects the pin to GND) makes it read LOW.

```
    3.3V
     |
    [R] <-- Pull-up resistor (internal, ~45kOhm)
     |
     +--- Pin (reads HIGH normally)
     |
   [Button]
     |
    GND      (pressing button pulls pin to LOW)
```

**Pull-down resistor:** Connects the pin to GND through a resistor. The pin reads LOW by default. Pressing the button (connected to 3.3V) makes it read HIGH.

We use **INPUT_PULLUP** because it's simpler — no external resistor needed.

### What is an Interrupt?

Think of it like a doorbell. You have two ways to know if someone is at the door:

1. **Polling:** Walk to the door every 10 seconds and check. You might miss someone, and you waste a lot of time walking back and forth.
2. **Interrupt (doorbell):** Go about your business. When someone rings the bell, you immediately know.

Hardware interrupts work the same way. Instead of constantly checking the button in `loop()` (polling), we tell the hardware: "Watch this pin. When it changes, interrupt whatever the CPU is doing and call this function."

### Why Interrupts Beat Polling

| Aspect | Polling | Interrupts |
|--------|---------|------------|
| Responsiveness | Depends on loop speed | Nearly instant |
| CPU usage | Wastes cycles checking | Only runs when needed |
| Missed events | Possible if loop is slow | Impossible (hardware watches the pin) |
| Code complexity | Simpler | Slightly more complex |

### What is Debouncing?

When you press a mechanical button, the metal contacts don't make a clean single connection. They **bounce** — rapidly connecting and disconnecting for a few milliseconds:

```
    Ideal button press:        Real button press:

    HIGH ____                  HIGH ____
              |                         |_|‾|_|‾|_
    LOW       |____            LOW                 |______

         (clean)                  (bouncy!)
```

Without debouncing, a single press might register as 3-10 presses. Our debounce code ignores any triggers within 200ms of the last valid one, filtering out the bounce.

### IRAM_ATTR Explained

On the ESP32, code normally runs from flash memory (which is slower to access). The `IRAM_ATTR` attribute tells the compiler to place the interrupt function in the ESP32's internal RAM (IRAM), which is much faster. This is required for ISR functions on the ESP32 to avoid crashes caused by slow flash access during an interrupt.

---

## Try It Out

1. Upload the sketch and open the Serial Monitor.
2. Press the button. Verify the LED toggles and the counter increments.
3. Press rapidly. Each physical press should register exactly once (not double-count).
4. Hold the button down. Only one press should register.
5. Watch the counter climb. It should be perfectly accurate.
6. Try disconnecting the button wire from D1. The pin floats, but the pull-up resistor prevents false triggers.

---

## Challenges

1. **Add a long-press feature.** Track how long the button is held down. If held for more than 2 seconds, trigger a different action (e.g., blink the LED 5 times rapidly). Hint: record `millis()` on FALLING and check elapsed time on RISING.

2. **Implement double-click detection.** If two presses occur within 400ms, treat it as a "double click" and trigger a different behavior (e.g., start a rapid blink pattern). Single clicks still toggle normally.

3. **Add a second button.** Connect another button to D2 (GPIO 3) and add a second interrupt. One button turns the LED on, the other turns it off. Print which button was pressed.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **Button press not detected** | Check your wiring. One button pin goes to D1, the other to GND. Try reversing them — tactile buttons have two pairs of connected pins. |
| **LED toggles randomly without pressing** | Your wiring may be loose, causing the pin to float. Make sure the connections are solid. Check that `INPUT_PULLUP` is in your code. |
| **Each press registers 2-3 times** | Increase `DEBOUNCE_DELAY` from 200 to 300 or even 500. Some buttons bounce more than others. |
| **LED doesn't toggle but serial shows presses** | Verify `LED_PIN` is 21. Check that `pinMode(LED_PIN, OUTPUT)` is in `setup()`. |
| **Nothing shows in Serial Monitor** | Check baud rate is 115200. Try pressing the Reset button on the board. |
| **Sketch won't compile: "handleButtonInterrupt was not declared"** | Make sure the ISR function is defined before `setup()` in the code, or add a forward declaration. |

---

## What's Next

You've now covered the fundamentals: digital output (LED), serial communication, Wi-Fi networking, web servers, and digital input (buttons with interrupts). These building blocks form the foundation for nearly every ESP32 project. From here, you can combine these concepts to build increasingly sophisticated projects — sensor networks, home automation systems, IoT dashboards, and more.

Continue exploring the XIAO ESP32-S3 Sense's unique hardware: the OV2640 camera, PDM microphone, and SD card slot open up possibilities for image capture, audio recording, and data logging projects.
