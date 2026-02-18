/*
 * Lesson 9: Data Logger (SD Card)
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Logs simulated sensor data to a CSV file on the built-in MicroSD card.
 * Supports serial commands for control:
 *   - "log"    -> Toggle auto-logging on/off
 *   - "read"   -> Print the entire log file to serial
 *   - "status" -> Show SD card info and file size
 *   - "clear"  -> Delete the log file
 *   - "sample" -> Take a single manual reading
 *
 * No extra wiring needed — the Sense expansion board has a built-in
 * MicroSD card slot connected via SPI.
 *
 * SD Card Connections (internal to the Sense board):
 *   SD_CS   = GPIO 21
 *   SD_MOSI = GPIO 9
 *   SD_MISO = GPIO 8
 *   SD_SCK  = GPIO 7
 */

// ---------------------------------------------------------------------------
// Library Includes
// ---------------------------------------------------------------------------
#include "SD.h"       // SD card file system library
#include "SPI.h"      // SPI communication (used by SD library internally)

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
// Chip Select pin for the SD card on the XIAO ESP32-S3 Sense board.
// Note: This pin is shared with the built-in LED (GPIO 21), so the LED
// is unavailable while the SD card is in use.
#define SD_CS_PIN 21

// Log file path — must start with '/' for the ESP32 SD library
const char* LOG_FILE = "/datalog.csv";

// Logging interval in milliseconds (how often to record data)
const unsigned long LOG_INTERVAL_MS = 5000;  // Every 5 seconds

// ---------------------------------------------------------------------------
// State Variables
// ---------------------------------------------------------------------------
bool autoLogEnabled = true;           // Whether auto-logging is active
unsigned long lastLogTime = 0;        // Timestamp of last log entry
unsigned long readingCount = 0;       // Total readings taken this session
bool sdInitialized = false;           // Whether the SD card is ready

// Simulated sensor state — a sine wave with noise for realistic data
float simPhase = 0.0;                 // Phase angle for sine wave simulation

// ---------------------------------------------------------------------------
// Function: initSDCard
// Attempts to initialize the SD card and prints diagnostic info.
// Returns true on success, false on failure.
// ---------------------------------------------------------------------------
bool initSDCard() {
  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERROR] SD card initialization failed!");
    Serial.println("  - Is a MicroSD card inserted?");
    Serial.println("  - Is it formatted as FAT32?");
    Serial.println("  - Is the card 32GB or smaller?");
    return false;
  }

  // Print card information
  uint8_t cardType = SD.cardType();
  Serial.print("Card type: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC");    break;
    case CARD_SD:   Serial.println("SD");     break;
    case CARD_SDHC: Serial.println("SDHC");   break;
    default:        Serial.println("Unknown"); break;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);  // Convert to MB
  Serial.printf("Card size: %llu MB\n", cardSize);

  uint64_t usedSpace = SD.usedBytes() / 1024;  // Convert to KB
  uint64_t totalSpace = SD.totalBytes() / (1024 * 1024);  // Convert to MB
  Serial.printf("Used: %llu KB | Total: %llu MB\n", usedSpace, totalSpace);

  return true;
}

// ---------------------------------------------------------------------------
// Function: writeCSVHeader
// Writes the CSV header row if the log file doesn't exist yet.
// The header describes each column for easy import into spreadsheets.
// ---------------------------------------------------------------------------
void writeCSVHeader() {
  // Check if the file already exists
  if (SD.exists(LOG_FILE)) {
    Serial.printf("Log file '%s' already exists. Appending data.\n", LOG_FILE);
    return;
  }

  // Create the file and write the header
  File dataFile = SD.open(LOG_FILE, FILE_WRITE);
  if (dataFile) {
    dataFile.println("reading,timestamp_ms,uptime_s,temp_c,humidity_pct,heat_index_c");
    dataFile.close();
    Serial.printf("Created new log file: %s\n", LOG_FILE);
  } else {
    Serial.printf("[ERROR] Failed to create log file: %s\n", LOG_FILE);
  }
}

// ---------------------------------------------------------------------------
// Function: getSimulatedTemperature
// Returns a simulated temperature value that oscillates realistically.
// Uses a sine wave centered at 25°C with some random noise.
// ---------------------------------------------------------------------------
float getSimulatedTemperature() {
  // Base temperature with slow sine wave oscillation (+/- 5°C)
  float temp = 25.0 + 5.0 * sin(simPhase);
  // Add small random noise (+/- 0.3°C)
  temp += (random(-30, 31) / 100.0);
  return temp;
}

// ---------------------------------------------------------------------------
// Function: getSimulatedHumidity
// Returns a simulated humidity value that varies inversely with temperature.
// (In real life, relative humidity often drops as temperature rises.)
// ---------------------------------------------------------------------------
float getSimulatedHumidity() {
  float humidity = 55.0 - 10.0 * sin(simPhase);
  humidity += (random(-50, 51) / 100.0);
  // Clamp to valid range
  if (humidity < 0) humidity = 0;
  if (humidity > 100) humidity = 100;
  return humidity;
}

// ---------------------------------------------------------------------------
// Function: computeSimpleHeatIndex
// A simplified heat index approximation.
// For a proper calculation, use the DHT library's computeHeatIndex().
// ---------------------------------------------------------------------------
float computeSimpleHeatIndex(float tempC, float humidity) {
  // Simple approximation — accurate enough for demonstration
  float hi = tempC + 0.5555 * (6.11 * exp(5417.7530 * (1.0/273.16 - 1.0/(273.15 + tempC))) * humidity / 100.0 - 10.0);
  return hi;
}

// ---------------------------------------------------------------------------
// Function: logDataPoint
// Records one data point to the CSV file on the SD card.
// ---------------------------------------------------------------------------
void logDataPoint() {
  readingCount++;

  // Get simulated sensor values
  float temperature = getSimulatedTemperature();
  float humidity    = getSimulatedHumidity();
  float heatIndex   = computeSimpleHeatIndex(temperature, humidity);

  // Advance the simulation phase (slowly cycles over ~5 minutes)
  simPhase += 0.02;
  if (simPhase > 2.0 * PI) {
    simPhase -= 2.0 * PI;
  }

  // Calculate uptime in seconds
  unsigned long uptimeSec = millis() / 1000;

  // Open the file in append mode
  File dataFile = SD.open(LOG_FILE, FILE_APPEND);
  if (dataFile) {
    // Write one CSV row
    dataFile.printf("%lu,%lu,%lu,%.1f,%.1f,%.1f\n",
      readingCount,
      millis(),
      uptimeSec,
      temperature,
      humidity,
      heatIndex
    );
    dataFile.close();  // IMPORTANT: Always close to flush data to card

    // Also print to serial for monitoring
    Serial.printf("[LOG #%lu] t=%lus | Temp=%.1f°C | Hum=%.1f%% | HI=%.1f°C\n",
      readingCount, uptimeSec, temperature, humidity, heatIndex);
  } else {
    Serial.println("[ERROR] Failed to open log file for writing!");
  }
}

// ---------------------------------------------------------------------------
// Function: readLogFile
// Reads the entire log file and prints it to the serial monitor.
// ---------------------------------------------------------------------------
void readLogFile() {
  if (!SD.exists(LOG_FILE)) {
    Serial.println("No log file found. Start logging first.");
    return;
  }

  File dataFile = SD.open(LOG_FILE, FILE_READ);
  if (!dataFile) {
    Serial.println("[ERROR] Failed to open log file for reading!");
    return;
  }

  Serial.println();
  Serial.println("========== LOG FILE CONTENTS ==========");
  Serial.printf("File: %s | Size: %lu bytes\n", LOG_FILE, dataFile.size());
  Serial.println("----------------------------------------");

  // Read and print the file in chunks for efficiency
  // Reading byte-by-byte over serial is slow for large files
  const int CHUNK_SIZE = 512;
  uint8_t buffer[CHUNK_SIZE];
  int lineCount = 0;

  while (dataFile.available()) {
    int bytesRead = dataFile.read(buffer, CHUNK_SIZE);
    Serial.write(buffer, bytesRead);

    // Count lines for summary
    for (int i = 0; i < bytesRead; i++) {
      if (buffer[i] == '\n') lineCount++;
    }
  }

  dataFile.close();

  Serial.println("----------------------------------------");
  Serial.printf("Total lines: %d (including header)\n", lineCount);
  Serial.println("========================================");
  Serial.println();
}

// ---------------------------------------------------------------------------
// Function: showStatus
// Displays SD card and log file status information.
// ---------------------------------------------------------------------------
void showStatus() {
  Serial.println();
  Serial.println("========== STATUS ==========");

  // Card info
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  uint64_t usedBytes = SD.usedBytes();
  uint64_t totalBytes = SD.totalBytes();
  Serial.printf("Card: %llu MB total\n", cardSize);
  Serial.printf("Used: %llu KB / %llu MB\n", usedBytes / 1024, totalBytes / (1024 * 1024));

  // Log file info
  if (SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_READ);
    Serial.printf("Log file: %s (%lu bytes)\n", LOG_FILE, f.size());
    f.close();
  } else {
    Serial.printf("Log file: %s (not created yet)\n", LOG_FILE);
  }

  // Session info
  Serial.printf("Readings this session: %lu\n", readingCount);
  Serial.printf("Auto-log: %s\n", autoLogEnabled ? "ENABLED" : "DISABLED");
  Serial.printf("Log interval: %lu ms\n", LOG_INTERVAL_MS);

  // Uptime
  unsigned long sec = millis() / 1000;
  Serial.printf("Uptime: %luh %lum %lus\n", sec / 3600, (sec % 3600) / 60, sec % 60);

  Serial.println("============================");
  Serial.println();
}

// ---------------------------------------------------------------------------
// Function: clearLogFile
// Deletes the log file and recreates it with a fresh header.
// ---------------------------------------------------------------------------
void clearLogFile() {
  if (SD.exists(LOG_FILE)) {
    SD.remove(LOG_FILE);
    Serial.printf("Deleted '%s'.\n", LOG_FILE);
  }
  readingCount = 0;
  writeCSVHeader();
  Serial.println("Log file cleared. Ready for new data.");
}

// ---------------------------------------------------------------------------
// Function: handleSerialInput
// Processes serial commands from the user.
// ---------------------------------------------------------------------------
void handleSerialInput() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "log") {
      autoLogEnabled = !autoLogEnabled;
      Serial.printf("Auto-logging: %s\n", autoLogEnabled ? "ENABLED" : "DISABLED");

    } else if (cmd == "read") {
      readLogFile();

    } else if (cmd == "status") {
      showStatus();

    } else if (cmd == "clear") {
      clearLogFile();

    } else if (cmd == "sample") {
      Serial.println("Taking manual sample...");
      logDataPoint();

    } else if (cmd == "help") {
      printHelp();

    } else {
      Serial.printf("Unknown command: '%s'. Type 'help' for commands.\n", cmd.c_str());
    }
  }
}

// ---------------------------------------------------------------------------
// Function: printHelp
// Shows the list of available serial commands.
// ---------------------------------------------------------------------------
void printHelp() {
  Serial.println();
  Serial.println("Available Commands:");
  Serial.println("  log    - Toggle auto-logging on/off");
  Serial.println("  read   - Display the entire log file");
  Serial.println("  status - Show SD card and log info");
  Serial.println("  clear  - Delete log file and start fresh");
  Serial.println("  sample - Take a single manual reading");
  Serial.println("  help   - Show this help message");
  Serial.println();
}

// ---------------------------------------------------------------------------
// setup() - Runs once at startup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==========================================");
  Serial.println("  Lesson 9: Data Logger (SD Card)");
  Serial.println("  XIAO ESP32-S3 Sense");
  Serial.println("==========================================");
  Serial.println();

  // Seed the random number generator for simulated noise
  randomSeed(analogRead(A0));

  // Initialize the SD card
  sdInitialized = initSDCard();

  if (!sdInitialized) {
    Serial.println();
    Serial.println("WARNING: Running without SD card.");
    Serial.println("Data will only be shown on serial, not logged.");
    Serial.println();
  } else {
    Serial.println();
    writeCSVHeader();
    Serial.println();
  }

  // Print available commands
  printHelp();

  if (autoLogEnabled) {
    Serial.printf("Auto-logging every %lu ms. Type 'log' to toggle.\n", LOG_INTERVAL_MS);
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
// loop() - Runs repeatedly after setup()
// ---------------------------------------------------------------------------
void loop() {
  // Always check for serial commands
  handleSerialInput();

  // Auto-log at the configured interval
  if (autoLogEnabled && sdInitialized) {
    if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
      lastLogTime = millis();
      logDataPoint();
    }
  }
}
