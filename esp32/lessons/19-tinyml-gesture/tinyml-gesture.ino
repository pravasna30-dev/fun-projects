/*
 * Lesson 19: TinyML Gesture Recognition
 *
 * Board: Seeed Studio XIAO ESP32S3 Sense
 * Sensor: MPU6050 accelerometer/gyroscope via I2C
 *
 * This sketch demonstrates the TinyML pipeline for gesture recognition:
 * 1. Read accelerometer/gyroscope data from the MPU6050
 * 2. Collect gesture samples via serial commands
 * 3. Classify gestures using a threshold-based approach
 *
 * In a real TinyML project, step 3 would use a TensorFlow Lite Micro
 * model trained on the collected data. This sketch uses thresholds as
 * a stand-in to demonstrate the concept without requiring the full
 * training pipeline.
 *
 * Gestures:
 *   - PUNCH: Quick thrust of the sensor forward
 *   - FLEX:  Slow upward tilt of the wrist
 *   - IDLE:  No movement (sensor at rest)
 *
 * Serial Commands:
 *   p - Record a punch gesture sample
 *   f - Record a flex gesture sample
 *   c - Classify the next gesture
 *   r - Stream raw sensor data
 *   s - Stop streaming
 *
 * Wiring:
 *   MPU6050 VCC -> 3V3
 *   MPU6050 GND -> GND
 *   MPU6050 SDA -> D4 (GPIO 6)
 *   MPU6050 SCL -> D5 (GPIO 7)
 *
 * Libraries needed:
 *   - Adafruit MPU6050
 *   - Adafruit Unified Sensor
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ============================================================
// Configuration
// ============================================================

// Sampling parameters
#define SAMPLE_RATE_HZ   50       // Samples per second during recording
#define SAMPLE_DURATION  1000     // Recording duration in milliseconds
#define NUM_SAMPLES      (SAMPLE_RATE_HZ * SAMPLE_DURATION / 1000)  // 50 samples
#define NUM_AXES         6        // ax, ay, az, gx, gy, gz

// Classification thresholds
// These are tuned for typical hand gestures. Adjust based on your
// testing. A real TinyML model would learn these automatically.
#define PUNCH_ACCEL_THRESHOLD  15.0   // Peak acceleration for punch (m/s^2)
#define FLEX_GYRO_THRESHOLD    2.0    // Gyroscope energy for flex (rad/s)
#define IDLE_ACCEL_THRESHOLD   12.0   // Below this = idle (close to gravity ~9.8)
#define FLEX_ACCEL_MAX         15.0   // Flex has moderate accel, below punch

// Built-in LED for feedback
#define LED_PIN 21

// ============================================================
// Global Objects
// ============================================================

Adafruit_MPU6050 mpu;

// Gesture data buffer
// Stores NUM_SAMPLES readings, each with NUM_AXES values
float gestureData[NUM_SAMPLES][NUM_AXES];

// Extracted features (computed from gestureData)
float peakAccel     = 0;  // Maximum acceleration magnitude
float avgAccel      = 0;  // Average acceleration magnitude
float gyroEnergy    = 0;  // Total gyroscope activity
float accelVariance = 0;  // How much acceleration varies (dynamic vs static)

// Streaming mode flag
bool streamingRaw = false;

// Gesture labels for display
const char* gestureNames[] = {"IDLE", "PUNCH", "FLEX", "UNKNOWN"};
enum Gesture { IDLE = 0, PUNCH = 1, FLEX = 2, UNKNOWN = 3 };

// ============================================================
// MPU6050 Functions
// ============================================================

/*
 * initMPU() -- Initialize the MPU6050 sensor with appropriate
 * range settings for gesture detection.
 */
bool initMPU() {
  if (!mpu.begin()) {
    Serial.println("[MPU6050] Could not find sensor. Check wiring!");
    return false;
  }

  // Set accelerometer range to +/- 8g
  // Punches can generate significant force; 8g gives good range
  // without losing resolution for subtle movements
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // Set gyroscope range to +/- 500 degrees/second
  // Adequate for wrist movements
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Set filter bandwidth to 21 Hz
  // Reduces high-frequency noise while preserving gesture dynamics
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("[MPU6050] Initialized");
  Serial.println("[MPU6050] Accel range: +/- 8g");
  Serial.println("[MPU6050] Gyro range:  +/- 500 deg/s");
  Serial.println("[MPU6050] Filter:      21 Hz bandwidth");

  return true;
}

/*
 * readSensor() -- Read current accelerometer and gyroscope values.
 * Stores results in the provided array (6 values).
 */
void readSensor(float* data) {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  data[0] = accel.acceleration.x;  // m/s^2
  data[1] = accel.acceleration.y;
  data[2] = accel.acceleration.z;
  data[3] = gyro.gyro.x;           // rad/s
  data[4] = gyro.gyro.y;
  data[5] = gyro.gyro.z;
}

// ============================================================
// Data Collection
// ============================================================

/*
 * recordGesture() -- Record a gesture sample into the gestureData buffer.
 * Collects NUM_SAMPLES readings at SAMPLE_RATE_HZ.
 *
 * The function provides a countdown so the user can prepare,
 * then captures data for SAMPLE_DURATION milliseconds.
 */
void recordGesture(const char* gestureName) {
  Serial.printf("\n=== Recording '%s' gesture ===\n", gestureName);
  Serial.println("Get ready...");
  delay(500);
  Serial.println("Recording in 1 second...");
  delay(1000);

  // Flash LED to indicate recording has started
  digitalWrite(LED_PIN, LOW);  // LED ON (active low)
  Serial.println(">>> RECORDING NOW! Perform the gesture! <<<");

  unsigned long sampleInterval = 1000 / SAMPLE_RATE_HZ;  // ms between samples
  unsigned long startTime = millis();

  for (int i = 0; i < NUM_SAMPLES; i++) {
    unsigned long sampleStart = millis();
    readSensor(gestureData[i]);

    // Print data in CSV format for potential export
    Serial.printf("%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                  i,
                  gestureData[i][0], gestureData[i][1], gestureData[i][2],
                  gestureData[i][3], gestureData[i][4], gestureData[i][5]);

    // Maintain consistent sample rate
    unsigned long elapsed = millis() - sampleStart;
    if (elapsed < sampleInterval) {
      delay(sampleInterval - elapsed);
    }
  }

  unsigned long totalTime = millis() - startTime;
  digitalWrite(LED_PIN, HIGH);  // LED OFF

  Serial.printf(">>> Recording complete (%lu ms, %d samples) <<<\n",
                totalTime, NUM_SAMPLES);

  // Extract features from the recorded data
  extractFeatures();
  printFeatures();
}

// ============================================================
// Feature Extraction
// ============================================================

/*
 * extractFeatures() -- Compute summary features from the raw
 * gesture data. These features are what a classifier uses to
 * make decisions.
 *
 * In a neural network approach, the network learns what features
 * matter automatically. In our threshold approach, we manually
 * define the features we think are important.
 */
void extractFeatures() {
  peakAccel = 0;
  avgAccel = 0;
  gyroEnergy = 0;
  accelVariance = 0;

  float accelMagnitudes[NUM_SAMPLES];

  // First pass: compute per-sample acceleration magnitude and gyro energy
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float ax = gestureData[i][0];
    float ay = gestureData[i][1];
    float az = gestureData[i][2];
    float gx = gestureData[i][3];
    float gy = gestureData[i][4];
    float gz = gestureData[i][5];

    // Acceleration magnitude: sqrt(ax^2 + ay^2 + az^2)
    // At rest, this should be ~9.8 m/s^2 (gravity)
    float accelMag = sqrt(ax * ax + ay * ay + az * az);
    accelMagnitudes[i] = accelMag;

    // Track peak acceleration
    if (accelMag > peakAccel) {
      peakAccel = accelMag;
    }

    // Accumulate for average
    avgAccel += accelMag;

    // Gyroscope energy: sum of absolute rotation rates
    // Higher values indicate more rotational movement
    gyroEnergy += fabs(gx) + fabs(gy) + fabs(gz);
  }

  // Compute average
  avgAccel /= NUM_SAMPLES;

  // Average gyro energy per sample
  gyroEnergy /= NUM_SAMPLES;

  // Second pass: compute variance of acceleration
  // High variance = dynamic movement; low variance = static
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float diff = accelMagnitudes[i] - avgAccel;
    accelVariance += diff * diff;
  }
  accelVariance /= NUM_SAMPLES;
}

/*
 * printFeatures() -- Display extracted features for debugging
 * and understanding the data.
 */
void printFeatures() {
  Serial.println("\n--- Extracted Features ---");
  Serial.printf("  Peak acceleration: %.2f m/s^2\n", peakAccel);
  Serial.printf("  Avg acceleration:  %.2f m/s^2\n", avgAccel);
  Serial.printf("  Accel variance:    %.2f\n", accelVariance);
  Serial.printf("  Gyro energy:       %.2f rad/s\n", gyroEnergy);
  Serial.println("--------------------------");
}

// ============================================================
// Classification
// ============================================================

/*
 * classifyGesture() -- Classify the current gestureData buffer
 * using threshold-based rules.
 *
 * This is a SIMPLIFIED approach. A real TinyML system would use
 * a trained neural network here. The thresholds are:
 *
 * PUNCH: High peak acceleration (sharp, fast motion)
 * FLEX:  Moderate acceleration + significant gyroscope rotation
 * IDLE:  Low acceleration variance (sensor at rest)
 *
 * Returns a Gesture enum value.
 */
Gesture classifyGesture() {
  // Extract features from the current data buffer
  extractFeatures();
  printFeatures();

  // --- Decision Tree ---

  // Rule 1: If peak acceleration is very high, it's a punch
  if (peakAccel > PUNCH_ACCEL_THRESHOLD && accelVariance > 5.0) {
    return PUNCH;
  }

  // Rule 2: If there's significant rotation with moderate accel, it's a flex
  if (gyroEnergy > FLEX_GYRO_THRESHOLD && peakAccel < FLEX_ACCEL_MAX) {
    return FLEX;
  }

  // Rule 3: If acceleration is stable near gravity, it's idle
  if (peakAccel < IDLE_ACCEL_THRESHOLD && accelVariance < 2.0) {
    return IDLE;
  }

  // Rule 4: Moderate acceleration without much rotation -- could be a weak punch
  if (peakAccel > IDLE_ACCEL_THRESHOLD && gyroEnergy < FLEX_GYRO_THRESHOLD) {
    return PUNCH;
  }

  // Rule 5: Rotation with any movement -- more likely flex
  if (gyroEnergy > FLEX_GYRO_THRESHOLD * 0.7) {
    return FLEX;
  }

  // Default: not enough evidence for any gesture
  return UNKNOWN;
}

/*
 * runClassification() -- Record a gesture and classify it.
 */
void runClassification() {
  Serial.println("\n========================================");
  Serial.println("  GESTURE CLASSIFICATION MODE");
  Serial.println("  Perform any gesture after countdown");
  Serial.println("========================================");

  recordGesture("unknown");

  Gesture result = classifyGesture();

  Serial.println();
  Serial.println("========================================");
  Serial.printf("  CLASSIFICATION RESULT: %s\n", gestureNames[result]);
  Serial.println("========================================");

  // Visual feedback via LED
  switch (result) {
    case PUNCH:
      // Fast triple blink for punch
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
      }
      break;
    case FLEX:
      // Slow double blink for flex
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(300);
        digitalWrite(LED_PIN, HIGH);
        delay(300);
      }
      break;
    case IDLE:
      // Single long pulse for idle
      digitalWrite(LED_PIN, LOW);
      delay(1000);
      digitalWrite(LED_PIN, HIGH);
      break;
    default:
      // Rapid blink for unknown
      for (int i = 0; i < 6; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        delay(50);
      }
      break;
  }

  Serial.println("\nReady for next command (p/f/c/r/s)");
}

// ============================================================
// Raw Data Streaming
// ============================================================

/*
 * streamRawData() -- Print current sensor values.
 * Called every loop iteration when streaming is enabled.
 * Output format is CSV for easy capture and analysis.
 */
void streamRawData() {
  float data[NUM_AXES];
  readSensor(data);

  // Calculate acceleration magnitude for convenience
  float mag = sqrt(data[0]*data[0] + data[1]*data[1] + data[2]*data[2]);

  Serial.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                data[0], data[1], data[2],
                data[3], data[4], data[5],
                mag);
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Off (active low)

  Serial.println("==========================================");
  Serial.println("  XIAO ESP32S3 TinyML Gesture Recognition");
  Serial.println("==========================================");
  Serial.println();

  // Initialize I2C and MPU6050
  Wire.begin();  // Default SDA=D4, SCL=D5

  if (!initMPU()) {
    Serial.println("[FATAL] MPU6050 not found! Check wiring.");
    Serial.println("  SDA -> D4 (GPIO 6)");
    Serial.println("  SCL -> D5 (GPIO 7)");
    Serial.println("  VCC -> 3V3");
    Serial.println("  GND -> GND");
    while (true) {
      // Blink rapidly to indicate error
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
    }
  }

  // Print help
  Serial.println();
  Serial.println("=== Commands ===");
  Serial.println("  p - Record a PUNCH gesture");
  Serial.println("  f - Record a FLEX gesture");
  Serial.println("  c - Classify next gesture");
  Serial.println("  r - Stream raw sensor data");
  Serial.println("  s - Stop streaming");
  Serial.println("================");
  Serial.println();
  Serial.println("Ready! Send a command via Serial Monitor.");
  Serial.println("(Set line ending to 'Newline')");
}

// ============================================================
// Main Loop
// ============================================================

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    // Flush remaining characters (like newline)
    while (Serial.available()) Serial.read();

    switch (cmd) {
      case 'p':
      case 'P':
        streamingRaw = false;
        recordGesture("PUNCH");
        Serial.println("\nReady for next command (p/f/c/r/s)");
        break;

      case 'f':
      case 'F':
        streamingRaw = false;
        recordGesture("FLEX");
        Serial.println("\nReady for next command (p/f/c/r/s)");
        break;

      case 'c':
      case 'C':
        streamingRaw = false;
        runClassification();
        break;

      case 'r':
      case 'R':
        streamingRaw = true;
        Serial.println("\n--- Raw Data Streaming (ax,ay,az,gx,gy,gz,mag) ---");
        Serial.println("--- Send 's' to stop ---");
        break;

      case 's':
      case 'S':
        streamingRaw = false;
        Serial.println("\n--- Streaming stopped ---");
        Serial.println("Ready for next command (p/f/c/r/s)");
        break;

      default:
        Serial.printf("Unknown command: '%c'\n", cmd);
        Serial.println("Commands: p(unch), f(lex), c(lassify), r(aw), s(top)");
        break;
    }
  }

  // Stream raw data if enabled
  if (streamingRaw) {
    streamRawData();
    delay(20);  // ~50 Hz output rate
  }

  delay(1);  // Prevent watchdog issues
}
