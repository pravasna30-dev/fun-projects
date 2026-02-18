/*
 * Lesson 12: Camera Capture
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch initializes the OV2640 camera on the Sense expansion board,
 * creates a web server, and serves JPEG snapshots when you visit /capture
 * in your browser.
 *
 * Prerequisites:
 *   - PSRAM must be enabled: Tools > PSRAM > OPI PSRAM
 *   - Sense expansion board must be attached (includes camera)
 *
 * No additional libraries needed — esp_camera and WebServer are built in.
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ─── WiFi Configuration ─────────────────────────────────────────────────────

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ─── Camera Pin Definitions for XIAO ESP32-S3 Sense ─────────────────────────
// These pins are specific to the Seeed Studio XIAO ESP32-S3 Sense board.
// Do NOT change these unless you are using a different board.

#define PWDN_GPIO_NUM   -1   // Not connected (camera is always powered)
#define RESET_GPIO_NUM  -1   // Not connected (software reset used)
#define XCLK_GPIO_NUM   10   // Master clock from ESP32 to camera
#define SIOD_GPIO_NUM   40   // I2C data  (SCCB protocol)
#define SIOC_GPIO_NUM   39   // I2C clock (SCCB protocol)

#define Y9_GPIO_NUM     48   // Data bit 7 (MSB)
#define Y8_GPIO_NUM     11   // Data bit 6
#define Y7_GPIO_NUM     12   // Data bit 5
#define Y6_GPIO_NUM     14   // Data bit 4
#define Y5_GPIO_NUM     16   // Data bit 3
#define Y4_GPIO_NUM     18   // Data bit 2
#define Y3_GPIO_NUM     17   // Data bit 1
#define Y2_GPIO_NUM     15   // Data bit 0 (LSB)

#define VSYNC_GPIO_NUM  38   // Vertical sync (frame start)
#define HREF_GPIO_NUM   47   // Horizontal reference (row valid)
#define PCLK_GPIO_NUM   13   // Pixel clock (data valid)

// ─── Built-in LED ───────────────────────────────────────────────────────────

#define LED_PIN 21

// ─── Web Server ─────────────────────────────────────────────────────────────

WebServer server(80);

// ─── Camera Initialization ──────────────────────────────────────────────────

bool initCamera() {
  camera_config_t config;

  // Pin assignments
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

  // Clock and format settings
  config.xclk_freq_hz = 20000000;          // 20 MHz master clock
  config.ledc_timer    = LEDC_TIMER_0;
  config.ledc_channel  = LEDC_CHANNEL_0;
  config.pixel_format  = PIXFORMAT_JPEG;    // JPEG output (hardware encoded)

  // Frame size and quality depend on PSRAM availability
  if (psramFound()) {
    Serial.println("PSRAM found! Using higher resolution.");
    config.frame_size   = FRAMESIZE_VGA;     // 640x480 — good balance
    config.jpeg_quality = 10;                // 0-63, lower = better quality
    config.fb_count     = 2;                 // Double buffering for speed
    config.grab_mode    = CAMERA_GRAB_LATEST; // Always get the latest frame
  } else {
    Serial.println("WARNING: No PSRAM found. Using lower resolution.");
    config.frame_size   = FRAMESIZE_QVGA;    // 320x240 — fits in SRAM
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  }

  // Initialize the camera driver
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Optional: adjust camera sensor settings for better image quality
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_brightness(s, 1);    // Slightly brighter (-2 to 2)
    s->set_saturation(s, 0);    // Default saturation (-2 to 2)
    s->set_whitebal(s, 1);      // Enable auto white balance
    s->set_awb_gain(s, 1);      // Enable AWB gain
    s->set_exposure_ctrl(s, 1); // Enable auto exposure
    s->set_aec2(s, 1);          // Enable AEC DSP
  }

  return true;
}

// ─── Web Server Handlers ────────────────────────────────────────────────────

// Handler: Root page — simple HTML with embedded image
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>XIAO ESP32-S3 Camera</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; ";
  html += "background: #1a1a2e; color: #eee; margin: 0; padding: 20px; }";
  html += "h1 { color: #e94560; }";
  html += "img { max-width: 100%; border: 2px solid #e94560; border-radius: 8px; }";
  html += ".btn { display: inline-block; margin: 10px; padding: 12px 24px; ";
  html += "background: #e94560; color: white; text-decoration: none; ";
  html += "border-radius: 6px; font-size: 16px; }";
  html += ".btn:hover { background: #c23152; }";
  html += "</style></head><body>";
  html += "<h1>XIAO ESP32-S3 Camera</h1>";
  html += "<p>Click the image or button to refresh</p>";
  html += "<a href='/capture'><img src='/capture' id='photo'></a><br>";
  html += "<a class='btn' href='/'>Refresh</a>";
  html += "<a class='btn' href='/info'>Camera Info</a>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Handler: Capture a single JPEG photo and send it to the browser
void handleCapture() {
  // Acquire a frame buffer from the camera
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("ERROR: Frame buffer capture failed!");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  Serial.printf("Captured image: %dx%d, %u bytes\n", fb->width, fb->height, fb->len);

  // Send the JPEG data with the correct content type
  // The browser will render it as an image
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);

  // CRITICAL: Return the frame buffer to the pool so it can be reused.
  // Forgetting this will cause the camera to hang after fb_count captures.
  esp_camera_fb_return(fb);
}

// Handler: Return camera info as JSON
void handleInfo() {
  sensor_t *s = esp_camera_sensor_get();

  String json = "{\n";
  json += "  \"board\": \"XIAO ESP32-S3 Sense\",\n";
  json += "  \"camera\": \"OV2640\",\n";
  json += "  \"psram\": " + String(psramFound() ? "true" : "false") + ",\n";
  json += "  \"psram_size\": " + String(ESP.getPsramSize()) + ",\n";
  json += "  \"free_psram\": " + String(ESP.getFreePsram()) + ",\n";
  json += "  \"free_heap\": " + String(ESP.getFreeHeap()) + ",\n";
  json += "  \"uptime_sec\": " + String(millis() / 1000) + ",\n";

  if (s != NULL) {
    json += "  \"pixel_format\": \"JPEG\",\n";

    // Decode frame size to string
    String resolution;
    switch (s->status.framesize) {
      case FRAMESIZE_QQVGA: resolution = "160x120"; break;
      case FRAMESIZE_QVGA:  resolution = "320x240"; break;
      case FRAMESIZE_VGA:   resolution = "640x480"; break;
      case FRAMESIZE_SVGA:  resolution = "800x600"; break;
      case FRAMESIZE_XGA:   resolution = "1024x768"; break;
      case FRAMESIZE_SXGA:  resolution = "1280x1024"; break;
      case FRAMESIZE_UXGA:  resolution = "1600x1200"; break;
      default:              resolution = "unknown"; break;
    }
    json += "  \"resolution\": \"" + resolution + "\",\n";
    json += "  \"quality\": " + String(s->status.quality) + "\n";
  }

  json += "}";

  server.send(200, "application/json", json);
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
  Serial.println("=== Lesson 12: Camera Capture ===");
  Serial.println();

  // Configure LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize the camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    Serial.println("FATAL: Camera initialization failed. Halting.");
    // Blink LED rapidly to indicate error
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }

  // Print resolution info
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    Serial.printf("Resolution: frame_size=%d, quality=%d\n",
                  s->status.framesize, s->status.quality);
  }

  // Connect to WiFi
  setupWiFi();

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/info", handleInfo);

  // Start the server
  server.begin();
  Serial.println();
  Serial.println("Camera server started!");
  Serial.print("  http://");
  Serial.print(WiFi.localIP());
  Serial.println("/capture  - Take a photo");
  Serial.print("  http://");
  Serial.print(WiFi.localIP());
  Serial.println("/info     - Camera info");

  // Brief LED flash to indicate successful startup
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
}

// ─── Loop ───────────────────────────────────────────────────────────────────

void loop() {
  server.handleClient();
}
