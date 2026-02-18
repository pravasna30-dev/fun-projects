/*
 * Lesson 15: Deep Sleep & Battery
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Demonstrates deep sleep with timer and GPIO wake-up sources,
 * RTC memory persistence, boot count tracking, and wake reason detection.
 *
 * Wake-up sources:
 *   - Timer: wakes every 30 seconds
 *   - GPIO: pin 1 (D0) pulled LOW by a button (optional)
 *
 * RTC memory stores:
 *   - Boot count (incremented each wake-up)
 *   - Total awake time across all cycles
 *   - Last sleep duration configured
 */

#include <esp_sleep.h>

// ─── Configuration ──────────────────────────────────────────────────────────

// Sleep duration in seconds
#define SLEEP_DURATION_SEC     30

// GPIO pin for external wake-up (D0 on XIAO ESP32-S3)
// Connect a button between this pin and GND (internal pull-up enabled)
#define WAKEUP_GPIO_PIN        1

// Built-in LED
#define LED_PIN                21

// How long to stay awake before sleeping (seconds)
#define AWAKE_DURATION_SEC     5

// ─── RTC Memory Variables ───────────────────────────────────────────────────
// These variables are stored in the RTC slow memory domain.
// They retain their values across deep sleep cycles (but NOT across
// power cycles or hard resets).

RTC_DATA_ATTR int      bootCount = 0;
RTC_DATA_ATTR uint32_t totalAwakeTimeMs = 0;
RTC_DATA_ATTR uint32_t lastSleepDurationSec = 0;

// ─── Get Wake-Up Reason as String ───────────────────────────────────────────

String getWakeupReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      return "External (ext0) - GPIO pin";
    case ESP_SLEEP_WAKEUP_EXT1:
      return "External (ext1) - GPIO mask";
    case ESP_SLEEP_WAKEUP_TIMER:
      return "Timer";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      return "Touchpad";
    case ESP_SLEEP_WAKEUP_ULP:
      return "ULP coprocessor";
    default:
      return "Power-on / Reset";
  }
}

// ─── Blink LED to Show We Are Awake ─────────────────────────────────────────

void blinkLED(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) {
      delay(offMs);
    }
  }
}

// ─── Print System Information ───────────────────────────────────────────────

void printSystemInfo() {
  Serial.println("--- System Info ---");
  Serial.printf("  CPU Frequency:     %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("  Free Heap:         %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Uptime:            %lu ms\n", millis());

  if (psramFound()) {
    Serial.printf("  PSRAM Total:       %u bytes\n", ESP.getPsramSize());
    Serial.printf("  PSRAM Free:        %u bytes\n", ESP.getFreePsram());
  }

  Serial.println();
}

// ─── Print RTC Memory State ─────────────────────────────────────────────────

void printRTCState() {
  Serial.println("--- RTC Memory State ---");
  Serial.printf("  Boot count:          %d\n", bootCount);
  Serial.printf("  Total awake time:    %u ms (%.1f sec)\n",
                totalAwakeTimeMs, totalAwakeTimeMs / 1000.0);

  if (lastSleepDurationSec > 0) {
    Serial.printf("  Last sleep duration: %u sec\n", lastSleepDurationSec);
  }

  // Estimate total elapsed time (awake time + sleep time)
  if (bootCount > 1) {
    uint32_t estimatedTotalSec = (totalAwakeTimeMs / 1000) +
                                  (bootCount - 1) * lastSleepDurationSec;
    Serial.printf("  Est. total elapsed:  %u sec (%.1f min)\n",
                  estimatedTotalSec, estimatedTotalSec / 60.0);
  }

  Serial.println();
}

// ─── Configure Wake-Up Sources ──────────────────────────────────────────────

void configureWakeup() {
  Serial.println("Configuring wake-up sources:");

  // Source 1: Timer wake-up
  // The parameter is in microseconds: seconds * 1,000,000
  uint64_t sleepTimeUs = (uint64_t)SLEEP_DURATION_SEC * 1000000ULL;
  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  Serial.printf("  Timer: %d seconds\n", SLEEP_DURATION_SEC);

  // Source 2: External wake-up on GPIO pin (ext0)
  // ext0 monitors a single RTC GPIO and wakes on a specified level.
  // We use LOW level (button pressed connects to GND).
  // First, configure the pin with an internal pull-up so it reads HIGH
  // when the button is not pressed.
  pinMode(WAKEUP_GPIO_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_GPIO_PIN, LOW);
  Serial.printf("  GPIO:  Pin %d (LOW to wake)\n", WAKEUP_GPIO_PIN);

  Serial.println();
}

// ─── Enter Deep Sleep ───────────────────────────────────────────────────────

void enterDeepSleep() {
  // Record the time we spent awake in this cycle
  uint32_t awakeTime = millis();
  totalAwakeTimeMs += awakeTime;
  lastSleepDurationSec = SLEEP_DURATION_SEC;

  Serial.printf("Awake time this cycle: %lu ms\n", awakeTime);
  Serial.println();

  // Countdown before sleeping (gives time to read serial output)
  Serial.printf("Entering deep sleep in %d seconds...\n", AWAKE_DURATION_SEC);
  Serial.print("  ");
  for (int i = AWAKE_DURATION_SEC; i > 0; i--) {
    Serial.printf("%d... ", i);
    delay(1000);
  }
  Serial.println();

  // Turn off LED before sleeping
  digitalWrite(LED_PIN, LOW);

  // Optional: Isolate GPIO pins to reduce current leakage during sleep.
  // This disconnects the GPIO pads from the digital logic, reducing
  // current consumption by a few microamps.
  // Note: Do NOT isolate the wake-up pin!
  // gpio_deep_sleep_hold_en();  // Uncomment if needed

  Serial.println("Going to sleep now. Goodnight!");
  Serial.flush();  // Ensure all serial data is sent before sleeping

  // This is the point of no return. The ESP32 powers down everything
  // except the RTC domain and will not execute any code after this line
  // until it wakes up and reboots.
  esp_deep_sleep_start();

  // This line will never be reached
  Serial.println("This will never print");
}

// ─── Setup ──────────────────────────────────────────────────────────────────
// Remember: setup() runs from the beginning on EVERY wake-up from deep sleep.
// It is essentially a fresh boot, with only RTC memory preserved.

void setup() {
  Serial.begin(115200);

  // Wait for serial port to stabilize after wake-up
  // USB CDC takes a moment to reconnect
  delay(1000);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("=== Lesson 15: Deep Sleep & Battery    ===");
  Serial.println("==========================================");
  Serial.println();

  // Increment the boot counter (stored in RTC memory)
  bootCount++;

  // Print boot count and wake-up reason
  Serial.printf("Boot count: %d\n", bootCount);
  Serial.print("Wake-up reason: ");
  Serial.println(getWakeupReason());
  Serial.println();

  // Configure LED
  pinMode(LED_PIN, OUTPUT);

  // Print system and RTC info
  printSystemInfo();
  printRTCState();

  // Visual indicator that we are awake
  Serial.println("LED blink to show we are awake...");
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    // Timer wake: two quick blinks
    blinkLED(2, 100, 100);
  } else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    // Button wake: three longer blinks
    blinkLED(3, 200, 200);
  } else {
    // First boot: one long blink
    blinkLED(1, 500, 0);
  }

  Serial.println();

  // ─── This is where you would do useful work ───────────────────────────
  // In a real application, this is where you would:
  //   1. Read sensors
  //   2. Connect to WiFi
  //   3. Publish data via MQTT or HTTP
  //   4. Disconnect WiFi
  //   5. Go back to sleep
  //
  // For this demo, we just print info and go back to sleep.

  // Simulate doing some "work"
  Serial.println("--- Simulated Sensor Work ---");
  Serial.printf("  Reading sensor... (simulated: %.1f C)\n", 20.0 + random(0, 100) / 10.0);
  Serial.printf("  Battery voltage:  (simulated: %.2f V)\n", 3.5 + random(0, 50) / 100.0);
  Serial.println("  Work complete.");
  Serial.println();

  // Configure wake-up and enter deep sleep
  configureWakeup();
  enterDeepSleep();
}

// ─── Loop ───────────────────────────────────────────────────────────────────
// The loop() function will never execute in this sketch because the ESP32
// enters deep sleep at the end of setup(). Deep sleep is a full reset;
// when the chip wakes, it starts again from setup().
//
// We include an empty loop() because the Arduino framework requires it.

void loop() {
  // This will never be reached.
  // Deep sleep restarts from setup() on every wake-up.
}
