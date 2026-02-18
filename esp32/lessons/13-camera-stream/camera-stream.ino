/*
 * Lesson 13: Camera Live Stream (MJPEG)
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch creates a web server with three endpoints:
 *   /        — HTML dashboard with embedded live stream
 *   /capture — Single JPEG snapshot
 *   /stream  — MJPEG live stream (multipart HTTP response)
 *
 * Prerequisites:
 *   - PSRAM must be enabled: Tools > PSRAM > OPI PSRAM
 *   - Sense expansion board must be attached (includes camera)
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ─── WiFi Configuration ─────────────────────────────────────────────────────

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ─── Camera Pin Definitions for XIAO ESP32-S3 Sense ─────────────────────────

#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   10
#define SIOD_GPIO_NUM   40
#define SIOC_GPIO_NUM   39
#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15
#define VSYNC_GPIO_NUM  38
#define HREF_GPIO_NUM   47
#define PCLK_GPIO_NUM   13

#define LED_PIN 21

// ─── MJPEG Stream Constants ─────────────────────────────────────────────────

// The boundary string separates each JPEG frame in the multipart response.
// It can be any unique string — "frame" is conventional.
#define BOUNDARY       "frame"
#define STREAM_CONTENT_TYPE  "multipart/x-mixed-replace;boundary=" BOUNDARY
#define STREAM_BOUNDARY      "\r\n--" BOUNDARY "\r\n"
#define STREAM_PART          "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

// ─── Web Server ─────────────────────────────────────────────────────────────

WebServer server(80);

// ─── Frame rate tracking ────────────────────────────────────────────────────

unsigned long lastFPSTime = 0;
int frameCount = 0;
float currentFPS = 0;

// ─── Camera Initialization ──────────────────────────────────────────────────

bool initCamera() {
  camera_config_t config;

  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.ledc_timer    = LEDC_TIMER_0;
  config.ledc_channel  = LEDC_CHANNEL_0;
  config.pixel_format  = PIXFORMAT_JPEG;

  if (psramFound()) {
    Serial.println("PSRAM found! Using VGA with double buffering.");
    config.frame_size   = FRAMESIZE_VGA;      // 640x480
    config.jpeg_quality = 12;                  // Slightly lower quality for speed
    config.fb_count     = 2;                   // Two buffers for smooth streaming
    config.grab_mode    = CAMERA_GRAB_LATEST;  // Always get the newest frame
  } else {
    Serial.println("WARNING: No PSRAM. Falling back to QVGA.");
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Fine-tune sensor settings for streaming
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_brightness(s, 1);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
  }

  return true;
}

// ─── Handler: MJPEG Stream ──────────────────────────────────────────────────
// This is the core of the lesson. It sends a continuous stream of JPEG frames
// using the multipart/x-mixed-replace HTTP content type.

void handleStream() {
  Serial.println("Stream client connected!");

  // Get the underlying WiFiClient to write raw data
  WiFiClient client = server.client();

  // Send the HTTP response header with the multipart content type
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: " STREAM_CONTENT_TYPE "\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "\r\n";
  client.print(response);

  // Stream frames as long as the client is connected
  while (client.connected()) {
    // Capture a frame from the camera
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Stream: frame capture failed, skipping");
      delay(10);
      continue;
    }

    // Send the boundary that separates frames
    client.print(STREAM_BOUNDARY);

    // Send the part header with Content-Type and Content-Length
    char partHeader[64];
    snprintf(partHeader, sizeof(partHeader), STREAM_PART, fb->len);
    client.print(partHeader);

    // Send the actual JPEG image data
    // write() returns bytes written; we check for send failures
    size_t written = client.write(fb->buf, fb->len);

    // Return the frame buffer to the pool immediately
    esp_camera_fb_return(fb);

    // If write failed (client disconnected), break out of the loop
    if (written == 0) {
      Serial.println("Stream: client disconnected (write failed)");
      break;
    }

    // Track frame rate
    frameCount++;
    unsigned long now = millis();
    if (now - lastFPSTime >= 3000) {
      currentFPS = frameCount * 1000.0 / (now - lastFPSTime);
      Serial.printf("Stream FPS: %.1f (free heap: %u bytes)\n",
                    currentFPS, ESP.getFreeHeap());
      frameCount = 0;
      lastFPSTime = now;
    }

    // Small delay to prevent watchdog timer reset
    // and allow other tasks to run (WiFi stack, etc.)
    delay(1);
  }

  Serial.println("Stream client disconnected.");
}

// ─── Handler: Single Capture ────────────────────────────────────────────────

void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ─── Handler: HTML Dashboard ────────────────────────────────────────────────

void handleRoot() {
  // Build a nice HTML page with the stream embedded
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>XIAO ESP32-S3 Live Stream</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: #0f0f23;
      color: #e0e0e0;
      min-height: 100vh;
    }
    .header {
      background: linear-gradient(135deg, #1a1a3e, #2d1b4e);
      padding: 20px;
      text-align: center;
      border-bottom: 2px solid #e94560;
    }
    .header h1 { color: #e94560; font-size: 24px; }
    .header p { color: #999; font-size: 14px; margin-top: 5px; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    .stream-box {
      background: #1a1a2e;
      border: 2px solid #333;
      border-radius: 12px;
      overflow: hidden;
      margin: 20px 0;
    }
    .stream-box img {
      width: 100%;
      display: block;
    }
    .controls {
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
      margin: 15px 0;
    }
    .btn {
      padding: 10px 20px;
      border: none;
      border-radius: 6px;
      font-size: 14px;
      cursor: pointer;
      text-decoration: none;
      color: white;
      transition: transform 0.1s;
    }
    .btn:hover { transform: scale(1.05); }
    .btn-primary { background: #e94560; }
    .btn-secondary { background: #3a3a5c; }
    .info {
      background: #1a1a2e;
      border: 1px solid #333;
      border-radius: 8px;
      padding: 15px;
      font-size: 13px;
      color: #888;
    }
    .info code { color: #e94560; }
    .status {
      display: inline-block;
      width: 10px; height: 10px;
      background: #4caf50;
      border-radius: 50%;
      margin-right: 5px;
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.4; }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>XIAO ESP32-S3 Live Stream</h1>
    <p><span class="status"></span>Streaming from OV2640 Camera</p>
  </div>
  <div class="container">
    <div class="stream-box">
      <img src="/stream" alt="Live camera stream" />
    </div>
    <div class="controls">
      <a class="btn btn-primary" href="/capture" target="_blank">Take Snapshot</a>
      <a class="btn btn-secondary" href="/" >Reload Page</a>
    </div>
    <div class="info">
      <p><strong>Endpoints:</strong></p>
      <ul style="margin: 8px 0 0 20px;">
        <li><code>/stream</code> &mdash; Raw MJPEG stream (embed in &lt;img&gt; tags)</li>
        <li><code>/capture</code> &mdash; Single JPEG snapshot</li>
      </ul>
    </div>
  </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ─── WiFi Setup ─────────────────────────────────────────────────────────────

void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
}

// ─── Setup ──────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Lesson 13: Camera Live Stream ===");
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    Serial.println("FATAL: Camera init failed. Halting.");
    while (true) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW);  delay(100);
    }
  }
  Serial.println("Camera initialized successfully!");

  // Connect to WiFi
  setupWiFi();

  // Register URL handlers
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/stream", handleStream);

  // Start the web server
  server.begin();

  Serial.println();
  Serial.println("Stream server started!");
  Serial.print("  http://");
  Serial.print(WiFi.localIP());
  Serial.println("/         - Dashboard");
  Serial.print("  http://");
  Serial.print(WiFi.localIP());
  Serial.println("/capture  - Single photo");
  Serial.print("  http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream   - MJPEG stream");

  // Flash LED to indicate ready
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
}

// ─── Loop ───────────────────────────────────────────────────────────────────

void loop() {
  server.handleClient();
  delay(1); // Yield to other tasks
}
