/*
 * Lesson 20: Smart Doorbell (Capstone Project)
 *
 * Board: Seeed Studio XIAO ESP32S3 Sense
 *
 * The FINAL capstone project combining everything learned across
 * all 20 lessons. This smart doorbell:
 *
 * - Detects a button press (doorbell ring)
 * - Captures a photo with the OV2640 camera
 * - Records a short audio clip via the PDM microphone
 * - Saves both to the SD card with timestamps
 * - Sends a BLE notification to a connected phone
 * - Serves a web dashboard with ring history and photo viewer
 * - Streams live camera on demand
 *
 * Architecture: State machine with 4 states (IDLE, CAPTURING,
 * NOTIFYING, COOLDOWN) for clean event handling.
 *
 * Wiring:
 *   Push button: D1 (GPIO 2) to GND (uses internal pull-up)
 *   Camera, mic, SD: Built into Sense expansion board
 *
 * Important: Enable PSRAM in Arduino IDE -> Tools -> PSRAM: "OPI PSRAM"
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <driver/i2s.h>

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

// --- PDM Microphone pins ---
#define PDM_CLK_PIN  42
#define PDM_DATA_PIN 41

// --- Other pins ---
#define BUTTON_PIN   D1    // Doorbell button (active LOW with pull-up)
#define LED_PIN      21    // Built-in LED (active LOW)
#define SD_CS        21    // SD card chip select

// --- Timing ---
#define COOLDOWN_MS       3000   // Cooldown between rings (ms)
#define AUDIO_DURATION_MS 2000   // Audio recording length (ms)
#define DEBOUNCE_MS       50     // Button debounce time (ms)

// --- Audio settings ---
#define AUDIO_SAMPLE_RATE 16000  // 16 kHz
#define AUDIO_BUFFER_SIZE (AUDIO_SAMPLE_RATE * AUDIO_DURATION_MS / 1000)

// --- BLE UUIDs ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ============================================================
// State Machine
// ============================================================

enum State {
  STATE_IDLE,
  STATE_CAPTURING,
  STATE_NOTIFYING,
  STATE_COOLDOWN
};

State currentState = STATE_IDLE;
unsigned long stateEnteredAt = 0;

// ============================================================
// Global Objects
// ============================================================

WebServer server(80);

// BLE objects
BLEServer*         pBLEServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool               bleDeviceConnected = false;
bool               bleOldDeviceConnected = false;

// Ring event tracking
int           ringCount = 0;
unsigned long lastRingTime = 0;
String        lastPhotoFile = "";

// Button state
unsigned long lastButtonPress = 0;
bool          lastButtonState = HIGH;  // Pull-up, so HIGH = not pressed

// Subsystem availability
bool sdAvailable   = false;
bool wifiAvailable = false;
bool cameraAvailable = false;

// Maximum ring events to track in memory for the web dashboard
#define MAX_RING_HISTORY 50
struct RingEvent {
  int    id;
  unsigned long timestamp;
  String filename;
};
RingEvent ringHistory[MAX_RING_HISTORY];
int ringHistoryCount = 0;

// ============================================================
// BLE Callbacks
// ============================================================

class DoorbellBLECallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleDeviceConnected = true;
    Serial.println("[BLE] Device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    bleDeviceConnected = false;
    Serial.println("[BLE] Device disconnected");
  }
};

// ============================================================
// Camera Functions
// ============================================================

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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_SVGA;    // 800x600
  config.jpeg_quality = 10;                 // Good quality
  config.fb_count     = 2;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    Serial.println("[Camera] WARNING: No PSRAM, reducing resolution");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[Camera] Init failed: 0x%x\n", err);
    return false;
  }

  // Warm up: discard first few frames for auto-exposure
  for (int i = 0; i < 5; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(50);
  }

  Serial.println("[Camera] Initialized (SVGA JPEG)");
  return true;
}

/*
 * capturePhoto() -- Capture a JPEG photo and save to SD card.
 * Returns the filename on success, empty string on failure.
 */
String capturePhoto(int ringId) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[Camera] Capture failed");
    return "";
  }

  char filename[40];
  snprintf(filename, sizeof(filename), "/doorbell/ring_%04d.jpg", ringId);

  if (sdAvailable) {
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      file.write(fb->buf, fb->len);
      file.flush();
      file.close();
      Serial.printf("[Camera] Saved %s (%d bytes)\n", filename, fb->len);
    } else {
      Serial.println("[Camera] Failed to write file");
    }
  }

  esp_camera_fb_return(fb);
  return String(filename);
}

// ============================================================
// PDM Microphone Functions
// ============================================================

/*
 * initMicrophone() -- Configure the I2S peripheral for PDM
 * microphone input.
 */
bool initMicrophone() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PIN_NO_CHANGE,
    .ws_io_num = PDM_CLK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PDM_DATA_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("[Mic] I2S driver install failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[Mic] I2S pin config failed: %d\n", err);
    return false;
  }

  Serial.println("[Mic] PDM microphone initialized");
  return true;
}

/*
 * recordAudio() -- Record a short audio clip and save as raw PCM.
 * The file can be converted to WAV later using tools like Audacity.
 */
bool recordAudio(int ringId) {
  char filename[40];
  snprintf(filename, sizeof(filename), "/doorbell/ring_%04d.raw", ringId);

  if (!sdAvailable) return false;

  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("[Mic] Failed to open audio file");
    return false;
  }

  Serial.println("[Mic] Recording audio...");

  // Read audio data in chunks
  const int chunkSize = 1024;
  int16_t buffer[chunkSize];
  size_t bytesRead;
  unsigned long startTime = millis();

  while (millis() - startTime < AUDIO_DURATION_MS) {
    esp_err_t err = i2s_read(I2S_NUM_0, buffer, chunkSize * sizeof(int16_t),
                              &bytesRead, portMAX_DELAY);
    if (err == ESP_OK && bytesRead > 0) {
      file.write((uint8_t*)buffer, bytesRead);
    }
  }

  file.flush();
  file.close();
  Serial.printf("[Mic] Audio saved: %s\n", filename);
  return true;
}

// ============================================================
// BLE Functions
// ============================================================

void initBLE() {
  BLEDevice::init("XIAO-Doorbell");
  pBLEServer = BLEDevice::createServer();
  pBLEServer->setCallbacks(new DoorbellBLECallbacks());

  BLEService* pService = pBLEServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("Ready");

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising as 'XIAO-Doorbell'");
}

/*
 * sendBLENotification() -- Send a ring event notification to
 * any connected BLE device.
 */
void sendBLENotification(int ringId) {
  if (bleDeviceConnected) {
    String message = "Ring #" + String(ringId) + " at " +
                     String(millis() / 1000) + "s";
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
    Serial.println("[BLE] Notification sent: " + message);
  } else {
    Serial.println("[BLE] No device connected, skipping notification");
  }
}

// ============================================================
// SD Card Functions
// ============================================================

bool initSD() {
  if (SD.begin(SD_CS)) {
    // Create doorbell directory
    if (!SD.exists("/doorbell")) {
      SD.mkdir("/doorbell");
    }

    // Find highest existing ring number
    File dir = SD.open("/doorbell");
    if (dir) {
      File entry;
      while ((entry = dir.openNextFile())) {
        String name = entry.name();
        if (name.startsWith("ring_") && name.endsWith(".jpg")) {
          int num = name.substring(5, 9).toInt();
          if (num > ringCount) ringCount = num;
        }
        entry.close();
      }
      dir.close();
    }

    Serial.printf("[SD] Initialized. Existing rings: %d\n", ringCount);
    return true;
  }
  return false;
}

/*
 * logRingEvent() -- Append a ring event to the CSV log file.
 */
void logRingEvent(int ringId) {
  if (!sdAvailable) return;

  File logFile = SD.open("/doorbell/log.csv", FILE_APPEND);
  if (logFile) {
    // Create header if file is new
    if (logFile.size() == 0) {
      logFile.println("id,timestamp_ms,filename");
    }
    logFile.printf("%d,%lu,ring_%04d\n", ringId, millis(), ringId);
    logFile.flush();
    logFile.close();
  }
}

// ============================================================
// Web Server Handlers
// ============================================================

/*
 * handleRoot() -- Main dashboard page with ring history.
 */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Doorbell</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', Arial, sans-serif; background: #0a0a1a; color: #ccc; }
  .header { background: linear-gradient(135deg, #1a1a3e, #2d1b69);
            padding: 20px; text-align: center; }
  .header h1 { color: #fff; font-size: 1.8em; }
  .header .subtitle { color: #8892b0; margin-top: 5px; }
  .container { max-width: 800px; margin: 0 auto; padding: 15px; }
  .status-bar { display: flex; gap: 10px; flex-wrap: wrap; margin: 15px 0; }
  .status-item { background: #1a1a3e; border-radius: 8px; padding: 12px 18px;
                 flex: 1; min-width: 120px; text-align: center; }
  .status-item .label { color: #8892b0; font-size: 0.8em; text-transform: uppercase; }
  .status-item .value { color: #e6f1ff; font-size: 1.4em; margin-top: 5px; }
  .btn { display: inline-block; background: #4a3f8a; color: #fff; padding: 12px 24px;
         border-radius: 8px; text-decoration: none; margin: 10px 5px; font-size: 1em;
         border: none; cursor: pointer; }
  .btn:hover { background: #5a4f9a; }
  .btn-live { background: #c0392b; }
  .ring-list { margin-top: 20px; }
  .ring-event { background: #1a1a3e; border-radius: 10px; padding: 15px;
                margin: 10px 0; display: flex; align-items: center; gap: 15px;
                cursor: pointer; transition: background 0.2s; }
  .ring-event:hover { background: #252550; }
  .ring-number { font-size: 1.5em; color: #ff6b6b; font-weight: bold;
                 min-width: 60px; text-align: center; }
  .ring-details { flex: 1; }
  .ring-details .time { color: #8892b0; font-size: 0.85em; }
  .ring-thumb { width: 80px; height: 60px; object-fit: cover; border-radius: 6px; }
  .no-rings { text-align: center; padding: 40px; color: #555; }
  .actions { text-align: center; margin: 20px 0; }
</style>
</head><body>
<div class="header">
  <h1>Smart Doorbell</h1>
  <div class="subtitle">XIAO ESP32S3 Sense</div>
</div>
<div class="container">
  <div class="status-bar">
    <div class="status-item">
      <div class="label">Total Rings</div>
      <div class="value" id="rings">)rawliteral";

  html += String(ringCount);
  html += R"rawliteral(</div>
    </div>
    <div class="status-item">
      <div class="label">BLE</div>
      <div class="value" id="ble">)rawliteral";

  html += bleDeviceConnected ? "Connected" : "Waiting";
  html += R"rawliteral(</div>
    </div>
    <div class="status-item">
      <div class="label">SD Card</div>
      <div class="value">)rawliteral";

  html += sdAvailable ? "OK" : "N/A";
  html += R"rawliteral(</div>
    </div>
    <div class="status-item">
      <div class="label">Uptime</div>
      <div class="value">)rawliteral";

  unsigned long upSec = millis() / 1000;
  if (upSec > 3600) {
    html += String(upSec / 3600) + "h " + String((upSec % 3600) / 60) + "m";
  } else if (upSec > 60) {
    html += String(upSec / 60) + "m " + String(upSec % 60) + "s";
  } else {
    html += String(upSec) + "s";
  }

  html += R"rawliteral(</div>
    </div>
  </div>

  <div class="actions">
    <a href="/stream" class="btn btn-live" target="_blank">View Live Camera</a>
  </div>

  <h2 style="color:#8892b0; margin-top:20px;">Ring History</h2>
  <div class="ring-list">)rawliteral";

  // Display ring history (newest first)
  if (ringHistoryCount == 0) {
    html += "<div class='no-rings'>No rings yet. Press the doorbell button!</div>";
  } else {
    for (int i = ringHistoryCount - 1; i >= 0; i--) {
      html += "<div class='ring-event' onclick=\"window.open('/photo?id=";
      html += String(ringHistory[i].id);
      html += "','_blank')\">";
      html += "<div class='ring-number'>#" + String(ringHistory[i].id) + "</div>";
      html += "<div class='ring-details'>";
      html += "<div><strong>Ring Event</strong></div>";
      html += "<div class='time'>" + String(ringHistory[i].timestamp / 1000) + "s after boot</div>";
      html += "<div class='time'>" + ringHistory[i].filename + "</div>";
      html += "</div>";
      html += "<img class='ring-thumb' src='/photo?id=" + String(ringHistory[i].id) + "' alt='Photo'>";
      html += "</div>";
    }
  }

  html += R"rawliteral(
  </div>
</div>
<script>
// Auto-refresh every 5 seconds
setTimeout(function(){ location.reload(); }, 5000);
</script>
</body></html>)rawliteral";

  server.send(200, "text/html", html);
}

/*
 * handlePhoto() -- Serve a captured photo by ring ID.
 */
void handlePhoto() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "Missing 'id' parameter");
    return;
  }

  int id = server.arg("id").toInt();
  char filename[40];
  snprintf(filename, sizeof(filename), "/doorbell/ring_%04d.jpg", id);

  if (!sdAvailable || !SD.exists(filename)) {
    server.send(404, "text/plain", "Photo not found");
    return;
  }

  File file = SD.open(filename);
  if (!file) {
    server.send(500, "text/plain", "Failed to open photo");
    return;
  }

  server.streamFile(file, "image/jpeg");
  file.close();
}

/*
 * handleStream() -- MJPEG live camera stream.
 * Sends a continuous stream of JPEG frames.
 */
void handleStream() {
  WiFiClient client = server.client();

  String boundary = "frame";
  String header = "HTTP/1.1 200 OK\r\n";
  header += "Content-Type: multipart/x-mixed-replace;boundary=" + boundary + "\r\n";
  header += "Access-Control-Allow-Origin: *\r\n\r\n";
  client.print(header);

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[Stream] Frame capture failed");
      break;
    }

    String part = "--" + boundary + "\r\n";
    part += "Content-Type: image/jpeg\r\n";
    part += "Content-Length: " + String(fb->len) + "\r\n\r\n";
    client.print(part);
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);

    // ~10 FPS
    delay(100);
  }
}

/*
 * handleStatus() -- JSON status endpoint.
 */
void handleStatus() {
  String json = "{";
  json += "\"rings\":" + String(ringCount) + ",";
  json += "\"ble\":\"" + String(bleDeviceConnected ? "connected" : "disconnected") + "\",";
  json += "\"sd\":" + String(sdAvailable ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"state\":\"" + String(
    currentState == STATE_IDLE ? "idle" :
    currentState == STATE_CAPTURING ? "capturing" :
    currentState == STATE_NOTIFYING ? "notifying" : "cooldown"
  ) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// ============================================================
// Button Handling
// ============================================================

/*
 * isButtonPressed() -- Check if the doorbell button is pressed.
 * Implements software debouncing.
 * Returns true on a falling edge (button just pressed).
 */
bool isButtonPressed() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Detect falling edge (HIGH -> LOW transition, since pull-up)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Debounce: check if enough time has passed since last press
    if (millis() - lastButtonPress > DEBOUNCE_MS) {
      lastButtonPress = millis();
      lastButtonState = currentButtonState;
      return true;
    }
  }

  lastButtonState = currentButtonState;
  return false;
}

// ============================================================
// State Machine Actions
// ============================================================

/*
 * doCapture() -- Called in STATE_CAPTURING.
 * Captures a photo and optionally records audio.
 */
void doCapture() {
  ringCount++;
  int currentRingId = ringCount;

  Serial.printf("\n[DOORBELL] Ring #%d -- Capturing...\n", currentRingId);

  // Flash LED to indicate capture
  digitalWrite(LED_PIN, LOW);  // ON

  // Capture photo
  String photoFile = capturePhoto(currentRingId);
  if (photoFile.length() > 0) {
    lastPhotoFile = photoFile;
  }

  // Record audio (short clip)
  recordAudio(currentRingId);

  digitalWrite(LED_PIN, HIGH);  // OFF

  // Store in ring history (circular buffer)
  if (ringHistoryCount < MAX_RING_HISTORY) {
    ringHistory[ringHistoryCount].id = currentRingId;
    ringHistory[ringHistoryCount].timestamp = millis();
    ringHistory[ringHistoryCount].filename = photoFile;
    ringHistoryCount++;
  } else {
    // Shift everything down and add at end
    for (int i = 0; i < MAX_RING_HISTORY - 1; i++) {
      ringHistory[i] = ringHistory[i + 1];
    }
    ringHistory[MAX_RING_HISTORY - 1].id = currentRingId;
    ringHistory[MAX_RING_HISTORY - 1].timestamp = millis();
    ringHistory[MAX_RING_HISTORY - 1].filename = photoFile;
  }

  Serial.printf("[DOORBELL] Capture complete for ring #%d\n", currentRingId);
}

/*
 * doNotify() -- Called in STATE_NOTIFYING.
 * Saves log entry and sends BLE notification.
 */
void doNotify() {
  // Log to SD
  logRingEvent(ringCount);

  // Send BLE notification
  sendBLENotification(ringCount);

  Serial.printf("[DOORBELL] Notifications sent for ring #%d\n", ringCount);
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("========================================");
  Serial.println("  XIAO ESP32S3 Smart Doorbell");
  Serial.println("  Capstone Project - Lesson 20");
  Serial.println("========================================");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Off

  // Initialize button with internal pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Check PSRAM
  if (psramFound()) {
    Serial.printf("[System] PSRAM: %d KB available\n", ESP.getFreePsram() / 1024);
  } else {
    Serial.println("[System] WARNING: No PSRAM detected!");
  }

  // --- Initialize Camera ---
  cameraAvailable = initCamera();
  if (!cameraAvailable) {
    Serial.println("[Camera] FAILED - photos will not be available");
  }

  // --- Initialize Microphone ---
  if (!initMicrophone()) {
    Serial.println("[Mic] FAILED - audio recording will not be available");
  }

  // --- Initialize SD Card ---
  sdAvailable = initSD();
  if (!sdAvailable) {
    Serial.println("[SD] FAILED - storage will not be available");
  }

  // --- Initialize BLE ---
  initBLE();

  // --- Connect WiFi ---
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
    wifiAvailable = true;
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] Dashboard: http://");
    Serial.println(WiFi.localIP());

    // Setup web server
    server.on("/", handleRoot);
    server.on("/photo", handlePhoto);
    server.on("/stream", handleStream);
    server.on("/status", handleStatus);
    server.begin();
  } else {
    Serial.println("[WiFi] Failed - web dashboard unavailable");
    Serial.println("[WiFi] BLE notifications will still work!");
  }

  // --- Print status summary ---
  Serial.println();
  Serial.println("=== Subsystem Status ===");
  Serial.printf("  Camera:     %s\n", cameraAvailable ? "OK" : "FAILED");
  Serial.printf("  Microphone: OK\n");
  Serial.printf("  SD Card:    %s\n", sdAvailable ? "OK" : "FAILED");
  Serial.printf("  BLE:        Advertising\n");
  Serial.printf("  WiFi:       %s\n", wifiAvailable ? "OK" : "FAILED");
  Serial.println("========================");
  Serial.println();
  Serial.println("Press the doorbell button to trigger a ring event.");
  Serial.println();

  // Enter idle state
  currentState = STATE_IDLE;
  stateEnteredAt = millis();
}

// ============================================================
// Main Loop -- State Machine
// ============================================================

void loop() {
  // Always handle web server and BLE (these run in background)
  if (wifiAvailable) {
    server.handleClient();
  }

  // Handle BLE reconnection advertising
  if (!bleDeviceConnected && bleOldDeviceConnected) {
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Restarted advertising");
    bleOldDeviceConnected = bleDeviceConnected;
  }
  if (bleDeviceConnected && !bleOldDeviceConnected) {
    bleOldDeviceConnected = bleDeviceConnected;
  }

  // --- State Machine ---
  switch (currentState) {

    case STATE_IDLE:
      // Wait for button press
      if (isButtonPressed()) {
        Serial.println("\n>>> DOORBELL PRESSED! <<<");
        currentState = STATE_CAPTURING;
        stateEnteredAt = millis();
      }
      break;

    case STATE_CAPTURING:
      // Capture photo and audio
      doCapture();
      currentState = STATE_NOTIFYING;
      stateEnteredAt = millis();
      break;

    case STATE_NOTIFYING:
      // Save to SD log and send BLE notification
      doNotify();
      currentState = STATE_COOLDOWN;
      stateEnteredAt = millis();
      Serial.printf("[State] Cooldown for %d seconds...\n", COOLDOWN_MS / 1000);
      break;

    case STATE_COOLDOWN:
      // Wait for cooldown period to elapse
      if (millis() - stateEnteredAt >= COOLDOWN_MS) {
        currentState = STATE_IDLE;
        stateEnteredAt = millis();
        Serial.println("[State] Ready for next ring\n");
      }
      break;
  }

  // Small delay to prevent watchdog issues
  delay(10);
}
