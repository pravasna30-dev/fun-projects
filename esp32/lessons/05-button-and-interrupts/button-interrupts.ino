/*
 * Lesson 5: Button & Interrupts
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch demonstrates:
 *   - Reading a physical push button
 *   - Using hardware interrupts instead of polling
 *   - Software debouncing to handle contact bounce
 *   - Toggling the built-in LED on each button press
 *   - Counting button presses
 *
 * Hardware required:
 *   - 1 push button connected between pin D1 (GPIO 2) and GND
 *   - No resistor needed — we use the ESP32's internal pull-up resistor
 *
 * Wiring:
 *   Button Pin 1 ---> D1 (GPIO 2)
 *   Button Pin 2 ---> GND
 */

// ---------- CONSTANTS ----------

// The pin connected to the push button.
// D1 on the XIAO ESP32-S3 corresponds to GPIO 2.
const int BUTTON_PIN = 2;

// The built-in LED pin.
const int LED_PIN = 21;

// Debounce time in milliseconds.
// Mechanical buttons "bounce" — when pressed, the metal contacts
// rapidly connect and disconnect for a few milliseconds before
// settling. This creates multiple false triggers. We ignore any
// signals that arrive within this window after the first one.
const unsigned long DEBOUNCE_DELAY = 200;

// ---------- VOLATILE VARIABLES ----------
// Variables shared between the interrupt service routine (ISR) and
// the main code MUST be declared "volatile". This tells the compiler
// that the variable can change at any time (from the interrupt),
// so it should always read it from memory rather than using a cached copy.

// Flag set by the interrupt to signal the main loop.
volatile bool buttonPressed = false;

// Timestamp of the last valid button press (for debouncing).
volatile unsigned long lastPressTime = 0;

// ---------- REGULAR VARIABLES ----------

// Track the LED state (on or off).
bool ledState = false;

// Count total button presses.
unsigned long pressCount = 0;

// ---------- INTERRUPT SERVICE ROUTINE (ISR) ----------
// This function is called AUTOMATICALLY by the hardware whenever
// the button pin changes state. It runs immediately, interrupting
// whatever the main code is doing.
//
// IMPORTANT rules for ISRs:
//   1. Keep them as SHORT as possible.
//   2. Don't use Serial.print() inside an ISR.
//   3. Don't use delay() inside an ISR.
//   4. Only modify "volatile" variables.
//   5. Use IRAM_ATTR to place the function in RAM for fast execution.

void IRAM_ATTR handleButtonInterrupt() {
  // Get the current time.
  unsigned long currentTime = millis();

  // Check if enough time has passed since the last press (debounce).
  if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
    // This is a valid press — set the flag for the main loop to handle.
    buttonPressed = true;
    lastPressTime = currentTime;
  }
  // If not enough time has passed, we ignore this trigger (it's bounce).
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure the LED pin as output.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Configure the button pin as input with the INTERNAL PULL-UP resistor.
  //
  // INPUT_PULLUP means:
  //   - The pin is an input (reads voltage).
  //   - An internal resistor "pulls" the pin to HIGH (3.3V) by default.
  //   - When the button is pressed, it connects the pin to GND (LOW).
  //   - So: NOT pressed = HIGH, Pressed = LOW.
  //
  // This is why we don't need an external resistor — the ESP32 has one built in.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach a hardware interrupt to the button pin.
  //
  // attachInterrupt() takes three arguments:
  //   1. digitalPinToInterrupt(pin) — converts pin number to interrupt number.
  //   2. ISR function — the function to call when the interrupt fires.
  //   3. Trigger mode — when to fire the interrupt:
  //        FALLING = when the pin goes from HIGH to LOW (button pressed).
  //        RISING  = when the pin goes from LOW to HIGH (button released).
  //        CHANGE  = on any change (both press and release).
  //
  // We use FALLING because with INPUT_PULLUP, pressing the button
  // pulls the pin from HIGH (3.3V) down to LOW (GND).
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);

  // Print startup banner.
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Lesson 5: Button & Interrupts");
  Serial.println("  Board: XIAO ESP32-S3 Sense");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Press the button connected to D1 (GPIO 2).");
  Serial.println("Each press toggles the LED and increments the counter.");
  Serial.println();
  Serial.println("Button wiring:");
  Serial.println("  Button Pin 1 -> D1 (GPIO 2)");
  Serial.println("  Button Pin 2 -> GND");
  Serial.println();
}

// ---------- LOOP ----------

void loop() {
  // Check if the interrupt has signaled a button press.
  // We do NOT put the logic directly in the ISR because:
  //   - ISRs should be very short.
  //   - We can't use Serial.print() in an ISR.
  //   - Complex logic in ISRs can cause crashes.
  //
  // Instead, the ISR sets a flag, and we handle it here in the main loop.

  if (buttonPressed) {
    // Reset the flag.
    buttonPressed = false;

    // Toggle the LED state.
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    // Increment the press counter.
    pressCount++;

    // Print the result.
    Serial.print("Button press #");
    Serial.print(pressCount);
    Serial.print(" | LED is now: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  // The main loop runs continuously, but most of the time there's
  // nothing to do. The interrupt handles the timing-critical part
  // (detecting the button press), and the main loop handles the
  // response (toggling the LED and printing).
  //
  // This is much better than polling (checking the button in a loop)
  // because:
  //   1. We never miss a press, even if the main loop is busy.
  //   2. The CPU isn't wasting time constantly checking the pin.
  //   3. The response is nearly instantaneous.
}
