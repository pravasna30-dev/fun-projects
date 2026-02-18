/*
 * Lesson 18: Motion Detection Camera
 *
 * Board: Seeed Studio XIAO ESP32S3 Sense
 *
 * This sketch uses the onboard OV2640 camera to detect motion via
 * frame differencing. When motion is detected, it captures a higher
 * resolution JPEG photo and saves it to the SD card.
 *
 * A web interface shows the detection log and the latest captured image.
 *
 * Algorithm:
 * 1. Capture frames at QQVGA (160x120) in grayscale for speed
 * 2. Compare consecutive frames pixel-by-pixel
 * 3. If enough pixels changed significantly, declare motion
 * 4. Switch to SVGA JPEG, capture, save to SD, switch back
 *
 * Important: Enable PSRAM in Arduino IDE -> Tools -> PSRAM: "OPI PSRAM"
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>

// ============================================================
// Configuration
// ============================================================

// WiFi credentials
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD  = "YOUR_PASSWORD";

// --- Camera pin definitions for XIAO ESP32S3 Sense ---
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    10
#define SIOD_GPIO_NUM    40
#define SIOC_GPIO_NUM    39
#define Y9_GPIO_NUM      48
#define Y8_GPIO_NUM      11
#define Y7_GPIO_NUM      12
#define Y6_GPIO_NUM      14
#define Y5_GPIO_NUM      16
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM      17
#define Y2_GPIO_NUM      15
#define VSYNC_GPIO_NUM   38
#define HREF_GPIO_NUM    47
#define PCLK_GPIO_NUM    13

// SD card chip select
#define SD_CS 21

// --- Motion detection parameters ---
// How much a single pixel must change to be considered "different"
// Range: 1-255. Higher = less sensitive. Start with 30.
#define PIXEL_THRESHOLD  30

// What percentage of pixels must change to trigger motion
// Range: 0.1-100.0. Higher = less sensitive. Start with 5.0.
#define MOTION_PERCENT   5.0

// Cooldown period after a detection (milliseconds)
// Prevents capturing the same event multiple times
#define COOLDOWN_MS      5000

// Number of warm-up frames to skip after camera init
// (auto-exposure needs time to stabilize)
#define WARMUP_FRAMES    10

// ============================================================
// Global Variables
// ============================================================

WebServer server(80);

// Frame buffers for comparison (QQVGA 160x120 = 19200 bytes)
#define FRAME_WIDTH  160
#define FRAME_HEIGHT 120
#define FRAME_SIZE   (FRAME_WIDTH * FRAME_HEIGHT)

uint8_t* previousFrame = NULL;  // Reference frame
uint8_t* currentFrame  = NULL;  // Current frame

// Detection state
unsigned long lastDetectionTime = 0;
int           detectionCount    = 0;
bool          motionActive      = false;
String        lastCaptureFile   = "";
unsigned long lastDetectionMillis = 0;

// SD card state
bool sdAvailable = false;
int  captureNumber = 0;

// ============================================================
// Camera Functions
// ============================================================

/*
 * initCamera() -- Configure and initialize the OV2640 camera
 * in QQVGA grayscale mode for frame differencing.
 */
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // Start in grayscale QQVGA for motion detection
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size   = FRAMESIZE_QQVGA;  // 160x120
  config.fb_count     = 2;  // Double buffering
  config.grab_mode    = CAMERA_GRAB_LATEST;

  // Use PSRAM for frame buffers if available
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    Serial.println("[Camera] PSRAM found, using for frame buffers");
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("[Camera] WARNING: No PSRAM! Enable in Tools menu.");
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[Camera] Init FAILED with error 0x%x\n", err);
    return false;
  }

  // Camera sensor settings
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor) {
    sensor->set_brightness(sensor, 0);
    sensor->set_contrast(sensor, 0);
    sensor->set_saturation(sensor, 0);
    // Enable auto-exposure and auto-white-balance
    sensor->set_whitebal(sensor, 1);
    sensor->set_awb_gain(sensor, 1);
    sensor->set_exposure_ctrl(sensor, 1);
    sensor->set_aec2(sensor, 1);
    sensor->set_gain_ctrl(sensor, 1);
  }

  Serial.println("[Camera] Initialized (QQVGA grayscale)");
  return true;
}

/*
 * captureGrayscaleFrame() -- Capture a frame and copy the
 * grayscale pixel data into the provided buffer.
 * Returns true on success.
 */
bool captureGrayscaleFrame(uint8_t* buffer) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[Camera] Frame capture failed");
    return false;
  }

  // Copy frame data to our buffer
  // The frame should be exactly FRAME_SIZE bytes in QQVGA grayscale
  if (fb->len >= FRAME_SIZE) {
    memcpy(buffer, fb->buf, FRAME_SIZE);
  } else {
    Serial.printf("[Camera] Unexpected frame size: %d (expected %d)\n",
                  fb->len, FRAME_SIZE);
    esp_camera_fb_return(fb);
    return false;
  }

  esp_camera_fb_return(fb);
  return true;
}

/*
 * captureAndSaveJPEG() -- Temporarily switch the camera to
 * higher resolution JPEG mode, capture a frame, and save
 * it to the SD card.
 */
bool captureAndSaveJPEG() {
  if (!sdAvailable) {
    Serial.println("[Capture] SD card not available");
    return false;
  }

  // We need to deinit and reinit the camera with JPEG settings
  // because changing pixel format on-the-fly is not reliable
  esp_camera_deinit();
  delay(100);

  // Reinitialize in JPEG mode at higher resolution
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_SVGA;    // 800x600 for captures
  config.jpeg_quality = 10;                 // 1-63, lower = better quality
  config.fb_count     = 1;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_QVGA;  // Smaller if no PSRAM
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[Capture] Camera reinit failed: 0x%x\n", err);
    // Try to go back to grayscale mode
    initCamera();
    return false;
  }

  // Skip a few frames to let auto-exposure settle
  for (int i = 0; i < 3; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(50);
  }

  // Capture the actual frame
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[Capture] JPEG capture failed");
    esp_camera_deinit();
    delay(100);
    initCamera();
    return false;
  }

  // Generate filename
  captureNumber++;
  char filename[40];
  snprintf(filename, sizeof(filename), "/captures/motion_%04d.jpg", captureNumber);

  // Save to SD card
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.write(fb->buf, fb->len);
    file.close();
    lastCaptureFile = String(filename);
    Serial.printf("[Capture] Saved %s (%d bytes)\n", filename, fb->len);
  } else {
    Serial.println("[Capture] Failed to open file for writing");
  }

  esp_camera_fb_return(fb);

  // Switch back to grayscale mode for continued monitoring
  esp_camera_deinit();
  delay(100);
  initCamera();

  return true;
}

// ============================================================
// Motion Detection
// ============================================================

/*
 * detectMotion() -- Compare currentFrame and previousFrame
 * pixel by pixel. Returns the percentage of changed pixels.
 */
float detectMotion() {
  int changedPixels = 0;

  for (int i = 0; i < FRAME_SIZE; i++) {
    // Calculate absolute difference between corresponding pixels
    int diff = abs((int)currentFrame[i] - (int)previousFrame[i]);

    // Is this pixel significantly different?
    if (diff > PIXEL_THRESHOLD) {
      changedPixels++;
    }
  }

  // Calculate percentage of frame that changed
  float percentChanged = (changedPixels * 100.0) / FRAME_SIZE;
  return percentChanged;
}

// ============================================================
// Web Server
// ============================================================

/*
 * handleRoot() -- Main dashboard page showing detection status.
 */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="5">
<title>Motion Detection Camera</title>
<style>
  body { font-family: 'Segoe UI', Arial, sans-serif; max-width: 700px;
         margin: 0 auto; padding: 20px; background: #0f0f23; color: #ccc; }
  h1 { color: #ff6b6b; text-align: center; }
  .status { background: #1a1a3e; border-radius: 10px; padding: 20px;
            margin: 15px 0; }
  .status .label { color: #8892b0; font-size: 0.85em; }
  .status .value { font-size: 1.3em; color: #e6f1ff; margin-top: 5px; }
  .alert { background: #3d0a0a; border-left: 4px solid #ff6b6b;
           padding: 15px; border-radius: 0 8px 8px 0; margin: 15px 0; }
  .ok { background: #0a3d0a; border-left: 4px solid #00ff41; }
  img { max-width: 100%; border-radius: 8px; margin: 10px 0; }
  .footer { text-align: center; color: #555; font-size: 0.8em; margin-top: 20px; }
</style>
</head><body>
<h1>Motion Detection Camera</h1>
)rawliteral";

  // Status indicator
  if (millis() - lastDetectionMillis < COOLDOWN_MS) {
    html += "<div class='status alert'>MOTION DETECTED!</div>";
  } else {
    html += "<div class='status ok'>Monitoring... no motion</div>";
  }

  html += "<div class='status'>";
  html += "<div class='label'>Total Detections</div>";
  html += "<div class='value'>" + String(detectionCount) + "</div>";
  html += "</div>";

  html += "<div class='status'>";
  html += "<div class='label'>Last Detection</div>";
  html += "<div class='value'>" + (lastCaptureFile.length() > 0 ?
          lastCaptureFile : "None yet") + "</div>";
  html += "</div>";

  html += "<div class='status'>";
  html += "<div class='label'>Uptime</div>";
  html += "<div class='value'>" + String(millis() / 1000) + " seconds</div>";
  html += "</div>";

  html += "<div class='status'>";
  html += "<div class='label'>Settings</div>";
  html += "<div class='value'>Pixel threshold: " + String(PIXEL_THRESHOLD);
  html += " | Motion percent: " + String(MOTION_PERCENT, 1) + "%</div>";
  html += "</div>";

  // Show latest capture
  if (lastCaptureFile.length() > 0) {
    html += "<h3 style='color:#8892b0;'>Latest Capture</h3>";
    html += "<img src='/latest' alt='Latest motion capture'>";
  }

  html += "<div class='footer'>Page auto-refreshes every 5 seconds</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

/*
 * handleLatest() -- Serve the most recently captured JPEG image.
 */
void handleLatest() {
  if (lastCaptureFile.length() == 0 || !sdAvailable) {
    server.send(404, "text/plain", "No captures yet");
    return;
  }

  File file = SD.open(lastCaptureFile);
  if (!file) {
    server.send(404, "text/plain", "Capture file not found");
    return;
  }

  server.streamFile(file, "image/jpeg");
  file.close();
}

/*
 * handleStatus() -- JSON status endpoint for programmatic access.
 */
void handleStatus() {
  String json = "{";
  json += "\"detections\":" + String(detectionCount) + ",";
  json += "\"lastCapture\":\"" + lastCaptureFile + "\",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"sdAvailable\":" + String(sdAvailable ? "true" : "false") + ",";
  json += "\"pixelThreshold\":" + String(PIXEL_THRESHOLD) + ",";
  json += "\"motionPercent\":" + String(MOTION_PERCENT, 1);
  json += "}";

  server.send(200, "application/json", json);
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("====================================");
  Serial.println("  XIAO ESP32S3 Motion Detection");
  Serial.println("====================================");

  // Check PSRAM
  if (psramFound()) {
    Serial.printf("[System] PSRAM: %d bytes available\n", ESP.getFreePsram());
  } else {
    Serial.println("[System] WARNING: PSRAM not found! Enable in Tools menu.");
    Serial.println("[System] Camera performance will be severely limited.");
  }

  // Allocate frame buffers
  previousFrame = (uint8_t*)malloc(FRAME_SIZE);
  currentFrame  = (uint8_t*)malloc(FRAME_SIZE);

  if (!previousFrame || !currentFrame) {
    Serial.println("[System] FATAL: Could not allocate frame buffers!");
    while (true) delay(1000);
  }
  Serial.printf("[System] Frame buffers allocated (%d bytes each)\n", FRAME_SIZE);

  // Initialize camera
  if (!initCamera()) {
    Serial.println("[Camera] FATAL: Camera initialization failed!");
    while (true) delay(1000);
  }

  // Initialize SD card
  if (SD.begin(SD_CS)) {
    sdAvailable = true;
    Serial.println("[SD] Card initialized");

    // Create captures directory if it doesn't exist
    if (!SD.exists("/captures")) {
      SD.mkdir("/captures");
      Serial.println("[SD] Created /captures directory");
    }

    // Find the highest existing capture number to avoid overwriting
    File dir = SD.open("/captures");
    if (dir) {
      File entry;
      while ((entry = dir.openNextFile())) {
        String name = entry.name();
        if (name.startsWith("motion_") && name.endsWith(".jpg")) {
          // Extract number from "motion_0042.jpg"
          int num = name.substring(7, 11).toInt();
          if (num > captureNumber) captureNumber = num;
        }
        entry.close();
      }
      dir.close();
    }
    Serial.printf("[SD] Starting capture number: %d\n", captureNumber + 1);
  } else {
    Serial.println("[SD] Init failed -- captures will not be saved");
  }

  // Connect WiFi
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] Dashboard: http://");
    Serial.println(WiFi.localIP());

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/latest", handleLatest);
    server.on("/status", handleStatus);
    server.begin();
  } else {
    Serial.println("[WiFi] Connection failed -- web dashboard unavailable");
  }

  // Warm up the camera: capture and discard several frames
  // This allows auto-exposure and auto-white-balance to stabilize
  Serial.print("[Camera] Warming up");
  for (int i = 0; i < WARMUP_FRAMES; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(100);
    Serial.print(".");
  }
  Serial.println(" done");

  // Capture the initial reference frame
  if (!captureGrayscaleFrame(previousFrame)) {
    Serial.println("[Camera] FATAL: Could not capture reference frame!");
    while (true) delay(1000);
  }
  Serial.println("[Camera] Reference frame captured");

  Serial.println();
  Serial.println("=== Monitoring Started ===");
  Serial.printf("Pixel threshold: %d | Motion percent: %.1f%%\n",
                PIXEL_THRESHOLD, MOTION_PERCENT);
  Serial.printf("Cooldown: %d ms | Frame size: %dx%d\n",
                COOLDOWN_MS, FRAME_WIDTH, FRAME_HEIGHT);
  Serial.println();
}

// ============================================================
// Main Loop
// ============================================================

void loop() {
  // Handle web server requests
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  // Capture current frame
  if (!captureGrayscaleFrame(currentFrame)) {
    delay(100);  // Brief pause before retry
    return;
  }

  // Perform motion detection (compare current to previous)
  float percentChanged = detectMotion();

  // Check if motion threshold is exceeded
  bool motionDetected = (percentChanged >= MOTION_PERCENT);

  // Only act on motion if we are past the cooldown period
  if (motionDetected && (millis() - lastDetectionMillis > COOLDOWN_MS)) {
    detectionCount++;
    lastDetectionMillis = millis();

    Serial.printf("[MOTION] Detected! %.1f%% of pixels changed (threshold: %.1f%%)\n",
                  percentChanged, MOTION_PERCENT);
    Serial.printf("[MOTION] Detection #%d -- capturing JPEG...\n", detectionCount);

    // Capture and save a high-resolution JPEG
    if (captureAndSaveJPEG()) {
      Serial.println("[MOTION] Capture saved successfully");
    } else {
      Serial.println("[MOTION] Capture save failed");
    }

    // After capturing (which reinits the camera), we need a fresh reference
    captureGrayscaleFrame(previousFrame);
  } else {
    // No motion (or in cooldown) -- update reference frame
    // Swap current to previous for next comparison
    uint8_t* temp = previousFrame;
    previousFrame = currentFrame;
    currentFrame  = temp;
  }

  // Brief delay to control frame rate (~10 FPS)
  delay(100);
}
