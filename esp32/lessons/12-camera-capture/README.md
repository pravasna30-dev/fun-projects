# Lesson 12: Camera Capture

**Difficulty:** Intermediate ![Intermediate](https://img.shields.io/badge/level-intermediate-yellow)

**Estimated Time:** 60-90 minutes

---

## What You'll Learn

- How the OV2640 camera module connects to the ESP32-S3 on the Sense expansion board
- How to configure camera pins specific to the XIAO ESP32-S3 Sense
- How to initialize the camera using the `esp_camera` library
- What a frame buffer is and how JPEG capture works
- How to serve a captured JPEG image through a web server in the browser
- How PSRAM is used for storing camera frame buffers
- How frame size and JPEG quality affect image output

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Must have the Sense expansion board attached (includes OV2640 camera) |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |
| WiFi network | 1 | 2.4 GHz, same network as your computer |

The camera is built into the Sense expansion board. No extra wiring is needed.

---

## Background: The OV2640 Camera

The **OV2640** is a 2-megapixel CMOS image sensor commonly used in embedded systems. It can output JPEG-compressed images directly, which saves the ESP32 from having to compress raw pixel data. Key specs:

- Resolution: up to 1600x1200 (UXGA)
- Output formats: JPEG, RGB565, YUV422
- Built-in JPEG encoder
- Interface: 8-bit parallel data bus + sync signals

### How the Camera Connects to the ESP32-S3

The camera uses a parallel interface with the following signal groups:

| Signal Group | Pins | Purpose |
|-------------|------|---------|
| Data bus (Y2-Y9) | 8 GPIO pins | Pixel data, 8 bits at a time |
| XCLK | 1 GPIO | Master clock from ESP32 to camera (typically 20 MHz) |
| PCLK | 1 GPIO | Pixel clock from camera to ESP32 |
| VSYNC | 1 GPIO | Vertical sync — signals the start of a new frame |
| HREF | 1 GPIO | Horizontal reference — signals valid pixel data on a row |
| SIOD/SIOC | 2 GPIO | I2C bus for configuring camera registers (SCCB protocol) |
| PWDN/RESET | 2 GPIO | Power down and reset (not used on XIAO, set to -1) |

### XIAO ESP32-S3 Sense Camera Pin Map

```
Camera Pin    GPIO    Description
──────────    ────    ───────────
PWDN          -1      Not connected (always on)
RESET         -1      Not connected (software reset used)
XCLK          10      Master clock output
SIOD          40      I2C data (SCCB)
SIOC          39      I2C clock (SCCB)
Y9            48      Data bit 7 (MSB)
Y8            11      Data bit 6
Y7            12      Data bit 5
Y6            14      Data bit 4
Y5            16      Data bit 3
Y4            18      Data bit 2
Y3            17      Data bit 1
Y2            15      Data bit 0 (LSB)
VSYNC         38      Vertical sync
HREF          47      Horizontal reference
PCLK          13      Pixel clock
```

### PSRAM and Frame Buffers

The XIAO ESP32-S3 has **8 MB of PSRAM** (pseudo-static RAM). This is critical for camera operation because a single UXGA frame in RGB565 format requires 1600 x 1200 x 2 = 3.84 MB. Even JPEG frames at high resolution can be hundreds of kilobytes. The ESP32's internal SRAM (about 520 KB) is far too small, so frame buffers are allocated in PSRAM.

### Frame Sizes

The `esp_camera` library defines these common frame sizes:

| Constant | Resolution | Use Case |
|----------|-----------|----------|
| FRAMESIZE_QQVGA | 160x120 | Tiny thumbnails |
| FRAMESIZE_QVGA | 320x240 | Small previews |
| FRAMESIZE_VGA | 640x480 | Good balance of speed and quality |
| FRAMESIZE_SVGA | 800x600 | Higher detail |
| FRAMESIZE_XGA | 1024x768 | High resolution |
| FRAMESIZE_SXGA | 1280x1024 | Near maximum |
| FRAMESIZE_UXGA | 1600x1200 | Maximum resolution |

Larger frames take longer to capture and transmit. For web serving, VGA or SVGA is a good default.

---

## Step-by-Step Instructions

### 1. Enable PSRAM in Board Settings

1. In Arduino IDE, go to **Tools > Board** and select **XIAO_ESP32S3**.
2. Go to **Tools > PSRAM** and select **OPI PSRAM** (this enables the 8 MB PSRAM).

> **Important:** If PSRAM is not enabled, the camera will fail to allocate frame buffers and initialization will fail.

### 2. Configure WiFi Credentials

Open `camera-capture.ino` and replace the placeholders:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

### 3. Upload the Sketch

1. Select **XIAO_ESP32S3** as the board.
2. Select `/dev/cu.usbmodem101` as the port.
3. Ensure **PSRAM: OPI PSRAM** is selected.
4. Click **Upload**.

### 4. Open the Serial Monitor

Set baud rate to **115200**. You should see:

```
Camera initialized successfully!
Resolution: 640x480
Connecting to WiFi...
WiFi connected! IP: 192.168.1.42
Camera server started!
  http://192.168.1.42/capture  - Take a photo
  http://192.168.1.42/info     - Camera info
```

### 5. Capture a Photo

Open a web browser on the same WiFi network and navigate to:

```
http://<your-esp32-ip>/capture
```

A JPEG image should appear in your browser. Refresh the page to take a new photo.

### 6. Check Camera Info

Navigate to:

```
http://<your-esp32-ip>/info
```

This shows a JSON response with camera details like resolution, JPEG quality, and PSRAM status.

---

## The Code

### Camera Pin Configuration

The `camera_config_t` structure maps every camera signal to the correct GPIO pin on the XIAO ESP32-S3 Sense. Getting even one pin wrong will cause initialization to fail with a cryptic error.

### Camera Initialization

The `initCamera()` function sets up the camera configuration (pins, clock frequency, pixel format, frame size, JPEG quality, frame buffer count) and calls `esp_camera_init()`. If PSRAM is detected, it uses a higher resolution and allocates two frame buffers for better throughput.

### Web Server Handlers

The sketch creates three URL handlers:

- **`/capture`** — Captures a single JPEG frame and sends it to the browser with the `Content-Type: image/jpeg` header.
- **`/info`** — Returns a JSON object with camera settings and system info.
- **`/`** — A simple HTML page with a link to capture and an auto-refreshing image tag.

### Frame Buffer Management

Each call to `esp_camera_fb_get()` acquires a frame buffer. It is critical to release it with `esp_camera_fb_return()` when done, or the system will run out of buffers and hang.

---

## How It Works

1. At startup, the ESP32-S3 configures the camera hardware through the SCCB (I2C) interface, setting resolution, format, and quality.
2. The `XCLK` pin provides a 20 MHz clock to the camera, which drives the pixel output.
3. When a frame is requested, the camera fills the data bus (Y2-Y9) pixel by pixel, synchronized by `PCLK`, `HREF`, and `VSYNC`.
4. The ESP32-S3's LCD_CAM peripheral (a DMA-capable camera interface) captures the data directly into PSRAM without CPU involvement.
5. The `esp_camera` library provides the captured frame as a pointer to a JPEG blob in memory.
6. The web server sends this blob to the browser with the appropriate MIME type.

---

## Try It Out

- [ ] Serial monitor shows "Camera initialized successfully!" with no errors
- [ ] Navigating to `/capture` displays a JPEG photo in the browser
- [ ] Refreshing the page shows a new (different) photo
- [ ] The `/info` endpoint returns valid camera information
- [ ] The root `/` page shows an HTML interface with an embedded image

---

## Challenges

1. **Add a resolution selector** — Create additional endpoints like `/capture?res=uxga` and `/capture?res=vga` that change the frame size before capturing. Remember to reinitialize the camera or change sensor settings.
2. **Save to SD card** — Combine this lesson with Lesson 9 (SD card). Add a `/save` endpoint that captures a photo and saves it to the SD card with a timestamped filename.
3. **Auto-capture timelapse** — Add a mode that automatically captures a photo every N seconds and saves a numbered sequence to the SD card. This could be used for a timelapse video.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Camera init failed with error 0x105" | PSRAM is not enabled. Go to Tools > PSRAM > OPI PSRAM and re-upload. |
| "Camera init failed with error 0x103" | Check that the Sense expansion board is properly seated on the XIAO. The camera ribbon cable may be loose. |
| Brown/garbled image | The XCLK frequency may be too high. Try reducing `xclk_freq_hz` from 20000000 to 10000000. |
| Image is very dark | The camera needs a moment to adjust auto-exposure. Discard the first 2-3 frames before serving. |
| "No PSRAM found" warning | Ensure the board selected is XIAO_ESP32S3 and PSRAM is set to OPI PSRAM. |
| Browser shows broken image icon | Check the serial monitor for errors. The frame buffer may have failed to allocate. |

---

## What's Next

In [Lesson 13: Camera Live Stream](../13-camera-stream/), you will upgrade from single captures to a smooth MJPEG video stream viewable in any web browser.
