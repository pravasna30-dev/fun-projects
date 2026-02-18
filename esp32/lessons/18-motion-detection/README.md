# Lesson 18: Motion Detection Camera

![Difficulty: Expert](https://img.shields.io/badge/Difficulty-Expert-red)

**Estimated Time: 90-120 minutes**

---

## What You'll Learn

- Frame differencing for motion detection
- Pixel-by-pixel image comparison algorithms
- Configuring the OV2640 camera for grayscale and low resolution
- Saving JPEG captures to an SD card upon motion events
- Reducing false positives with threshold tuning and pixel counting
- Building a web interface to monitor detections

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| XIAO ESP32S3 Sense | 1 | Must have the Sense expansion board attached (camera + SD card) |
| MicroSD card | 1 | Formatted FAT32, for storing captured images |
| USB-C cable | 1 | For programming and power |
| WiFi network | 1 | For the web monitoring interface |

> **No extra wiring!** The camera and SD card are built into the Sense expansion board.

## Circuit Diagram

```
No external wiring required.

   +-------------------------------+
   |   XIAO ESP32S3 Sense Board    |
   |                               |
   |  [OV2640 Camera]   [SD Slot]  |
   |  (front-facing)    (side)     |
   |                               |
   |  [PDM Microphone]             |
   |  (not used in this lesson)    |
   |                               |
   +----------[USB-C]-------------+

   Camera is permanently connected via the Sense expansion board.
   SD card slides into the slot on the side of the expansion board.

   Camera Pin Mapping (for reference, already wired on-board):
     PWDN  = -1 (not used)    RESET = -1 (not used)
     XCLK  = GPIO 10          SIOD  = GPIO 40
     SIOC  = GPIO 39          Y9    = GPIO 48
     Y8    = GPIO 11          Y7    = GPIO 12
     Y6    = GPIO 14          Y5    = GPIO 16
     Y4    = GPIO 18          Y3    = GPIO 17
     Y2    = GPIO 15          VSYNC = GPIO 38
     HREF  = GPIO 47          PCLK  = GPIO 13
```

---

## Step-by-Step Instructions

### Step 1: Understand Frame Differencing

Motion detection by frame differencing works by comparing two consecutive camera frames:

1. **Capture Frame A** (the "background" or "reference" frame)
2. **Wait a short interval**
3. **Capture Frame B** (the "current" frame)
4. **For each pixel**, calculate `|B - A|` (absolute difference)
5. **If the difference exceeds a threshold**, that pixel has "changed"
6. **If enough pixels changed**, motion is detected

```
Frame A (no motion):     Frame B (person enters):   Difference:
+------------------+    +------------------+        +------------------+
|                  |    |      ####        |        |      ####        |
|                  | => |      #  #        | =>     |      #  #        |
|                  |    |      ####        |        |      ####        |
+------------------+    +------------------+        +------------------+
                                                     Changed pixels > threshold
                                                     => MOTION DETECTED!
```

### Step 2: Why Grayscale and Low Resolution?

We configure the camera for **QQVGA (160x120) grayscale** for several reasons:

- **Speed** -- Smaller images process faster. At QQVGA, each frame is only 19,200 pixels vs. 921,600 for VGA.
- **Memory** -- Two frames at QQVGA grayscale = ~38 KB. Two frames at VGA RGB = ~5.5 MB.
- **Simplicity** -- Grayscale means one value per pixel (0-255 brightness), making comparison trivial.

When motion IS detected, we switch to a higher resolution for the actual capture (SVGA JPEG) to get a useful image.

### Step 3: Install Required Libraries

No additional libraries are needed beyond the ESP32 board package. The `esp_camera.h` library is included with the ESP32-S3 board support.

### Step 4: Prepare the SD Card

1. Format a MicroSD card as FAT32
2. Insert it into the Sense expansion board's SD slot
3. The sketch will create a `/captures` directory automatically

### Step 5: Upload and Configure

1. Open `motion-detection.ino`
2. Replace `YOUR_SSID` and `YOUR_PASSWORD` with your WiFi credentials
3. Select board **XIAO_ESP32S3** and port `/dev/cu.usbmodem101`
4. Set **PSRAM: "OPI PSRAM"** in the Tools menu (important for camera buffers)
5. Upload the sketch
6. Open Serial Monitor at 115200 baud

### Step 6: Tune Sensitivity

The sketch has two key parameters you can adjust:

- **`PIXEL_THRESHOLD`** (default: 30) -- How much a single pixel must change to be considered "different." Lower = more sensitive, but more false positives from noise.
- **`MOTION_PERCENT`** (default: 5.0) -- What percentage of pixels must change to trigger motion detection. Lower = more sensitive to small movements.

Start with the defaults and adjust based on your environment. A stable, well-lit scene needs lower thresholds. A scene with flickering lights or swaying trees needs higher thresholds.

---

## The Code

### Camera Initialization

```cpp
camera_config_t config;
config.frame_size = FRAMESIZE_QQVGA;  // 160x120
config.pixel_format = PIXFORMAT_GRAYSCALE;
```

We start in QQVGA grayscale for fast frame differencing. The camera configuration includes all the pin assignments specific to the XIAO ESP32S3 Sense board.

### Frame Differencing Algorithm

The core algorithm is straightforward:

```cpp
int changedPixels = 0;
for (int i = 0; i < frameSize; i++) {
    int diff = abs(currentFrame[i] - previousFrame[i]);
    if (diff > PIXEL_THRESHOLD) {
        changedPixels++;
    }
}
float percentChanged = (changedPixels * 100.0) / frameSize;
```

For each pixel position, we compute the absolute difference in brightness between the current and previous frame. If this difference exceeds `PIXEL_THRESHOLD`, that pixel is marked as "changed." If the total percentage of changed pixels exceeds `MOTION_PERCENT`, we declare motion.

### Capture and Save

When motion is detected, the sketch:

1. Reconfigures the camera to a higher resolution (SVGA, JPEG format)
2. Captures a single frame
3. Saves it to SD with a sequential filename (`/captures/motion_0001.jpg`)
4. Reconfigures back to QQVGA grayscale for continued monitoring

### Web Monitoring Interface

The web server provides:
- `/` -- Dashboard showing status, last detection time, and detection count
- `/latest` -- Serves the most recently captured JPEG image
- `/status` -- JSON endpoint for programmatic access

---

## How It Works

### Pixel Threshold vs. Motion Percent

These two parameters work together to filter out noise:

- **Pixel Threshold** handles per-pixel sensor noise. Camera sensors always have some random variation between frames (electronic noise). A threshold of 30 means a pixel must change by more than ~12% of its range to count.

- **Motion Percent** handles scene-level noise. Even with per-pixel thresholding, a few scattered pixels might trigger. Requiring 5% of all pixels to change ensures that only significant motion (a person, an animal, a door opening) triggers detection.

### False Positive Reduction Strategies

1. **Cooldown period** -- After a detection, ignore motion for a few seconds to avoid capturing the same event repeatedly.
2. **Minimum changed region** -- Instead of counting scattered pixels, require a connected region of change (more complex but more accurate).
3. **Reference frame adaptation** -- Gradually blend the reference frame toward the current frame to adapt to slow lighting changes (like sunrise/sunset).
4. **Noise filtering** -- Apply a simple box blur to the difference image before thresholding.

The sketch implements the cooldown period approach. The other strategies are described as challenges.

### Memory Considerations

The ESP32-S3 has 8MB PSRAM, which is essential for camera operations:

- Two QQVGA grayscale buffers: ~38 KB (fits in regular RAM)
- JPEG capture buffer: ~50-100 KB (allocated in PSRAM by the camera driver)
- Web server HTML: ~5 KB

The PSRAM is accessed via the OPI (Octal SPI) interface, which is fast enough for our purposes but slower than internal SRAM.

---

## Try It Out

1. Upload the sketch and confirm camera initialization in Serial Monitor
2. Point the camera at a stable scene (a desk, a wall)
3. Wait for the "Monitoring started" message
4. Walk in front of the camera -- you should see "MOTION DETECTED" in Serial Monitor
5. Check the SD card -- there should be a new JPEG in the `/captures` folder
6. Open the web dashboard and verify the detection log
7. Open `http://[IP]/latest` to see the most recent capture in your browser

---

## Challenges

1. **Adaptive reference frame** -- Instead of replacing the reference frame only when motion is NOT detected, implement exponential moving average blending: `reference = alpha * current + (1 - alpha) * reference`. This lets the system adapt to gradual lighting changes while still detecting sudden motion.

2. **Motion region highlighting** -- Modify the capture to overlay a bounding box around the area where motion was detected. You would need to track the min/max x,y coordinates of changed pixels and draw a rectangle on the JPEG (or serve an HTML page that overlays the region).

3. **Time-lapse mode** -- Add a secondary mode that captures a frame every N minutes regardless of motion. Store these alongside the motion captures with different filenames. This creates a time-lapse you can assemble into a video on your computer.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Camera init fails | Ensure the Sense expansion board is properly seated. Check that PSRAM is enabled in Tools menu ("OPI PSRAM"). |
| Constant false detections | Increase `PIXEL_THRESHOLD` (try 40-50) or `MOTION_PERCENT` (try 10-15). Avoid pointing the camera at flickering lights or moving curtains. |
| No detections at all | Decrease thresholds. Check Serial Monitor for frame capture errors. Ensure the camera lens cap is removed. |
| SD card write fails | Verify FAT32 format. Try a different card. Check that the card is not write-protected. |
| Images are dark or overexposed | The camera auto-exposure may need time to adjust. Add a warm-up period of 5-10 frames after initialization. |
| Web page shows no image | The `/latest` endpoint only works after the first detection. Trigger motion first, then check. |
| Sketch won't upload (too large) | Select a partition scheme with more app space. Ensure you are not accidentally including extra libraries. |

---

## What's Next

In [Lesson 19: TinyML Gesture Recognition](../19-tinyml-gesture/), we will explore machine learning on microcontrollers by building a gesture recognition system using an accelerometer and the TensorFlow Lite Micro framework -- bringing AI to the edge.
