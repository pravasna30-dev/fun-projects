/*
 * Lesson 7: Temperature Sensor (DHT22)
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Reads temperature and humidity from a DHT22 sensor every 2 seconds.
 * Displays formatted data on the Serial Monitor, including:
 *   - Temperature in Celsius and Fahrenheit
 *   - Relative humidity percentage
 *   - Computed heat index (feels-like temperature)
 *
 * Required Libraries (install via Library Manager):
 *   - "DHT sensor library" by Adafruit
 *   - "Adafruit Unified Sensor" by Adafruit
 *
 * Wiring:
 *   DHT22 pin 1 (VCC)  -> 3V3
 *   DHT22 pin 2 (DATA) -> D1 (with 10K pull-up to 3V3)
 *   DHT22 pin 3 (NC)   -> not connected
 *   DHT22 pin 4 (GND)  -> GND
 */

// ---------------------------------------------------------------------------
// Library Includes
// ---------------------------------------------------------------------------
// The DHT library handles the timing-sensitive protocol for reading
// the DHT22 sensor. It manages the start signal, bit-level decoding,
// and checksum verification internally.
#include "DHT.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define DHTPIN  D1       // GPIO pin connected to the DHT22 data line
#define DHTTYPE DHT22    // Sensor type: DHT22 (AM2302)
                         // Use DHT11 if you have that sensor instead

// Reading interval — the DHT22 requires at least 2 seconds between reads.
// Reading faster will either return stale data or NaN.
const unsigned long READ_INTERVAL_MS = 2000;

// ---------------------------------------------------------------------------
// Global Objects
// ---------------------------------------------------------------------------
// Create the DHT sensor object. The constructor takes the data pin
// and sensor type. Internally, it configures the pin and timing
// parameters for the specific sensor model.
DHT dht(DHTPIN, DHTTYPE);

// Track the reading number for the serial output
unsigned long readingCount = 0;

// ---------------------------------------------------------------------------
// setup() - Runs once at startup
// ---------------------------------------------------------------------------
void setup() {
  // Start serial communication at 115200 baud
  Serial.begin(115200);
  delay(1000);  // Allow serial monitor to connect

  Serial.println("==========================================");
  Serial.println("  Lesson 7: Temperature & Humidity Sensor");
  Serial.println("  DHT22 on XIAO ESP32-S3 Sense");
  Serial.println("==========================================");
  Serial.println();

  // Initialize the DHT sensor.
  // This sets the data pin mode and prepares internal timing variables.
  dht.begin();

  Serial.println("DHT22 sensor initialized.");
  Serial.printf("Data pin: D1 | Read interval: %lu ms\n", READ_INTERVAL_MS);
  Serial.println();
  Serial.println("Waiting for first reading...");
  Serial.println();
}

// ---------------------------------------------------------------------------
// loop() - Runs repeatedly after setup()
// ---------------------------------------------------------------------------
void loop() {
  // Use a non-blocking delay with millis() instead of delay().
  // This is a good practice — it keeps the loop responsive for
  // other tasks (like serial commands) while waiting.
  static unsigned long lastReadTime = 0;

  if (millis() - lastReadTime < READ_INTERVAL_MS) {
    return;  // Not time to read yet — exit early
  }
  lastReadTime = millis();

  // -----------------------------------------------------------------------
  // Read all sensor values
  // -----------------------------------------------------------------------
  // readHumidity() returns relative humidity as a percentage (0-100%).
  // readTemperature() returns temperature in Celsius by default.
  // readTemperature(true) returns temperature in Fahrenheit.
  //
  // Each call takes about 250ms as it waits for the full sensor response.
  float humidity = dht.readHumidity();
  float tempC    = dht.readTemperature();        // Celsius
  float tempF    = dht.readTemperature(true);     // Fahrenheit

  // -----------------------------------------------------------------------
  // Validate readings
  // -----------------------------------------------------------------------
  // The sensor returns NaN (Not a Number) if:
  //   - The wiring is incorrect (missing pull-up, wrong pin)
  //   - The sensor failed to respond within the expected timeframe
  //   - There's a checksum error (data corruption)
  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("[ERROR] Failed to read from DHT22 sensor!");
    Serial.println("  Check: Is the 10K pull-up resistor in place?");
    Serial.println("  Check: Is the sensor wired to the correct pin?");
    Serial.println();
    return;
  }

  // -----------------------------------------------------------------------
  // Calculate the heat index
  // -----------------------------------------------------------------------
  // The heat index (or "feels like" temperature) factors in humidity.
  // When humidity is high, the human body can't cool itself as efficiently
  // through sweat evaporation, so it "feels" hotter.
  //
  // Parameters: temperature, humidity, isFahrenheit
  // The third parameter (false) tells the function we're passing Celsius.
  float heatIndexC = dht.computeHeatIndex(tempC, humidity, false);
  float heatIndexF = dht.computeHeatIndex(tempF, humidity);  // Fahrenheit by default

  // -----------------------------------------------------------------------
  // Increment reading counter
  // -----------------------------------------------------------------------
  readingCount++;

  // -----------------------------------------------------------------------
  // Display formatted output
  // -----------------------------------------------------------------------
  Serial.println("╔══════════════════════════════════════╗");
  Serial.printf( "║   Reading #%-4lu          ", readingCount);
  printUptime();
  Serial.println("║");
  Serial.println("╠══════════════════════════════════════╣");
  Serial.printf( "║  Temperature: %6.1f °C  | %6.1f °F  ║\n", tempC, tempF);
  Serial.printf( "║  Humidity:    %6.1f %%               ║\n", humidity);
  Serial.printf( "║  Heat Index:  %6.1f °C  | %6.1f °F  ║\n", heatIndexC, heatIndexF);
  Serial.println("╠══════════════════════════════════════╣");

  // -----------------------------------------------------------------------
  // Comfort level indicator
  // -----------------------------------------------------------------------
  // Provide a human-readable comfort assessment based on temperature
  // and humidity ranges.
  Serial.print("║  Comfort:     ");
  if (tempC >= 20.0 && tempC <= 26.0 && humidity >= 30.0 && humidity <= 60.0) {
    Serial.println("Comfortable           ║");
  } else if (tempC < 18.0) {
    Serial.println("Too Cold              ║");
  } else if (tempC > 30.0) {
    Serial.println("Too Hot               ║");
  } else if (humidity > 70.0) {
    Serial.println("Too Humid             ║");
  } else if (humidity < 20.0) {
    Serial.println("Too Dry               ║");
  } else {
    Serial.println("Moderate              ║");
  }

  Serial.println("╚══════════════════════════════════════╝");
  Serial.println();
}

// ---------------------------------------------------------------------------
// Helper: printUptime
// Formats and prints the time since boot in HH:MM:SS format.
// ---------------------------------------------------------------------------
void printUptime() {
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours   = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  Serial.printf("%02lu:%02lu:%02lu ", hours, minutes, seconds);
}
