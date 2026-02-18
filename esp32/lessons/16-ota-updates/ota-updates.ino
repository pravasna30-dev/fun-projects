/*
 * Lesson 16: OTA (Over-the-Air) Updates
 *
 * Board: Seeed Studio XIAO ESP32S3 Sense
 *
 * This sketch demonstrates Over-the-Air firmware updates using the
 * ArduinoOTA library. After the initial USB upload, all subsequent
 * firmware updates can be done wirelessly over WiFi.
 *
 * The sketch also runs a simple web server that displays device
 * information (IP address, uptime, firmware version, free heap memory).
 *
 * Setup:
 * 1. Replace YOUR_SSID and YOUR_PASSWORD with your WiFi credentials
 * 2. Upload via USB the first time
 * 3. After that, select the network port in Arduino IDE for OTA uploads
 *
 * OTA Password: xiao-ota-2024
 * Hostname: xiao-esp32s3 (accessible at xiao-esp32s3.local)
 */

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>

// ============================================================
// Configuration
// ============================================================

// WiFi credentials -- replace with your network details
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD  = "YOUR_PASSWORD";

// OTA configuration
const char* OTA_HOSTNAME = "xiao-esp32s3";
const char* OTA_PASSWORD = "xiao-ota-2024";

// Firmware version -- update this each time you deploy new code
// so you can verify OTA worked by checking the web dashboard
#define FIRMWARE_VERSION "1.0.0"

// Built-in LED for status indication
#define LED_PIN 21

// Web server on port 80
WebServer server(80);

// Track when the device started (for uptime calculation)
unsigned long bootTime;

// ============================================================
// Web Server Handlers
// ============================================================

/*
 * formatUptime() -- Convert milliseconds to a human-readable string.
 * Returns something like "2h 15m 30s".
 */
String formatUptime() {
  unsigned long seconds = (millis() - bootTime) / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours   = minutes / 60;
  unsigned long days    = hours / 24;

  seconds %= 60;
  minutes %= 60;
  hours   %= 24;

  String result = "";
  if (days > 0)    result += String(days) + "d ";
  if (hours > 0)   result += String(hours) + "h ";
  if (minutes > 0) result += String(minutes) + "m ";
  result += String(seconds) + "s";

  return result;
}

/*
 * handleRoot() -- Serves the device info web page.
 * This page auto-refreshes every 5 seconds so you can watch
 * the uptime tick and heap memory fluctuate.
 */
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='5'>";  // Auto-refresh every 5 seconds
  html += "<title>XIAO ESP32S3 - Device Info</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Arial, sans-serif; max-width: 600px; ";
  html += "margin: 40px auto; padding: 20px; background: #1a1a2e; color: #e0e0e0; }";
  html += "h1 { color: #00d4ff; border-bottom: 2px solid #00d4ff; padding-bottom: 10px; }";
  html += ".info-card { background: #16213e; border-radius: 10px; padding: 20px; ";
  html += "margin: 15px 0; box-shadow: 0 2px 10px rgba(0,0,0,0.3); }";
  html += ".label { color: #8892b0; font-size: 0.85em; text-transform: uppercase; }";
  html += ".value { font-size: 1.4em; color: #ccd6f6; margin-top: 5px; }";
  html += ".ota-status { background: #0a3d0a; border-left: 4px solid #00ff41; ";
  html += "padding: 10px 15px; border-radius: 0 5px 5px 0; margin: 20px 0; }";
  html += ".footer { text-align: center; margin-top: 30px; color: #555; font-size: 0.8em; }";
  html += "</style></head><body>";

  html += "<h1>XIAO ESP32S3 Device Info</h1>";

  html += "<div class='ota-status'>OTA Updates: ENABLED | Hostname: <strong>";
  html += OTA_HOSTNAME;
  html += ".local</strong></div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>IP Address</div>";
  html += "<div class='value'>" + WiFi.localIP().toString() + "</div>";
  html += "</div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>Firmware Version</div>";
  html += "<div class='value'>v" FIRMWARE_VERSION "</div>";
  html += "</div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>Uptime</div>";
  html += "<div class='value'>" + formatUptime() + "</div>";
  html += "</div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>Free Heap Memory</div>";
  html += "<div class='value'>" + String(ESP.getFreeHeap() / 1024) + " KB</div>";
  html += "</div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>WiFi Signal Strength (RSSI)</div>";
  html += "<div class='value'>" + String(WiFi.RSSI()) + " dBm</div>";
  html += "</div>";

  html += "<div class='info-card'>";
  html += "<div class='label'>MAC Address</div>";
  html += "<div class='value'>" + WiFi.macAddress() + "</div>";
  html += "</div>";

  html += "<div class='footer'>Page auto-refreshes every 5 seconds</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

/*
 * handleNotFound() -- Returns a 404 for any unrecognized URL.
 */
void handleNotFound() {
  server.send(404, "text/plain", "404 - Not Found");
}

// ============================================================
// OTA Setup
// ============================================================

/*
 * setupOTA() -- Configures ArduinoOTA with hostname, password,
 * and callback functions for progress indication.
 */
void setupOTA() {
  // Set the hostname for mDNS discovery.
  // This makes the device appear as "xiao-esp32s3" in the Arduino IDE
  // port list and accessible at "xiao-esp32s3.local" in a browser.
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  // Set a password to prevent unauthorized firmware uploads.
  // Anyone on your network could otherwise flash arbitrary code
  // to your device. The password is hashed before transmission
  // but this is still basic security -- not suitable for production.
  ArduinoOTA.setPassword(OTA_PASSWORD);

  // Callback: OTA update is starting
  ArduinoOTA.onStart([]() {
    // Determine what type of update is being applied
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";      // New application firmware
    } else {
      type = "filesystem";  // SPIFFS/LittleFS filesystem update
    }
    Serial.println("\n[OTA] Update starting: " + type);

    // Turn on LED to indicate update in progress
    digitalWrite(LED_PIN, LOW);  // LED is active LOW on XIAO
  });

  // Callback: OTA update is complete
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update complete! Rebooting...");
    digitalWrite(LED_PIN, HIGH);  // Turn off LED
  });

  // Callback: Progress update (called repeatedly during upload)
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = progress / (total / 100);
    Serial.printf("[OTA] Progress: %u%%\r", percent);

    // Blink LED during upload to give visual feedback
    // LED toggles every 10% of progress
    if (percent % 10 == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  });

  // Callback: Error occurred during OTA
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);

    switch (error) {
      case OTA_AUTH_ERROR:
        Serial.println("Authentication failed");
        break;
      case OTA_BEGIN_ERROR:
        Serial.println("Begin failed - not enough space?");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Connection failed");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Receive failed");
        break;
      case OTA_END_ERROR:
        Serial.println("End failed");
        break;
    }

    // Rapid blink to indicate error
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
  });

  // Start the OTA service
  ArduinoOTA.begin();
  Serial.println("[OTA] Service started");
  Serial.println("[OTA] Hostname: " + String(OTA_HOSTNAME) + ".local");
  Serial.println("[OTA] Password protected: YES");
}

// ============================================================
// WiFi Connection
// ============================================================

/*
 * connectWiFi() -- Connects to the configured WiFi network.
 * Blocks until connected (with timeout). The built-in LED
 * blinks while connecting.
 */
void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);  // Station mode (client, not access point)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection with a 20-second timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");

    // Blink LED while connecting
    digitalWrite(LED_PIN, (attempts % 2 == 0) ? LOW : HIGH);
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Solid LED off to indicate connected
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("[WiFi] Connection FAILED!");
    Serial.println("[WiFi] Check your SSID and password.");

    // Rapid blink to indicate failure
    while (true) {
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
    }
  }
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give Serial time to initialize

  // Initialize built-in LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Start with LED off (active LOW)

  Serial.println("================================");
  Serial.println("  XIAO ESP32S3 OTA Updates");
  Serial.println("  Firmware v" FIRMWARE_VERSION);
  Serial.println("================================");

  // Record boot time for uptime calculation
  bootTime = millis();

  // Step 1: Connect to WiFi (required for both OTA and web server)
  connectWiFi();

  // Step 2: Set up OTA update service
  setupOTA();

  // Step 3: Set up web server for device info
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[Web] Server started on port 80");

  // Print summary
  Serial.println();
  Serial.println("=== Ready! ===");
  Serial.println("Web Dashboard: http://" + WiFi.localIP().toString());
  Serial.println("mDNS Address:  http://" + String(OTA_HOSTNAME) + ".local");
  Serial.println("OTA Port:      Select '" + String(OTA_HOSTNAME) + "' in Arduino IDE");
  Serial.println();
}

// ============================================================
// Main Loop
// ============================================================

void loop() {
  // IMPORTANT: ArduinoOTA.handle() must be called frequently.
  // It checks for incoming OTA update requests. If you have
  // long-running operations in your loop, make sure to call
  // this regularly or the OTA service won't respond.
  ArduinoOTA.handle();

  // Handle incoming web server requests
  server.handleClient();

  // Small delay to prevent watchdog timer issues
  delay(10);
}
