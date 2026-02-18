/*
 * Lesson 14: Audio Recorder
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Records audio from the built-in PDM microphone and saves WAV files
 * to the SD card. Controlled via serial commands.
 *
 * Hardware used:
 *   - PDM microphone (CLK=GPIO42, DATA=GPIO41) on Sense expansion board
 *   - SD card slot on Sense expansion board
 *
 * Commands (via Serial Monitor at 115200 baud):
 *   record        — Record for 5 seconds (default)
 *   record <sec>  — Record for specified seconds
 *   list          — List all WAV files on SD card
 *   delete <file> — Delete a specific file
 *   info          — Show SD card info
 */

#include <driver/i2s_pdm.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ─── Audio Configuration ────────────────────────────────────────────────────

#define SAMPLE_RATE      16000   // 16 kHz — good quality for voice
#define SAMPLE_BITS      16      // 16-bit audio depth
#define CHANNELS         1       // Mono recording
#define RECORD_SECONDS   5       // Default recording duration

// ─── PDM Microphone Pins (XIAO ESP32-S3 Sense) ─────────────────────────────

#define PDM_CLK_PIN      42      // Clock output to microphone
#define PDM_DATA_PIN     41      // Data input from microphone

// ─── SD Card Pin (XIAO ESP32-S3 Sense) ─────────────────────────────────────

#define SD_CS_PIN        21      // SD card chip select (directly on Sense board)

// ─── I2S Channel Handle ─────────────────────────────────────────────────────

i2s_chan_handle_t rx_handle = NULL;

// ─── Recording State ────────────────────────────────────────────────────────

int recordingNumber = 0;  // Auto-incrementing file number

// ─── WAV Header Structure ───────────────────────────────────────────────────
// The WAV header is a fixed 44-byte structure placed at the beginning of
// every WAV file. It tells audio players the format of the data that follows.

struct WavHeader {
  // RIFF chunk descriptor
  char     riffTag[4];       // "RIFF"
  uint32_t riffSize;         // File size minus 8 bytes
  char     waveTag[4];       // "WAVE"

  // Format sub-chunk
  char     fmtTag[4];        // "fmt "
  uint32_t fmtSize;          // 16 for PCM
  uint16_t audioFormat;      // 1 = PCM (uncompressed)
  uint16_t numChannels;      // 1 = mono, 2 = stereo
  uint32_t sampleRate;       // Samples per second
  uint32_t byteRate;         // sampleRate * numChannels * bitsPerSample/8
  uint16_t blockAlign;       // numChannels * bitsPerSample/8
  uint16_t bitsPerSample;    // 16

  // Data sub-chunk
  char     dataTag[4];       // "data"
  uint32_t dataSize;         // Number of bytes of audio data
};

// ─── Generate WAV Header ────────────────────────────────────────────────────

WavHeader createWavHeader(uint32_t dataSize) {
  WavHeader header;

  // RIFF chunk
  memcpy(header.riffTag, "RIFF", 4);
  header.riffSize = dataSize + 36;  // 36 = header size (44) - 8
  memcpy(header.waveTag, "WAVE", 4);

  // Format sub-chunk
  memcpy(header.fmtTag, "fmt ", 4);
  header.fmtSize      = 16;
  header.audioFormat   = 1;          // PCM
  header.numChannels   = CHANNELS;
  header.sampleRate    = SAMPLE_RATE;
  header.bitsPerSample = SAMPLE_BITS;
  header.blockAlign    = CHANNELS * (SAMPLE_BITS / 8);
  header.byteRate      = SAMPLE_RATE * header.blockAlign;

  // Data sub-chunk
  memcpy(header.dataTag, "data", 4);
  header.dataSize = dataSize;

  return header;
}

// ─── Initialize I2S in PDM RX Mode ──────────────────────────────────────────

bool setupI2S() {
  // Channel configuration
  i2s_chan_config_t chan_cfg = {
    .id = I2S_NUM_0,
    .role = I2S_ROLE_MASTER,
    .dma_desc_num = 6,      // Number of DMA descriptors
    .dma_frame_num = 240,   // Frames per DMA descriptor
    .auto_clear_after_cb = false,
    .auto_clear_before_cb = false,
    .intr_priority = 0,
  };

  esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
  if (err != ESP_OK) {
    Serial.printf("I2S channel creation failed: 0x%x\n", err);
    return false;
  }

  // PDM RX configuration
  i2s_pdm_rx_config_t pdm_rx_cfg = {
    .clk_cfg = {
      .sample_rate_hz = SAMPLE_RATE,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
      .dn_sample_mode = I2S_PDM_DSR_16S,
    },
    .slot_cfg = {
      .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
      .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
      .slot_mode = I2S_SLOT_MODE_MONO,
      .slot_mask = I2S_PDM_SLOT_RIGHT,
    },
    .gpio_cfg = {
      .clk = (gpio_num_t)PDM_CLK_PIN,
      .din = (gpio_num_t)PDM_DATA_PIN,
      .invert_flags = {
        .clk_inv = false,
      },
    },
  };

  err = i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg);
  if (err != ESP_OK) {
    Serial.printf("I2S PDM RX init failed: 0x%x\n", err);
    return false;
  }

  err = i2s_channel_enable(rx_handle);
  if (err != ESP_OK) {
    Serial.printf("I2S channel enable failed: 0x%x\n", err);
    return false;
  }

  return true;
}

// ─── Record Audio to SD Card ────────────────────────────────────────────────

void recordAudio(int durationSeconds) {
  // Generate filename
  recordingNumber++;
  char filename[32];
  snprintf(filename, sizeof(filename), "/recording_%03d.wav", recordingNumber);

  // Calculate expected data size
  uint32_t totalSamples = SAMPLE_RATE * durationSeconds * CHANNELS;
  uint32_t dataSize = totalSamples * (SAMPLE_BITS / 8);

  Serial.printf("Recording for %d seconds...\n", durationSeconds);
  Serial.printf("  File: %s\n", filename);
  Serial.printf("  Expected size: %u bytes\n", dataSize + 44);

  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("ERROR: Failed to open file for writing!");
    return;
  }

  // Write WAV header (with calculated data size)
  WavHeader header = createWavHeader(dataSize);
  file.write((uint8_t*)&header, sizeof(WavHeader));

  // Read audio from I2S and write to SD card in chunks
  const int bufferSize = 1024;            // Bytes per read chunk
  uint8_t buffer[bufferSize];
  uint32_t bytesWritten = 0;
  uint32_t targetBytes = dataSize;
  size_t bytesRead = 0;
  int lastPercent = -1;

  unsigned long startTime = millis();

  Serial.print("Progress: [");

  while (bytesWritten < targetBytes) {
    // Read audio samples from the I2S DMA buffer
    esp_err_t err = i2s_channel_read(rx_handle, buffer, bufferSize, &bytesRead, portMAX_DELAY);

    if (err != ESP_OK || bytesRead == 0) {
      Serial.printf("\nI2S read error: 0x%x\n", err);
      break;
    }

    // Determine how many bytes to actually write (don't exceed target)
    size_t toWrite = bytesRead;
    if (bytesWritten + toWrite > targetBytes) {
      toWrite = targetBytes - bytesWritten;
    }

    // Write to SD card
    file.write(buffer, toWrite);
    bytesWritten += toWrite;

    // Update progress bar (10 segments)
    int percent = (bytesWritten * 100) / targetBytes;
    int segments = percent / 10;
    if (segments != lastPercent / 10 && lastPercent >= 0) {
      Serial.print("#");
    }
    lastPercent = percent;
  }

  // Close the file
  file.close();

  unsigned long elapsed = millis() - startTime;

  Serial.println("]");
  Serial.printf("Saved: %s (%u bytes, %.1f sec, recorded in %lu ms)\n",
                filename, bytesWritten + 44,
                (float)bytesWritten / (SAMPLE_RATE * CHANNELS * (SAMPLE_BITS / 8)),
                elapsed);
}

// ─── List Files on SD Card ──────────────────────────────────────────────────

void listRecordings() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("ERROR: Failed to open SD card root!");
    return;
  }

  Serial.println("─── Recordings on SD Card ───");
  int count = 0;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = file.name();
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        count++;
        Serial.printf("  %d. %-25s  %8u bytes\n", count, file.name(), file.size());
      }
    }
    file = root.openNextFile();
  }

  if (count == 0) {
    Serial.println("  No recordings found.");
  } else {
    Serial.printf("  Total: %d recording(s)\n", count);
  }
}

// ─── Delete a File ──────────────────────────────────────────────────────────

void deleteFile(String filename) {
  // Ensure path starts with /
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.printf("Deleted: %s\n", filename.c_str());
  } else {
    Serial.printf("File not found: %s\n", filename.c_str());
  }
}

// ─── Show SD Card Info ──────────────────────────────────────────────────────

void showCardInfo() {
  Serial.println("─── SD Card Info ───");
  Serial.printf("  Type: ");
  switch (SD.cardType()) {
    case CARD_MMC:  Serial.println("MMC");  break;
    case CARD_SD:   Serial.println("SD");   break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("Unknown"); break;
  }
  Serial.printf("  Total:  %llu MB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("  Used:   %llu MB\n", SD.usedBytes() / (1024 * 1024));
  Serial.printf("  Free:   %llu MB\n", (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024));
}

// ─── Print Help Menu ────────────────────────────────────────────────────────

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  record        - Record for 5 seconds (default)");
  Serial.println("  record <sec>  - Record for specified seconds (1-60)");
  Serial.println("  list          - List all recordings on SD card");
  Serial.println("  delete <file> - Delete a recording (e.g., delete recording_001.wav)");
  Serial.println("  info          - Show SD card info");
  Serial.println("  help          - Show this menu");
  Serial.println();
}

// ─── Setup ──────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Lesson 14: Audio Recorder ===");
  Serial.println();

  // Initialize SD card
  // The Sense expansion board connects SD card via SPI
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("ERROR: SD card mount failed!");
    Serial.println("  - Is the SD card inserted?");
    Serial.println("  - Is it formatted as FAT32?");
    while (true) delay(1000);
  }

  Serial.printf("SD card mounted. Type: ");
  switch (SD.cardType()) {
    case CARD_MMC:  Serial.println("MMC");  break;
    case CARD_SD:   Serial.println("SD");   break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("Unknown"); break;
  }
  Serial.printf("SD card size: %llu MB\n", SD.totalBytes() / (1024 * 1024));

  // Count existing recordings to set starting number
  File root = SD.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String name = file.name();
        if (name.startsWith("recording_") && name.endsWith(".wav")) {
          // Extract number from filename
          String numStr = name.substring(10, 13);  // "recording_XXX.wav"
          int num = numStr.toInt();
          if (num > recordingNumber) {
            recordingNumber = num;
          }
        }
      }
      file = root.openNextFile();
    }
  }
  if (recordingNumber > 0) {
    Serial.printf("Found existing recordings (last: recording_%03d.wav)\n", recordingNumber);
  }

  // Initialize I2S PDM microphone
  if (!setupI2S()) {
    Serial.println("FATAL: I2S PDM microphone initialization failed!");
    while (true) delay(1000);
  }

  Serial.printf("I2S PDM microphone initialized (%d Hz, %d-bit)\n", SAMPLE_RATE, SAMPLE_BITS);

  // Print command menu
  printHelp();
}

// ─── Loop ───────────────────────────────────────────────────────────────────

void loop() {
  // Check for serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() == 0) {
      return;
    }

    Serial.print("> ");
    Serial.println(input);

    // Parse command
    if (input == "record") {
      // Record with default duration
      recordAudio(RECORD_SECONDS);

    } else if (input.startsWith("record ")) {
      // Record with custom duration
      String durStr = input.substring(7);
      durStr.trim();
      int duration = durStr.toInt();
      if (duration >= 1 && duration <= 60) {
        recordAudio(duration);
      } else {
        Serial.println("Duration must be between 1 and 60 seconds.");
      }

    } else if (input == "list") {
      listRecordings();

    } else if (input.startsWith("delete ")) {
      String filename = input.substring(7);
      filename.trim();
      deleteFile(filename);

    } else if (input == "info") {
      showCardInfo();

    } else if (input == "help") {
      printHelp();

    } else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }

    Serial.println();
  }
}
