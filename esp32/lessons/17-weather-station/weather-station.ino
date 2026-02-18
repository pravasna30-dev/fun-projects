/*
 * Lesson 17: Weather Station
 *
 * Board: Seeed Studio XIAO ESP32S3 Sense
 *
 * A complete weather monitoring station that combines:
 * - DHT22 temperature/humidity sensor
 * - SSD1306 OLED display (128x64, I2C)
 * - MicroSD card data logging (CSV format)
 * - WiFi web dashboard with live updates
 *
 * This is a capstone project combining skills from Lessons 7, 8, and 9.
 *
 * Wiring:
 *   DHT22 data pin -> D1 (with 10K pull-up to 3V3)
 *   OLED SDA       -> D4 (GPIO 6)
 *   OLED SCL       -> D5 (GPIO 7)
 *   SD card        -> Built into Sense expansion board
 *
 * Libraries needed:
 *   - DHT sensor library (Adafruit)
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <SPI.h>

// ============================================================
// Configuration
// ============================================================

// WiFi credentials
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD  = "YOUR_PASSWORD";

// DHT22 sensor on pin D1
#define DHT_PIN    D1
#define DHT_TYPE   DHT22

// OLED display settings (I2C on default SDA/SCL)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// SD card chip select (Sense expansion board default)
#define SD_CS 21

// Built-in LED
#define LED_PIN 21

// Timing intervals (milliseconds)
#define DHT_INTERVAL     2000    // Read sensor every 2 seconds
#define OLED_INTERVAL    1000    // Update display every 1 second
#define SD_LOG_INTERVAL  30000   // Log to SD every 30 seconds

// CSV log file path
#define LOG_FILE "/weather.csv"

// ============================================================
// Global Objects
// ============================================================

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// ============================================================
// Sensor Data
// ============================================================

// Current readings
float currentTemp     = 0.0;
float currentHumidity = 0.0;
bool  sensorOK        = false;

// Min/Max tracking
float minTemp = 999.0;
float maxTemp = -999.0;
float minHumidity = 999.0;
float maxHumidity = -999.0;

// Reading counter
unsigned long readingCount = 0;

// ============================================================
// Timing Variables (non-blocking with millis())
// ============================================================

unsigned long lastDHTRead   = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastSDLog      = 0;

// ============================================================
// Subsystem Status Flags
// ============================================================

bool oledAvailable = false;
bool sdAvailable   = false;
bool wifiAvailable = false;

// ============================================================
// Recent readings buffer for web dashboard chart
// ============================================================

#define HISTORY_SIZE 60  // Store last 60 readings (~2 minutes at 2s interval)
float tempHistory[HISTORY_SIZE];
float humHistory[HISTORY_SIZE];
int   historyIndex = 0;
int   historyCount = 0;

// ============================================================
// DHT22 Sensor Functions
// ============================================================

/*
 * readDHT() -- Read temperature and humidity from the DHT22.
 * Updates current values and min/max tracking.
 */
void readDHT() {
  float t = dht.readTemperature();    // Celsius
  float h = dht.readHumidity();       // Percent

  // DHT library returns NaN if the read fails
  if (isnan(t) || isnan(h)) {
    Serial.println("[DHT] Read failed!");
    sensorOK = false;
    return;
  }

  sensorOK = true;
  currentTemp = t;
  currentHumidity = h;
  readingCount++;

  // Update min/max
  if (t < minTemp) minTemp = t;
  if (t > maxTemp) maxTemp = t;
  if (h < minHumidity) minHumidity = h;
  if (h > maxHumidity) maxHumidity = h;

  // Store in history buffer (circular buffer)
  tempHistory[historyIndex] = t;
  humHistory[historyIndex] = h;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) historyCount++;

  Serial.printf("[DHT] Temp: %.1f C, Humidity: %.1f%%\n", t, h);
}

// ============================================================
// OLED Display Functions
// ============================================================

/*
 * updateOLED() -- Refresh the OLED display with current readings
 * and min/max values.
 *
 * Display layout:
 * +---------------------------+
 * | WEATHER STATION           |
 * |                           |
 * | Temp:  23.5 C             |
 * | Humid: 45.2%              |
 * |                           |
 * | Min: 21.0  Max: 25.3      |
 * | Min: 40.1% Max: 52.0%     |
 * +---------------------------+
 */
void updateOLED() {
  if (!oledAvailable) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Title bar
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WEATHER STATION");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  if (!sensorOK) {
    display.setTextSize(1);
    display.setCursor(0, 25);
    display.println("Sensor Error!");
    display.println("Check DHT22 wiring");
    display.display();
    return;
  }

  // Current readings (larger font)
  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("Temp:  ");
  display.setTextSize(2);
  display.print(currentTemp, 1);
  display.setTextSize(1);
  display.println(" C");

  display.setCursor(0, 33);
  display.print("Humid: ");
  display.setTextSize(2);
  display.print(currentHumidity, 1);
  display.setTextSize(1);
  display.println(" %");

  // Min/Max (small font at bottom)
  display.drawLine(0, 52, 127, 52, SSD1306_WHITE);
  display.setCursor(0, 55);
  display.printf("T:%.0f-%.0f  H:%.0f-%.0f%%", minTemp, maxTemp, minHumidity, maxHumidity);

  display.display();
}

// ============================================================
// SD Card Logging Functions
// ============================================================

/*
 * initSDLog() -- Create the CSV file with a header row if it
 * does not already exist.
 */
void initSDLog() {
  if (!sdAvailable) return;

  // Check if file exists; if not, create with header
  if (!SD.exists(LOG_FILE)) {
    File file = SD.open(LOG_FILE, FILE_WRITE);
    if (file) {
      file.println("timestamp_ms,temperature_c,humidity_pct");
      file.close();
      Serial.println("[SD] Created log file with header");
    }
  }
}

/*
 * logToSD() -- Append a CSV row with the current timestamp and readings.
 */
void logToSD() {
  if (!sdAvailable || !sensorOK) return;

  File file = SD.open(LOG_FILE, FILE_APPEND);
  if (file) {
    file.printf("%lu,%.1f,%.1f\n", millis(), currentTemp, currentHumidity);
    file.flush();  // Ensure data is written to card (prevents loss on power fail)
    file.close();
    Serial.println("[SD] Data logged");
  } else {
    Serial.println("[SD] Failed to open log file!");
  }
}

// ============================================================
// Web Server Handlers
// ============================================================

/*
 * handleRoot() -- Serve the main dashboard HTML page.
 * The page uses JavaScript fetch() to poll the /data endpoint
 * every 2 seconds for live updates without full page reloads.
 */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Weather Station Dashboard</title>
<style>
  body { font-family: 'Segoe UI', Arial, sans-serif; max-width: 800px;
         margin: 0 auto; padding: 20px; background: #0f0f23; color: #ccc; }
  h1 { color: #00d4ff; text-align: center; }
  .cards { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; }
  .card { background: #1a1a3e; border-radius: 12px; padding: 20px;
          min-width: 180px; flex: 1; text-align: center;
          box-shadow: 0 4px 15px rgba(0,0,0,0.4); }
  .card .label { color: #8892b0; font-size: 0.85em; text-transform: uppercase; }
  .card .value { font-size: 2.2em; color: #e6f1ff; margin: 10px 0; }
  .card .minmax { font-size: 0.8em; color: #666; }
  .temp .value { color: #ff6b6b; }
  .humid .value { color: #48dbfb; }
  .chart-container { background: #1a1a3e; border-radius: 12px; padding: 20px;
                     margin-top: 20px; }
  .chart-bar { display: flex; align-items: center; margin: 3px 0; }
  .chart-bar .bar { height: 8px; border-radius: 4px; transition: width 0.3s; }
  .bar-temp { background: linear-gradient(90deg, #ff6b6b, #ee5a24); }
  .bar-humid { background: linear-gradient(90deg, #48dbfb, #0abde3); }
  .status { text-align: center; margin: 15px 0; font-size: 0.85em; color: #555; }
  .legend { display: flex; gap: 20px; justify-content: center; margin-bottom: 10px; }
  .legend span { display: flex; align-items: center; gap: 5px; font-size: 0.85em; }
  .legend .dot { width: 10px; height: 10px; border-radius: 50%; }
</style>
</head><body>
<h1>Weather Station</h1>
<div class="cards">
  <div class="card temp">
    <div class="label">Temperature</div>
    <div class="value" id="temp">--</div>
    <div class="minmax" id="temp-range">Min: -- / Max: --</div>
  </div>
  <div class="card humid">
    <div class="label">Humidity</div>
    <div class="value" id="humid">--</div>
    <div class="minmax" id="humid-range">Min: -- / Max: --</div>
  </div>
  <div class="card">
    <div class="label">Readings</div>
    <div class="value" id="count">0</div>
    <div class="minmax">Total samples taken</div>
  </div>
</div>
<div class="chart-container">
  <h3 style="color:#8892b0;margin-top:0;">Recent History</h3>
  <div class="legend">
    <span><span class="dot" style="background:#ff6b6b;"></span> Temperature</span>
    <span><span class="dot" style="background:#48dbfb;"></span> Humidity</span>
  </div>
  <div id="chart"></div>
</div>
<div class="status" id="status">Connecting...</div>
<script>
function update() {
  fetch('/data').then(r => r.json()).then(d => {
    document.getElementById('temp').textContent = d.temp.toFixed(1) + ' C';
    document.getElementById('humid').textContent = d.humid.toFixed(1) + '%';
    document.getElementById('count').textContent = d.count;
    document.getElementById('temp-range').textContent =
      'Min: ' + d.minTemp.toFixed(1) + ' / Max: ' + d.maxTemp.toFixed(1);
    document.getElementById('humid-range').textContent =
      'Min: ' + d.minHumid.toFixed(1) + '% / Max: ' + d.maxHumid.toFixed(1) + '%';
    document.getElementById('status').textContent =
      'Last update: ' + new Date().toLocaleTimeString() + ' | SD: ' + d.sd;

    // Draw simple bar chart from history
    let chart = '';
    const temps = d.tempHist;
    const humids = d.humidHist;
    for (let i = 0; i < temps.length; i++) {
      const tw = Math.max(2, (temps[i] / 50) * 100);
      const hw = Math.max(2, (humids[i] / 100) * 100);
      chart += '<div class="chart-bar">';
      chart += '<div class="bar bar-temp" style="width:' + tw + '%"></div>';
      chart += '</div>';
      chart += '<div class="chart-bar">';
      chart += '<div class="bar bar-humid" style="width:' + hw + '%"></div>';
      chart += '</div>';
    }
    document.getElementById('chart').innerHTML = chart;
  }).catch(e => {
    document.getElementById('status').textContent = 'Connection error: ' + e;
  });
}
setInterval(update, 2000);
update();
</script>
</body></html>
)rawliteral";

  server.send(200, "text/html", html);
}

/*
 * handleData() -- JSON endpoint for AJAX requests from the dashboard.
 * Returns current readings, min/max, and history arrays.
 */
void handleData() {
  String json = "{";
  json += "\"temp\":" + String(currentTemp, 1) + ",";
  json += "\"humid\":" + String(currentHumidity, 1) + ",";
  json += "\"minTemp\":" + String(minTemp, 1) + ",";
  json += "\"maxTemp\":" + String(maxTemp, 1) + ",";
  json += "\"minHumid\":" + String(minHumidity, 1) + ",";
  json += "\"maxHumid\":" + String(maxHumidity, 1) + ",";
  json += "\"count\":" + String(readingCount) + ",";
  json += "\"sd\":\"" + String(sdAvailable ? "OK" : "N/A") + "\",";

  // Send history arrays (oldest first)
  json += "\"tempHist\":[";
  for (int i = 0; i < historyCount; i++) {
    int idx = (historyCount < HISTORY_SIZE)
              ? i
              : (historyIndex + i) % HISTORY_SIZE;
    if (i > 0) json += ",";
    json += String(tempHistory[idx], 1);
  }
  json += "],\"humidHist\":[";
  for (int i = 0; i < historyCount; i++) {
    int idx = (historyCount < HISTORY_SIZE)
              ? i
              : (historyIndex + i) % HISTORY_SIZE;
    if (i > 0) json += ",";
    json += String(humHistory[idx], 1);
  }
  json += "]}";

  server.send(200, "application/json", json);
}

// ============================================================
// WiFi Connection
// ============================================================

void connectWiFi() {
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
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connection failed -- web dashboard will be unavailable");
  }
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("  XIAO ESP32S3 Weather Station");
  Serial.println("================================");

  // --- Initialize DHT22 ---
  dht.begin();
  Serial.println("[DHT22] Initialized on pin D1");

  // --- Initialize OLED ---
  Wire.begin();  // Use default SDA (D4) and SCL (D5)
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    oledAvailable = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 25);
    display.println("Weather Station");
    display.setCursor(20, 40);
    display.println("Starting...");
    display.display();
    Serial.println("[OLED] Initialized at address 0x3C");
  } else {
    Serial.println("[OLED] Init FAILED -- continuing without display");
  }

  // --- Initialize SD Card ---
  if (SD.begin(SD_CS)) {
    sdAvailable = true;
    Serial.println("[SD] Card initialized");
    initSDLog();

    // Print card info
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SD] Card size: %llu MB\n", cardSize);
  } else {
    Serial.println("[SD] Init FAILED -- continuing without logging");
  }

  // --- Connect WiFi ---
  connectWiFi();

  // --- Start Web Server ---
  if (wifiAvailable) {
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
    Serial.println("[Web] Dashboard: http://" + WiFi.localIP().toString());
  }

  // --- Print status summary ---
  Serial.println();
  Serial.println("=== Subsystem Status ===");
  Serial.printf("  DHT22 sensor:   OK\n");
  Serial.printf("  OLED display:   %s\n", oledAvailable ? "OK" : "FAILED");
  Serial.printf("  SD card:        %s\n", sdAvailable ? "OK" : "FAILED");
  Serial.printf("  WiFi:           %s\n", wifiAvailable ? "OK" : "FAILED");
  Serial.println("========================\n");

  // Take first reading immediately
  readDHT();
}

// ============================================================
// Main Loop -- Non-blocking multi-tasking with millis()
// ============================================================

void loop() {
  unsigned long now = millis();

  // Task 1: Read DHT22 sensor every 2 seconds
  if (now - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = now;
    readDHT();
  }

  // Task 2: Update OLED display every 1 second
  if (now - lastOLEDUpdate >= OLED_INTERVAL) {
    lastOLEDUpdate = now;
    updateOLED();
  }

  // Task 3: Log to SD card every 30 seconds
  if (now - lastSDLog >= SD_LOG_INTERVAL) {
    lastSDLog = now;
    logToSD();
  }

  // Task 4: Handle web server requests (runs every loop iteration)
  if (wifiAvailable) {
    server.handleClient();
  }

  // Small yield to prevent watchdog issues
  delay(1);
}
