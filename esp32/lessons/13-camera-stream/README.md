# Lesson 13: Camera Live Stream

**Difficulty:** Advanced ![Advanced](https://img.shields.io/badge/level-advanced-red)

**Estimated Time:** 75-90 minutes

---

## What You'll Learn

- What MJPEG streaming is and how it differs from modern video codecs like H.264
- How the HTTP multipart content type enables continuous image delivery
- How to build a web server with dual endpoints: still capture and live stream
- How to create a responsive HTML dashboard with an embedded live stream
- How to optimize frame rate and image quality for smooth streaming
- How the ESP32-S3's dual cores and PSRAM make real-time streaming possible

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Must have Sense expansion board with OV2640 camera |
| USB-C cable | 1 | Data-capable (not charge-only) |
| Computer with Arduino IDE | 1 | Version 2.x recommended |
| WiFi network | 1 | 2.4 GHz, same network as your computer |

No extra hardware is needed. The camera is built into the Sense expansion board.

---

## Background: How Video Streaming Works on a Microcontroller

### The Challenge

True video streaming (like YouTube or a Zoom call) uses sophisticated codecs like H.264 or H.265 that compress video by analyzing motion between frames (temporal compression). The ESP32-S3 does not have a hardware H.264 encoder, and doing it in software would be far too slow.

### The MJPEG Solution

**MJPEG** (Motion JPEG) is the simplest form of video streaming. Instead of encoding motion between frames, it simply sends a rapid sequence of individual JPEG images. Each frame is independently compressed — no inter-frame prediction, no B-frames, no motion vectors.

```
Frame 1 (JPEG) → Frame 2 (JPEG) → Frame 3 (JPEG) → ...
```

Pros:
- Every frame is a complete image (easy to decode, no artifacts from lost frames)
- The OV2640 camera outputs JPEG natively — no encoding work for the ESP32
- All modern browsers can render MJPEG streams natively
- Low latency (typically under 200ms)

Cons:
- Much larger bandwidth than H.264 (no temporal compression)
- Not suitable for recording (files would be huge)
- Frame rate limited by WiFi throughput and camera capture speed

### Multipart HTTP Response

MJPEG streaming uses a special HTTP content type called `multipart/x-mixed-replace`. It works like this:

```
HTTP/1.1 200 OK
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg
Content-Length: 12345

<JPEG bytes for frame 1>
--frame
Content-Type: image/jpeg
Content-Length: 12346

<JPEG bytes for frame 2>
--frame
...
```

The `boundary=frame` tells the browser: "Each part is separated by `--frame`." The browser replaces the previous image with each new part, creating the illusion of video.

### MJPEG vs H.264 Comparison

| Feature | MJPEG | H.264 |
|---------|-------|-------|
| Compression | Per-frame (spatial only) | Inter-frame (spatial + temporal) |
| Bandwidth | High (~2-5 Mbps at VGA) | Low (~0.5-1 Mbps at VGA) |
| Latency | Very low (<200ms) | Low to medium (depends on GOP) |
| CPU load on ESP32 | Minimal (camera does JPEG) | Impossible (no HW encoder) |
| Browser support | Universal (via multipart) | Requires JavaScript player |
| Seek/random access | Every frame is a keyframe | Only at keyframes |

---

## Step-by-Step Instructions

### 1. Enable PSRAM

In Arduino IDE, go to **Tools > PSRAM** and select **OPI PSRAM**. This is required for camera frame buffers.

### 2. Configure WiFi

Open `camera-stream.ino` and replace the placeholders:

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
WiFi connected! IP: 192.168.1.42
Stream server started!
  http://192.168.1.42/         - Dashboard
  http://192.168.1.42/capture  - Single photo
  http://192.168.1.42/stream   - MJPEG stream
```

### 5. View the Live Stream

Open a browser and go to `http://<your-esp32-ip>/`. You will see an HTML dashboard page with the embedded live stream. The stream should run at approximately 10-15 FPS at VGA resolution.

### 6. Try Individual Endpoints

- **`/capture`** — Returns a single JPEG snapshot (useful for integrations).
- **`/stream`** — The raw MJPEG stream (can be opened directly or embedded in `<img>` tags).

---

## The Code

### Camera Configuration

The camera is configured identically to Lesson 12, using the same pin definitions for the XIAO ESP32-S3 Sense. The key difference is that we use `CAMERA_GRAB_LATEST` mode with 2 frame buffers to ensure we always stream the most recent frame.

### The Stream Handler

The `handleStream()` function is the heart of this lesson. It:

1. Sets the HTTP response content type to `multipart/x-mixed-replace` with a boundary string.
2. Enters a loop that runs as long as the client is connected.
3. In each iteration: captures a frame, writes the boundary and JPEG headers, sends the JPEG bytes, and releases the frame buffer.
4. A small delay between frames prevents the watchdog timer from triggering.

### The HTML Dashboard

The root `/` handler serves a styled HTML page that embeds the stream using a simple `<img>` tag:

```html
<img src="/stream" />
```

The browser handles all the MJPEG decoding automatically. The page also includes buttons for taking snapshots and displays connection info.

### Frame Rate Tracking

The sketch tracks frames per second (FPS) and prints it to the serial monitor every 30 frames. This helps you optimize settings. Typical rates:

- QQVGA (160x120): 20-25 FPS
- QVGA (320x240): 15-20 FPS
- VGA (640x480): 10-15 FPS
- SVGA (800x600): 5-8 FPS

---

## How It Works

1. The camera continuously captures JPEG frames into PSRAM-backed frame buffers.
2. When a browser connects to `/stream`, the ESP32 starts sending frames in a multipart HTTP response.
3. Each frame is preceded by a boundary marker and content headers.
4. The browser replaces the displayed image with each new frame, creating smooth video.
5. The `WiFiClient` object tracks the TCP connection — when the browser disconnects (tab closed), the streaming loop exits.
6. Multiple clients can connect to `/capture` independently, but the MJPEG stream is best served to one client at a time (bandwidth limitation).

---

## Try It Out

- [ ] The dashboard page loads at `/` with a styled interface
- [ ] The live stream is smooth and updates continuously
- [ ] The serial monitor shows FPS readings (aim for 10+ at VGA)
- [ ] The `/capture` endpoint returns a single JPEG snapshot
- [ ] Closing the browser tab stops the stream (check serial monitor)
- [ ] Reopening the page restarts the stream cleanly

---

## Challenges

1. **Add resolution controls** — Add buttons to the HTML page that change the camera resolution on the fly. Create endpoints like `/set?framesize=vga` that reconfigure the sensor and redirect back to the dashboard.
2. **Add a frame counter overlay** — Use the sensor's built-in text overlay feature (`s->set_denoise()` and similar) or add a frame counter to the HTML page using JavaScript that increments with each received frame.
3. **Pan and tilt simulation** — Add buttons to the dashboard that adjust the camera's crop window (digital zoom/pan) using `s->set_hmirror()`, `s->set_vflip()`, and window offset registers.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Stream loads for a second then freezes | The frame buffer may not be returned properly. Ensure `esp_camera_fb_return(fb)` is called in every code path, including error paths. |
| Very low frame rate (< 5 FPS) | Reduce resolution to QVGA. Check WiFi signal strength — poor WiFi is the most common bottleneck. |
| Browser shows "connection reset" | The ESP32 may have run out of memory. Monitor free heap in the serial output. Reduce frame size or JPEG quality. |
| Stream works in Chrome but not Safari | Safari handles MJPEG differently. Ensure the boundary string has no extra whitespace and Content-Length is included for each part. |
| Camera init fails | Ensure PSRAM is enabled (Tools > PSRAM > OPI PSRAM) and the Sense expansion board is properly connected. |
| Image is rotated or flipped | Use `s->set_vflip(s, 1)` or `s->set_hmirror(s, 1)` in the camera init to correct orientation. |

---

## What's Next

In [Lesson 14: Audio Recorder](../14-audio-recorder/), you will use the built-in PDM microphone and SD card to record audio as WAV files — combining digital audio concepts with file I/O.
