/*
 * Lesson 6: PWM & RGB LED
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch demonstrates Pulse Width Modulation (PWM) by controlling
 * a common-cathode RGB LED. It includes:
 *   - Smooth rainbow color cycling using HSV-to-RGB conversion
 *   - Serial command interface to set specific colors (e.g., "255,0,128")
 *   - Type "rainbow" in the Serial Monitor to resume the rainbow cycle
 *
 * Wiring:
 *   D1 -> 220 ohm -> Red pin of RGB LED
 *   D2 -> 220 ohm -> Green pin of RGB LED
 *   D3 -> 220 ohm -> Blue pin of RGB LED
 *   GND -> Common cathode (longest pin) of RGB LED
 *
 * PWM Configuration:
 *   Frequency:  5 kHz (no visible flicker)
 *   Resolution: 8-bit (0-255 brightness levels per channel)
 */

// ---------------------------------------------------------------------------
// Pin Definitions
// ---------------------------------------------------------------------------
const int RED_PIN   = D1;   // Red channel of the RGB LED
const int GREEN_PIN = D2;   // Green channel of the RGB LED
const int BLUE_PIN  = D3;   // Blue channel of the RGB LED

// ---------------------------------------------------------------------------
// PWM Configuration
// ---------------------------------------------------------------------------
// The ESP32-S3 LEDC peripheral lets us choose frequency and resolution.
// 5 kHz at 8-bit resolution is a solid default for LED control:
//   - Fast enough to avoid visible flicker
//   - 256 brightness levels per channel = 16.7 million color combinations
const int PWM_FREQ       = 5000;   // PWM frequency in Hz
const int PWM_RESOLUTION = 8;      // 8-bit resolution (0-255)

// ---------------------------------------------------------------------------
// Rainbow Fade Settings
// ---------------------------------------------------------------------------
float hue = 0.0;                    // Current hue angle (0.0 - 360.0)
const float HUE_STEP = 0.5;        // How much the hue advances per frame
const int FADE_DELAY_MS = 20;       // Delay between frames (ms) — controls speed
bool rainbowMode = true;            // Whether we're in rainbow mode or manual mode

// ---------------------------------------------------------------------------
// Function: setColor
// Sets the RGB LED to a specific color using PWM duty cycle values.
// Each parameter ranges from 0 (off) to 255 (full brightness).
// ---------------------------------------------------------------------------
void setColor(int red, int green, int blue) {
  // ledcWrite sends the duty cycle value to the pin.
  // The LEDC channel is automatically managed when using ledcAttach().
  ledcWrite(RED_PIN, red);
  ledcWrite(GREEN_PIN, green);
  ledcWrite(BLUE_PIN, blue);
}

// ---------------------------------------------------------------------------
// Function: hsvToRgb
// Converts a Hue-Saturation-Value color to Red-Green-Blue values.
//
// Parameters:
//   h - Hue angle in degrees (0.0 to 360.0)
//   s - Saturation (0.0 to 1.0) — we always use 1.0 for vivid colors
//   v - Value/brightness (0.0 to 1.0) — we always use 1.0 for max brightness
//   r, g, b - Output pointers for the 0-255 RGB values
//
// This uses the standard HSV-to-RGB algorithm, which divides the color
// wheel into six 60-degree sectors and interpolates linearly within each.
// ---------------------------------------------------------------------------
void hsvToRgb(float h, float s, float v, int *r, int *g, int *b) {
  float c = v * s;                       // Chroma
  float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));  // Intermediate value
  float m = v - c;                       // Match value (brightness offset)

  float rf, gf, bf;

  if (h < 60)       { rf = c; gf = x; bf = 0; }
  else if (h < 120) { rf = x; gf = c; bf = 0; }
  else if (h < 180) { rf = 0; gf = c; bf = x; }
  else if (h < 240) { rf = 0; gf = x; bf = c; }
  else if (h < 300) { rf = x; gf = 0; bf = c; }
  else              { rf = c; gf = 0; bf = x; }

  // Scale from 0.0-1.0 to 0-255 and add the brightness offset
  *r = (int)((rf + m) * 255);
  *g = (int)((gf + m) * 255);
  *b = (int)((bf + m) * 255);
}

// ---------------------------------------------------------------------------
// Function: handleSerialInput
// Checks the serial buffer for commands. Supports:
//   - "rainbow"       -> resume rainbow cycling
//   - "R,G,B"         -> set a specific color (e.g., "255,0,128")
// ---------------------------------------------------------------------------
void handleSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // Remove leading/trailing whitespace and newline

    // Check if the user typed "rainbow" to resume cycling
    if (input.equalsIgnoreCase("rainbow")) {
      rainbowMode = true;
      Serial.println("Resumed rainbow mode.");
      return;
    }

    // Otherwise, try to parse as "R,G,B"
    int commaIndex1 = input.indexOf(',');
    int commaIndex2 = input.indexOf(',', commaIndex1 + 1);

    // Validate that we found two commas
    if (commaIndex1 == -1 || commaIndex2 == -1) {
      Serial.println("Invalid format. Use: R,G,B (e.g., 255,0,128) or 'rainbow'");
      return;
    }

    // Extract each color component
    int r = input.substring(0, commaIndex1).toInt();
    int g = input.substring(commaIndex1 + 1, commaIndex2).toInt();
    int b = input.substring(commaIndex2 + 1).toInt();

    // Constrain values to valid range
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

    // Apply the color and switch to manual mode
    rainbowMode = false;
    setColor(r, g, b);
    Serial.printf("Set color: R=%d, G=%d, B=%d\n", r, g, b);
  }
}

// ---------------------------------------------------------------------------
// setup() - Runs once when the board powers on or resets
// ---------------------------------------------------------------------------
void setup() {
  // Initialize serial communication for debugging and commands
  Serial.begin(115200);
  delay(1000);  // Give the serial monitor time to connect

  Serial.println("=================================");
  Serial.println("  Lesson 6: PWM & RGB LED");
  Serial.println("  XIAO ESP32-S3 Sense");
  Serial.println("=================================");
  Serial.println();

  // Attach each LED pin to the LEDC PWM peripheral.
  // ledcAttach(pin, frequency, resolution) configures the hardware PWM
  // channel automatically and returns true on success.
  //
  // Unlike basic Arduinos, the ESP32 does NOT use analogWrite().
  // Instead, the LEDC peripheral generates rock-solid PWM signals
  // in hardware — no CPU overhead and perfectly stable timing.
  ledcAttach(RED_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(GREEN_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BLUE_PIN, PWM_FREQ, PWM_RESOLUTION);

  Serial.printf("PWM configured: %d Hz, %d-bit resolution\n", PWM_FREQ, PWM_RESOLUTION);
  Serial.printf("Pins: Red=D1, Green=D2, Blue=D3\n");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  R,G,B    -> Set a specific color (e.g., 255,0,128)");
  Serial.println("  rainbow  -> Resume rainbow cycle");
  Serial.println();

  // Start with LED off
  setColor(0, 0, 0);
}

// ---------------------------------------------------------------------------
// loop() - Runs repeatedly after setup()
// ---------------------------------------------------------------------------
void loop() {
  // Always check for serial commands
  handleSerialInput();

  // If we're in rainbow mode, advance the hue and update the LED
  if (rainbowMode) {
    int r, g, b;
    hsvToRgb(hue, 1.0, 1.0, &r, &g, &b);
    setColor(r, g, b);

    // Print the current color periodically (every ~1 second)
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 1000) {
      Serial.printf("Hue: %6.1f° -> R=%3d, G=%3d, B=%3d\n", hue, r, g, b);
      lastPrint = millis();
    }

    // Advance the hue angle around the color wheel
    hue += HUE_STEP;
    if (hue >= 360.0) {
      hue = 0.0;
    }

    delay(FADE_DELAY_MS);
  }
}
