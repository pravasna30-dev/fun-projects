# Lesson 14: Audio Recorder

**Difficulty:** Advanced ![Advanced](https://img.shields.io/badge/level-advanced-red)

**Estimated Time:** 75-90 minutes

---

## What You'll Learn

- How PDM (Pulse Density Modulation) microphones work
- How the I2S (Inter-IC Sound) interface captures digital audio on the ESP32-S3
- How to sample audio at different rates and understand the Nyquist theorem
- The WAV file format and how to write a proper WAV header
- How to record audio to an SD card as playable WAV files
- How to manage serial commands for interactive recording control

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Sense expansion board has built-in PDM microphone and SD card slot |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |
| MicroSD card | 1 | FAT32 formatted, any size (even 1 GB is plenty) |

The PDM microphone and SD card slot are both on the Sense expansion board. No extra wiring is needed.

---

## Background: Digital Audio Fundamentals

### How PDM Microphones Work

A **PDM (Pulse Density Modulation)** microphone is a MEMS (Micro-Electro-Mechanical Systems) device that converts sound pressure waves into a stream of 1-bit digital pulses. The density of "1" pulses in the stream is proportional to the instantaneous sound pressure:

```
Loud sound:  1 1 1 0 1 1 1 0 1 1 0 1 1 1 0 1   (many 1s)
Quiet sound: 0 0 1 0 0 0 1 0 0 0 0 1 0 0 1 0   (few 1s)
Silence:     0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1   (balanced)
```

The ESP32-S3's I2S peripheral can receive this PDM bitstream and convert it to standard PCM (Pulse Code Modulation) audio samples using a built-in decimation filter.

### PDM Microphone Pins on XIAO ESP32-S3 Sense

```
Signal    GPIO    Description
──────    ────    ───────────
CLK       42      Clock output from ESP32 to microphone
DATA      41      PDM data input from microphone to ESP32
```

The clock pin provides the sampling clock to the microphone. The microphone returns its PDM bitstream on the data pin, synchronized to this clock.

### The I2S Interface

**I2S** (Inter-IC Sound) is a serial bus protocol designed specifically for digital audio data. On the ESP32-S3, the I2S peripheral can operate in several modes:

- **Standard I2S** — for I2S codec chips
- **PDM RX** — for receiving PDM microphone data (what we use)
- **PDM TX** — for driving PDM speakers/DACs

In PDM RX mode, the ESP32-S3 generates the clock on GPIO 42 and reads the PDM bitstream on GPIO 41. The hardware decimation filter converts the 1-bit PDM stream into 16-bit PCM samples.

### Sample Rates and the Nyquist Theorem

The **Nyquist theorem** states that to accurately capture a sound frequency, you must sample at least twice that frequency. Human hearing ranges from 20 Hz to 20,000 Hz, so:

| Sample Rate | Max Frequency | Quality Level |
|-------------|--------------|---------------|
| 8,000 Hz | 4,000 Hz | Telephone quality |
| 16,000 Hz | 8,000 Hz | Voice recording (good) |
| 22,050 Hz | 11,025 Hz | AM radio quality |
| 44,100 Hz | 22,050 Hz | CD quality |

For voice recording on a microcontroller, **16,000 Hz** is a good default. It captures clear speech while keeping file sizes manageable.

### The WAV File Format

A WAV file is one of the simplest audio file formats. It consists of a 44-byte header followed by raw PCM audio data:

```
Offset  Size  Field               Value
──────  ────  ─────               ─────
0       4     ChunkID             "RIFF"
4       4     ChunkSize           File size - 8
8       4     Format              "WAVE"
12      4     Subchunk1ID         "fmt "
16      4     Subchunk1Size       16 (for PCM)
20      2     AudioFormat         1 (PCM)
22      2     NumChannels         1 (mono)
24      4     SampleRate          16000
28      4     ByteRate            SampleRate * NumChannels * BitsPerSample/8
32      2     BlockAlign          NumChannels * BitsPerSample/8
34      2     BitsPerSample       16
36      4     Subchunk2ID         "data"
40      4     Subchunk2Size       NumSamples * NumChannels * BitsPerSample/8
44      ...   Data                Raw PCM samples
```

The beauty of WAV is its simplicity: the header is a fixed structure, and the data is just raw samples in sequence. Any audio player on any operating system can play WAV files.

---

## Step-by-Step Instructions

### 1. Format the SD Card

1. Insert the microSD card into your computer.
2. Format it as **FAT32** (use the SD Card Formatter tool for best results).
3. Insert it into the SD card slot on the Sense expansion board.

### 2. Configure the Sketch

Open `audio-recorder.ino`. The default settings work well:

```cpp
#define SAMPLE_RATE     16000   // 16 kHz — good for voice
#define SAMPLE_BITS     16      // 16-bit depth
#define RECORD_SECONDS  5       // Default recording duration
```

### 3. Upload the Sketch

1. Select **XIAO_ESP32S3** as the board.
2. Select `/dev/cu.usbmodem101` as the port.
3. Click **Upload**.

### 4. Open the Serial Monitor

Set baud rate to **115200**. You should see:

```
=== Lesson 14: Audio Recorder ===

SD card mounted. Type: SDHC
SD card size: 14832 MB
I2S PDM microphone initialized (16000 Hz, 16-bit)

Commands:
  record        - Record for 5 seconds (default)
  record 10     - Record for 10 seconds
  list          - List all recordings
  delete <file> - Delete a recording
  info          - Show SD card info
```

### 5. Make a Recording

Type `record` in the serial monitor and press Enter. You will see:

```
Recording for 5 seconds...
Progress: [##########] 100%
Saved: /recording_001.wav (160044 bytes, 5.0 sec)
```

### 6. Listen to the Recording

Remove the SD card and insert it into your computer. Open the WAV file with any audio player (VLC, QuickTime, Windows Media Player). You should hear the audio captured by the microphone.

---

## The Code

### I2S Configuration

The `setupI2S()` function configures the I2S peripheral in PDM RX mode. Key settings include the sample rate (16 kHz), bit depth (16-bit), and pin assignments (CLK on GPIO 42, DATA on GPIO 41).

### WAV Header Generation

The `writeWavHeader()` function constructs the 44-byte WAV header. It calculates the data size from the recording duration and sample rate, then writes each field in the correct byte order (little-endian for WAV).

### Recording Loop

The `recordAudio()` function:
1. Generates a unique filename (recording_001.wav, recording_002.wav, etc.).
2. Opens the file on the SD card and writes the WAV header.
3. Reads audio data from I2S in chunks using `i2s_channel_read()`.
4. Writes each chunk to the SD card.
5. Prints a progress bar to the serial monitor.
6. Closes the file when the recording duration is reached.

### Serial Command Parser

The `loop()` function reads serial input and parses simple commands. The `record` command accepts an optional duration parameter.

---

## How It Works

1. The ESP32-S3 generates a clock signal on GPIO 42, driving the PDM microphone at the configured rate.
2. The microphone converts sound pressure into a PDM bitstream on GPIO 41.
3. The I2S peripheral's decimation filter converts the 1-bit PDM stream into 16-bit PCM samples.
4. The firmware reads PCM samples from the I2S DMA buffer in chunks (typically 1024 bytes at a time).
5. Each chunk is written directly to the SD card as raw audio data.
6. The WAV header (written first with estimated size) makes the file playable by any audio software.

---

## Try It Out

- [ ] SD card is detected and information is printed to serial
- [ ] Typing `record` creates a WAV file on the SD card
- [ ] The recording plays back clearly on a computer
- [ ] The `list` command shows all recordings with file sizes
- [ ] Recording for different durations works (e.g., `record 10`)
- [ ] The progress bar updates smoothly during recording

---

## Challenges

1. **Add a clap-to-record trigger** — Instead of a serial command, monitor the audio amplitude continuously. When it exceeds a threshold (a clap or loud sound), automatically start recording for a set duration.
2. **Implement a simple volume meter** — Read audio samples continuously and calculate the RMS (root mean square) amplitude. Print a visual bar graph to the serial monitor that updates in real time.
3. **Add playback over serial** — Read a WAV file from the SD card and stream the raw PCM data over serial at the correct byte rate. A Python script on the computer could receive and play it.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "SD card mount failed" | Ensure the card is FAT32 formatted. Try reformatting. Check that the card is fully inserted into the Sense board's slot. |
| "I2S init failed" | Make sure no other sketch or library is using the I2S peripheral. The ESP32-S3 has limited I2S channels. |
| Recording plays but sounds distorted | Check the sample rate in the code matches the WAV header. Mismatched rates cause pitch shifting. |
| Recording is silent | The PDM microphone is on the bottom of the Sense expansion board. Ensure it is not blocked. Speak loudly and close to the board. |
| File is created but 0 bytes | The SD card may be write-protected or full. Check `SD.totalBytes()` and `SD.usedBytes()`. |
| Compile error about I2S | The ESP32-S3 Arduino core 3.x uses a new I2S driver. Make sure your esp32 board package is up to date. |

---

## What's Next

In [Lesson 15: Deep Sleep & Battery](../15-deep-sleep-battery/), you will learn how to put the ESP32-S3 into ultra-low-power deep sleep mode and wake it with a timer or button — essential knowledge for battery-powered IoT projects.
