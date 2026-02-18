# Lesson 2: Serial Communication

**Difficulty:** Beginner

**Estimated Time:** 25-35 minutes

---

## What You'll Learn

- What serial communication is and why it matters
- How to print text and variables to the Serial Monitor using `Serial.print()` and `Serial.println()`
- How to read user input from the Serial Monitor using `Serial.available()` and `Serial.read()`
- How to parse and respond to text commands
- How to format and display system information
- Why serial is the most important debugging tool in embedded development

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Same board from Lesson 1 |
| USB-C cable | 1 | Data-capable |
| Computer with Arduino IDE | 1 | Serial Monitor built in |

No extra hardware required.

---

## Step-by-Step Instructions

### 1. Open the Sketch

1. Open the Arduino IDE.
2. Open the file `serial-communication.ino` from this lesson folder.
3. Make sure your board is selected: **Tools > Board > XIAO_ESP32S3**.
4. Make sure your port is selected: **Tools > Port > /dev/cu.usbmodem101**.

### 2. Upload the Sketch

1. Click the **Upload** button (right-arrow icon).
2. Wait for "Done uploading" to appear.

### 3. Open the Serial Monitor

1. Click the **Serial Monitor** button (magnifying glass icon, top-right).
2. Set the baud rate to **115200** (dropdown at the bottom-right).
3. **Important:** Set the line ending dropdown to **"Newline"** or **"Both NL & CR"**. This ensures the board receives a newline character when you press Enter.

### 4. Try the Commands

Type each of these commands into the input field at the top of the Serial Monitor and press Enter:

1. `help` — Shows the list of available commands.
2. `on` — Turns the built-in LED on. Watch the board to confirm.
3. `off` — Turns the LED off.
4. `toggle` — Flips the LED to the opposite state.
5. `status` — Shows the current LED state and uptime.
6. `info` — Displays detailed system information about your ESP32-S3.
7. `banana` — Try an unknown command to see the error handling.

### 5. Observe the Output

Pay attention to how the board formats its responses. Notice the difference between `Serial.print()` (no newline) and `Serial.println()` (adds a newline). Look at how the `info` command uses `Serial.printf()` for formatted output.

---

## The Code

### Setting Up Serial Communication

```cpp
Serial.begin(115200);
```

This single line opens the serial port at 115200 baud. **Baud rate** is the speed of communication in bits per second. Both sides — the board and the Serial Monitor — must use the same baud rate. 115200 is the most common choice for ESP32 projects.

### Printing Text

The sketch uses several printing functions:

```cpp
Serial.print("LED: ");     // Prints text WITHOUT a newline at the end
Serial.println("ON");       // Prints text WITH a newline at the end
Serial.println(42);         // Prints a number
Serial.printf("%02lu:%02lu\n", min, sec);  // Formatted output (ESP32-specific)
```

- `print()` outputs text and keeps the cursor on the same line.
- `println()` outputs text and moves to a new line.
- `printf()` works like C's printf — lets you format numbers with padding, decimal places, etc.

### Reading User Input

```cpp
while (Serial.available() > 0) {
  char incomingChar = Serial.read();
  if (incomingChar == '\n' || incomingChar == '\r') {
    processCommand(inputBuffer);
    inputBuffer = "";
  } else {
    inputBuffer += incomingChar;
  }
}
```

- `Serial.available()` returns how many bytes are waiting in the receive buffer.
- `Serial.read()` reads and removes one byte from the buffer.
- Characters are accumulated in `inputBuffer` until a newline arrives, at which point we process the complete command.

### Processing Commands

```cpp
command.trim();
command.toLowerCase();
if (command == "on") { ... }
else if (command == "off") { ... }
```

- `trim()` removes whitespace from both ends of the string.
- `toLowerCase()` converts to lowercase so the user can type "ON", "On", or "on" and they all work.
- A chain of `if/else if` statements checks which command was typed.

### System Information

```cpp
Serial.println(ESP.getChipModel());
Serial.println(ESP.getFreeHeap());
Serial.println(ESP.getFlashChipSize() / 1024 / 1024);
```

The ESP32 Arduino library provides `ESP.*` functions that return hardware details. These are invaluable for debugging memory issues or verifying your board configuration.

---

## How It Works

### What is Serial Communication?

**Serial communication** is a way to send data one bit at a time over a wire. When you connect the XIAO ESP32-S3 via USB, a virtual serial port is created. The board sends bytes to your computer, and your computer sends bytes to the board, through this single USB cable.

Think of it like a text message conversation between your computer and the board.

### Baud Rate

Both sides must agree on a **baud rate** — the speed at which bits are sent. If the rates don't match, you'll see garbled text (random symbols). Common baud rates:

| Baud Rate | Use Case |
|-----------|----------|
| 9600 | Legacy, very slow |
| 115200 | Most common for ESP32 |
| 921600 | High speed, sometimes unstable |

### The Serial Buffer

When data arrives over serial, it goes into a **buffer** (a small area of memory). `Serial.available()` tells you how many bytes are waiting. `Serial.read()` pulls one byte out. If you don't read the buffer, it will eventually fill up and new data will be lost.

### Why Serial Matters for Debugging

Unlike programming on a computer where you can set breakpoints and step through code, on a microcontroller your primary debugging tool is **printing values to the Serial Monitor**. Whenever something isn't working:

1. Add `Serial.println()` statements to show variable values.
2. Add them at key points in your code to track the flow of execution.
3. Print "I reached this point" messages to find where things go wrong.

You will use serial debugging in every lesson from here on.

### Line Endings

When you press Enter in the Serial Monitor, what character gets sent? It depends on the line ending setting:

- **No line ending** — nothing extra is sent (your command won't be detected).
- **Newline (NL)** — sends `\n` (ASCII 10).
- **Carriage return (CR)** — sends `\r` (ASCII 13).
- **Both NL & CR** — sends `\r\n`.

Our code handles both `\n` and `\r`, so either setting works. But **"No line ending"** will not work because our code waits for a newline character to know the command is complete.

---

## Try It Out

1. Upload the sketch and open the Serial Monitor at 115200 baud.
2. Verify you see the welcome banner and help text.
3. Type `on` and press Enter. Confirm the LED turns on and the Serial Monitor says "LED turned ON."
4. Type `status` and press Enter. Check that the uptime increases each time you run it.
5. Type `info` and press Enter. Read through the system information. Note the chip model, core count, and memory sizes.
6. Try typing `ON` (uppercase). It should still work because the code converts to lowercase.
7. Try typing a command with extra spaces like `  off  `. It should still work because the code trims whitespace.

---

## Challenges

1. **Add a "blink" command.** When the user types `blink`, make the LED blink 5 times rapidly (100ms on, 100ms off), then return to its previous state. Hint: save `ledState` before blinking and restore it after.

2. **Add a "set" command for blink speed.** Accept a command like `set 500` that changes the blink delay to 500ms. You'll need to use `command.startsWith("set")` and `command.substring(4).toInt()` to parse the number.

3. **Add a command counter.** Keep a running count of how many commands have been processed and display it in the `status` output.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **Serial Monitor shows nothing** | Check that the baud rate is set to 115200. Press the Reset button on the board. |
| **Garbage/random characters appear** | Baud rate mismatch. Make sure both the code and the Serial Monitor are set to 115200. |
| **Commands are not recognized** | Set the line ending to **"Newline"** or **"Both NL & CR"** in the Serial Monitor dropdown. |
| **"on" works but "ON" doesn't** | This shouldn't happen — the code converts to lowercase. Make sure you uploaded the latest version. |
| **Serial Monitor is grayed out** | Close any other programs that might be using the serial port (other IDEs, terminal emulators, etc.). |

---

## What's Next

Now that you can communicate with your board and debug using serial output, the next lesson explores one of the ESP32's most powerful features: **Wi-Fi**. You'll scan for nearby wireless networks and display their details.

[Next: Lesson 3 - Wi-Fi Scanner](../03-wifi-scanner/)
