/*
 * Lesson 1: Blink LED
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This is the classic "Hello World" of electronics.
 * We will blink the built-in LED on the XIAO ESP32-S3,
 * which is connected to GPIO pin 21.
 *
 * What this sketch does:
 *   - Turns the built-in LED ON for 1 second
 *   - Turns the built-in LED OFF for 1 second
 *   - Repeats forever
 *   - Prints the LED state to the Serial Monitor
 *
 * No extra hardware required — just the board and a USB-C cable.
 */

// ---------- CONSTANTS ----------

// The built-in LED on the XIAO ESP32-S3 is connected to GPIO pin 21.
// We give it a meaningful name so the code is easy to read.
const int LED_PIN = 21;

// How long the LED stays ON or OFF, in milliseconds.
// 1000 milliseconds = 1 second.
const int BLINK_DELAY = 1000;

// ---------- SETUP ----------
// The setup() function runs ONCE when the board powers on or resets.
// Use it to configure pins and initialize communication.

void setup() {
  // Start serial communication at 115200 baud (bits per second).
  // This lets us send text from the board to your computer,
  // which you can view in the Arduino IDE Serial Monitor.
  Serial.begin(115200);

  // Wait a brief moment for the serial connection to establish.
  // This is helpful so you don't miss the first few messages.
  delay(1000);

  // Configure the LED pin as an OUTPUT.
  // OUTPUT means the pin will send voltage out (to drive the LED).
  // The other option is INPUT, which reads voltage coming in.
  pinMode(LED_PIN, OUTPUT);

  // Print a startup message to the Serial Monitor.
  Serial.println("========================================");
  Serial.println("  Lesson 1: Blink LED");
  Serial.println("  Board: XIAO ESP32-S3 Sense");
  Serial.println("========================================");
  Serial.println();
  Serial.println("The built-in LED on pin 21 will now blink.");
  Serial.println("Open the Serial Monitor to see status messages.");
  Serial.println();
}

// ---------- LOOP ----------
// The loop() function runs REPEATEDLY, forever, after setup() finishes.
// This is where the main action happens.

void loop() {
  // --- Turn the LED ON ---
  // digitalWrite() sets a pin to HIGH (3.3V) or LOW (0V).
  // HIGH sends voltage to the LED, turning it ON.
  digitalWrite(LED_PIN, HIGH);

  // Print the current state to the Serial Monitor.
  Serial.println("LED is ON");

  // Pause for the specified delay (1 second).
  // During delay(), the board does nothing — it just waits.
  delay(BLINK_DELAY);

  // --- Turn the LED OFF ---
  // LOW removes voltage from the pin, turning the LED OFF.
  digitalWrite(LED_PIN, LOW);

  // Print the current state to the Serial Monitor.
  Serial.println("LED is OFF");

  // Pause again before the loop repeats.
  delay(BLINK_DELAY);
}
