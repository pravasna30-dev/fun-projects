/*
 * Lesson 2: Serial Communication
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch demonstrates how to use the Serial Monitor to:
 *   - Print formatted text and data to your computer
 *   - Read commands typed by the user
 *   - Control the built-in LED by typing "on" or "off"
 *   - Display system information (uptime, free memory, etc.)
 *
 * Serial communication is the #1 debugging tool for embedded development.
 * You will use it in every single project going forward.
 *
 * No extra hardware required — just the board and a USB-C cable.
 */

// ---------- CONSTANTS ----------

// Built-in LED on the XIAO ESP32-S3 is on GPIO 21.
const int LED_PIN = 21;

// We'll track the LED state so we can report it.
bool ledState = false;

// A buffer to accumulate incoming serial characters into a complete command.
String inputBuffer = "";

// ---------- SETUP ----------

void setup() {
  // Initialize serial communication at 115200 baud.
  // Baud rate = bits per second. Both sides (board and computer)
  // must agree on the same baud rate to understand each other.
  Serial.begin(115200);

  // Wait for the serial port to be ready.
  // On some boards, the USB serial port takes a moment to initialize.
  delay(1000);

  // Configure the LED pin as output.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off.

  // Print a welcome banner.
  // println() adds a newline at the end; print() does not.
  printBanner();
  printHelp();
}

// ---------- LOOP ----------

void loop() {
  // Check if any data has arrived over the serial connection.
  // Serial.available() returns the number of bytes waiting to be read.
  while (Serial.available() > 0) {
    // Read one character at a time from the serial buffer.
    char incomingChar = Serial.read();

    // If the user pressed Enter (newline character), process the command.
    // Different systems send different line endings:
    //   '\n' = newline (Line Feed, LF)
    //   '\r' = carriage return (CR)
    // We'll treat both as "end of command".
    if (incomingChar == '\n' || incomingChar == '\r') {
      // Only process if there's actually something in the buffer.
      // (Ignore empty lines caused by CR+LF line endings sending two characters.)
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";  // Clear the buffer for the next command.
      }
    } else {
      // Append the character to our buffer, building up the command.
      inputBuffer += incomingChar;
    }
  }
}

// ---------- COMMAND PROCESSOR ----------
// This function takes a command string and performs the appropriate action.

void processCommand(String command) {
  // Convert to lowercase so "ON", "On", and "on" all work.
  command.trim();        // Remove any leading/trailing whitespace.
  command.toLowerCase(); // Convert to lowercase for easy comparison.

  Serial.print("> ");    // Echo the command back, like a terminal prompt.
  Serial.println(command);

  // --- Command: on ---
  if (command == "on") {
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
    Serial.println("LED turned ON.");
  }
  // --- Command: off ---
  else if (command == "off") {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
    Serial.println("LED turned OFF.");
  }
  // --- Command: toggle ---
  else if (command == "toggle") {
    ledState = !ledState;  // Flip the boolean: true becomes false, false becomes true.
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.print("LED toggled. Now: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
  // --- Command: status ---
  else if (command == "status") {
    printStatus();
  }
  // --- Command: info ---
  else if (command == "info") {
    printSystemInfo();
  }
  // --- Command: help ---
  else if (command == "help") {
    printHelp();
  }
  // --- Unknown command ---
  else {
    Serial.print("Unknown command: '");
    Serial.print(command);
    Serial.println("'");
    Serial.println("Type 'help' for a list of commands.");
  }

  Serial.println();  // Blank line for readability.
}

// ---------- HELPER FUNCTIONS ----------

// Print the startup banner with board information.
void printBanner() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Lesson 2: Serial Communication");
  Serial.println("  Board: XIAO ESP32-S3 Sense");
  Serial.println("========================================");
  Serial.println();
}

// Print the list of available commands.
void printHelp() {
  Serial.println("Available commands:");
  Serial.println("  on      - Turn the LED on");
  Serial.println("  off     - Turn the LED off");
  Serial.println("  toggle  - Toggle the LED state");
  Serial.println("  status  - Show the current LED state");
  Serial.println("  info    - Show system information");
  Serial.println("  help    - Show this help message");
  Serial.println();
  Serial.println("Type a command and press Enter.");
  Serial.println();
}

// Print the current LED status.
void printStatus() {
  Serial.println("----- STATUS -----");
  Serial.print("LED (pin ");
  Serial.print(LED_PIN);
  Serial.print("): ");
  Serial.println(ledState ? "ON" : "OFF");

  // millis() returns the number of milliseconds since the board started.
  unsigned long uptime = millis();
  Serial.print("Uptime: ");
  Serial.print(uptime / 1000);
  Serial.println(" seconds");
  Serial.println("------------------");
}

// Print detailed system information.
// This demonstrates various Serial.print formatting techniques.
void printSystemInfo() {
  Serial.println("----- SYSTEM INFO -----");

  // Print the chip model and revision.
  Serial.print("Chip Model:    ");
  Serial.println(ESP.getChipModel());

  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());

  Serial.print("CPU Cores:     ");
  Serial.println(ESP.getChipCores());

  // Print CPU frequency in MHz.
  Serial.print("CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  // Print memory information.
  // ESP.getFreeHeap() returns available RAM in bytes.
  Serial.print("Free Heap:     ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

  // Total heap size.
  Serial.print("Total Heap:    ");
  Serial.print(ESP.getHeapSize());
  Serial.println(" bytes");

  // Flash chip size.
  Serial.print("Flash Size:    ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");

  // PSRAM (external SPI RAM) — the XIAO ESP32-S3 Sense has 8MB.
  Serial.print("PSRAM Size:    ");
  Serial.print(ESP.getPsramSize() / 1024 / 1024);
  Serial.println(" MB");

  Serial.print("Free PSRAM:    ");
  Serial.print(ESP.getFreePsram() / 1024 / 1024);
  Serial.println(" MB");

  // SDK version.
  Serial.print("SDK Version:   ");
  Serial.println(ESP.getSdkVersion());

  // Uptime using formatted output.
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  Serial.print("Uptime:        ");
  // Use a formatted string. Arduino doesn't have printf by default,
  // but the ESP32 supports it.
  Serial.printf("%02lu:%02lu:%02lu\n", hours, minutes, seconds);

  Serial.println("-----------------------");
}
