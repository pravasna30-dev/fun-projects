# Lesson 9: Data Logger (SD Card)

**Difficulty:** Intermediate

**Estimated Time:** 60-75 minutes

---

## What You'll Learn

- How SD cards work with microcontrollers over SPI
- How to read and write files on a FAT32-formatted SD card
- How to create and append to CSV (Comma-Separated Values) files
- How to use `millis()` for timestamps and time tracking
- How to build a complete data logging system
- How to read back logged data via serial commands

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Main microcontroller (with Sense expansion board) |
| MicroSD card | 1 | Formatted as FAT32, 32GB or smaller recommended |
| USB-C cable | 1 | For programming and power |

> **No extra wiring needed!** The Sense expansion board has a built-in MicroSD card slot that connects to the ESP32-S3 via SPI. Just insert the card and go.

## Circuit Diagram

```
   No external wiring required!

   The XIAO ESP32-S3 Sense expansion board has a built-in
   MicroSD card slot on the back side of the board.

   ┌────────────────────────────────┐
   │  XIAO ESP32-S3 Sense Board    │
   │  ┌──────────────────────┐     │
   │  │  [Camera Module]     │     │
   │  └──────────────────────┘     │
   │                               │
   │  ┌──────────────┐             │
   │  │  MicroSD     │  <-- Insert │
   │  │  Card Slot   │      card   │
   │  └──────────────┘      here   │
   │                               │
   └────────────────────────────────┘

   Internal SPI connections (handled by the board):
     SD_CS   = GPIO 21
     SD_MOSI = GPIO 9  (shared SPI bus)
     SD_MISO = GPIO 8  (shared SPI bus)
     SD_SCK  = GPIO 7  (shared SPI bus)

   Note: The built-in LED is also on GPIO 21 and shares
   the pin with SD_CS. You cannot use the built-in LED
   while the SD card is active.
```

## Step-by-Step Instructions

### Step 1: Format Your MicroSD Card

The SD library requires a **FAT32** formatted card. Most cards come pre-formatted this way, but to be safe:

**On macOS:**
1. Open **Disk Utility**.
2. Select your SD card from the left sidebar.
3. Click **Erase**.
4. Format: **MS-DOS (FAT)** (this is FAT32 for cards under 32GB).
5. Click **Erase** to confirm.

**On Windows:**
1. Right-click the SD card in File Explorer.
2. Select **Format...**
3. File system: **FAT32**.
4. Click **Start**.

> **Important:** FAT32 has a maximum file size of 4GB and a maximum partition size of 32GB. For most data logging applications, this is more than enough. If your card is larger than 32GB, you may need a special formatting tool.

### Step 2: Insert the Card

1. Locate the MicroSD slot on the back of the Sense expansion board.
2. Insert the card with the contacts facing down (toward the board).
3. Push until it clicks into place.

### Step 3: No Library Installation Needed

The SD card library (`SD.h`) and SPI library (`SPI.h`) come pre-installed with the ESP32 Arduino core. No additional libraries are required.

### Step 4: Upload the Sketch

1. Open `data-logger-sd.ino` in Arduino IDE.
2. Select **Tools > Board > XIAO_ESP32S3**.
3. Select **Tools > Port > /dev/cu.usbmodem101**.
4. Upload and open Serial Monitor at **115200 baud**.

### Step 5: Interact via Serial Commands

The sketch supports several commands:
- **`log`** — Toggle auto-logging on/off
- **`read`** — Read the entire log file and print to serial
- **`status`** — Show SD card info and file size
- **`clear`** — Delete the log file and start fresh
- **`sample`** — Take a single manual reading

## The Code

### SD Card Initialization

```cpp
#include "SD.h"
#include "SPI.h"

#define SD_CS_PIN 21

if (!SD.begin(SD_CS_PIN)) {
  Serial.println("SD card initialization failed!");
}
```

`SD.begin()` initializes the SPI bus and attempts to communicate with the card. The chip-select (CS) pin tells the SD card when it's being addressed — essential when multiple SPI devices share the same bus.

### Writing CSV Data

```cpp
File dataFile = SD.open("/datalog.csv", FILE_APPEND);
if (dataFile) {
  dataFile.printf("%lu,%lu,%.1f,%.1f\n",
    readingNumber, millis(), temperature, humidity);
  dataFile.close();
}
```

Key points:
- **`FILE_APPEND`** opens the file for writing at the end. If the file doesn't exist, it creates it.
- **Always close the file** after writing. This flushes the buffer to the card and updates the directory entry. If you don't close, data can be lost on power failure.
- **CSV format** uses commas to separate fields, making it easy to import into Excel, Google Sheets, or Python.

### Reading Files Back

```cpp
File dataFile = SD.open("/datalog.csv", FILE_READ);
while (dataFile.available()) {
  Serial.write(dataFile.read());
}
dataFile.close();
```

The file is read byte by byte and sent to the serial monitor. For large files, this can take a while — the sketch reads in chunks for efficiency.

### Simulated Sensor Data

Since this lesson focuses on SD card operations, we simulate sensor data using:
- A sine wave for temperature (oscillates around 25 degrees C)
- Random noise added for realism
- Millis-based timestamps for each reading

If you've completed Lesson 7, you can easily swap in real DHT22 data.

## How It Works

### SPI Communication Protocol

SPI (Serial Peripheral Interface) uses four wires:

| Wire | Full Name | Direction | Purpose |
|------|-----------|-----------|---------|
| MOSI | Master Out Slave In | Master -> Slave | Data from ESP32 to SD card |
| MISO | Master In Slave Out | Slave -> Master | Data from SD card to ESP32 |
| SCK  | Serial Clock | Master -> Slave | Clock signal |
| CS   | Chip Select | Master -> Slave | Selects which device to talk to |

**SPI vs. I2C:**
- SPI is faster (up to 80 MHz on ESP32 vs. 400 kHz for I2C)
- SPI needs more wires (4 minimum vs. 2 for I2C)
- SPI uses CS pins instead of addresses (one CS pin per device)
- SD cards use SPI mode for simple microcontroller access

### How SD Cards Store Data

SD cards contain NAND flash memory organized into:

1. **Sectors** — The smallest readable/writable unit (512 bytes).
2. **Clusters** — Groups of sectors (typically 4-64 sectors). The file system allocates space in cluster-sized chunks.
3. **FAT (File Allocation Table)** — A table that maps which clusters belong to which file. It's essentially a linked list on disk.

**FAT32 specifics:**
- Maximum file size: 4 GB (2^32 bytes - 1)
- Maximum volume size: 32 GB (practical limit without special tools)
- File name format: 8.3 (classic DOS) or Long File Names (LFN)
- The "32" in FAT32 means the cluster addresses are 32-bit numbers

### CSV File Format

CSV is one of the simplest data formats:
```
reading,timestamp_ms,temperature_c,humidity_pct
1,2000,23.4,45.2
2,4000,23.5,45.1
3,6000,23.6,45.3
```

Rules:
- First row is typically a header describing each column
- Each subsequent row is one data record
- Fields are separated by commas
- No spaces after commas (unless the data contains spaces)
- Newline (`\n`) ends each row

CSV files can be opened directly in spreadsheet software for graphing and analysis.

### Timestamps with millis()

Since the ESP32-S3 doesn't have a battery-backed Real-Time Clock (RTC), we use `millis()` to track time since boot. This gives millisecond-precision relative timestamps. For absolute timestamps (actual date and time), you would need either:
- An external RTC module (like DS3231)
- WiFi connection to sync with an NTP time server (a future lesson)

## Try It Out

1. After uploading, the Serial Monitor should show:
   ```
   SD card initialized successfully.
   Card size: 29688 MB
   Starting auto-log mode...
   ```
2. Watch data being logged every 5 seconds.
3. Type **`status`** to see the current file size.
4. Type **`read`** to see all logged data.
5. Let it run for a few minutes, then type **`read`** to see the accumulated data.
6. Remove the SD card, insert it into your computer, and open `datalog.csv` in a spreadsheet application.
7. Type **`clear`** to start fresh.

## Challenges

1. **Real Sensor Integration:** If you completed Lesson 7, replace the simulated data with actual DHT22 temperature and humidity readings. Add the heat index as an additional CSV column.
2. **Daily Log Files:** Instead of one big `datalog.csv`, create a new file for each "session" (e.g., `log_001.csv`, `log_002.csv`). On startup, scan for existing files and increment the number.
3. **Binary Format:** CSV is human-readable but wastes space. Implement a binary logging mode where each record is a fixed-size struct written directly to the file. Compare file sizes — how much space do you save?

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| "SD card initialization failed!" | Card not inserted | Push the card in until it clicks |
| "SD card initialization failed!" | Card not FAT32 | Reformat as FAT32 (see Step 1) |
| "SD card initialization failed!" | Card too large | Use a card 32GB or smaller |
| File is empty after logging | File not closed properly | Ensure `dataFile.close()` is called after every write |
| Built-in LED doesn't work | GPIO 21 conflict | The built-in LED shares GPIO 21 with SD_CS; you can't use both simultaneously |
| Data looks corrupted | Power loss during write | Always close files promptly; consider using `flush()` for critical data |
| "Failed to open file" | File path issue | ESP32 SD library requires paths starting with `/` (e.g., `/datalog.csv`) |
| Card works on computer but not ESP32 | exFAT format | Cards over 32GB are usually exFAT — reformat to FAT32 |

## What's Next

With data logging mastered, it's time to go wireless! In the next lesson, we'll explore **Bluetooth Low Energy (BLE)** — broadcasting data wirelessly and controlling the board from your phone.

[Lesson 10: Bluetooth LE Basics ->](../10-bluetooth-le-basics/)
