/*
 * Lesson 4: Web Server + LED Control
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch connects the ESP32-S3 to your Wi-Fi network and
 * creates a web server. You can then open a web page in your phone
 * or computer's browser to turn the built-in LED on and off.
 *
 * How it works:
 *   1. The board connects to your home Wi-Fi network.
 *   2. It starts a web server on port 80 (the default HTTP port).
 *   3. You open the board's IP address in a browser.
 *   4. The web page has ON and OFF buttons.
 *   5. When you click a button, the browser sends an HTTP request
 *      to the board, which toggles the LED accordingly.
 *
 * IMPORTANT: Replace "YOUR_SSID" and "YOUR_PASSWORD" with your
 * actual Wi-Fi network name and password before uploading.
 *
 * No extra hardware required beyond the built-in LED.
 */

#include <WiFi.h>        // Wi-Fi connectivity
#include <WebServer.h>   // HTTP web server library

// ---------- CONFIGURATION ----------
// Replace these with your actual Wi-Fi credentials.
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// ---------- CONSTANTS ----------
const int LED_PIN = 21;   // Built-in LED pin

// ---------- GLOBAL OBJECTS ----------

// Create a web server object that listens on port 80.
// Port 80 is the default port for HTTP, so you don't need to
// type a port number in the browser URL.
WebServer server(80);

// Track the current LED state.
bool ledState = false;

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure the LED pin.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Print startup banner.
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Lesson 4: Web Server + LED Control");
  Serial.println("  Board: XIAO ESP32-S3 Sense");
  Serial.println("========================================");
  Serial.println();

  // --- Connect to Wi-Fi ---
  connectToWiFi();

  // --- Set up web server routes ---
  // A "route" maps a URL path to a function that handles it.
  // When someone visits http://<board-ip>/, the handleRoot function runs.
  server.on("/", handleRoot);

  // When someone visits http://<board-ip>/on, turn the LED on.
  server.on("/on", handleLedOn);

  // When someone visits http://<board-ip>/off, turn the LED off.
  server.on("/off", handleLedOff);

  // When someone visits http://<board-ip>/toggle, toggle the LED.
  server.on("/toggle", handleLedToggle);

  // When someone visits http://<board-ip>/status, return LED state as JSON.
  server.on("/status", handleStatus);

  // If someone visits a URL we haven't defined, show a 404 page.
  server.onNotFound(handleNotFound);

  // Start the web server.
  server.begin();
  Serial.println("Web server started!");
  Serial.println();
  Serial.print("Open your browser and go to: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// ---------- LOOP ----------

void loop() {
  // This is critical: the server.handleClient() function checks if
  // any HTTP requests have come in and dispatches them to the
  // appropriate handler function.
  // Without this line, the server won't respond to any requests.
  server.handleClient();
}

// ---------- WI-FI CONNECTION ----------

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  // Begin connecting to the Wi-Fi network.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for the connection to establish.
  // WiFi.status() returns the current connection state.
  // WL_CONNECTED means we've successfully connected.
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;

    // Give up after 30 seconds (60 attempts * 500ms).
    if (attempts > 60) {
      Serial.println();
      Serial.println("Failed to connect to Wi-Fi!");
      Serial.println("Check your SSID and password, then reset the board.");
      while (true) {
        delay(1000);  // Halt here.
      }
    }
  }

  // Connected! Print the details.
  Serial.println();
  Serial.println("Wi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// ---------- REQUEST HANDLERS ----------

// Handle the root page (/).
// This serves the main HTML page with the LED control buttons.
void handleRoot() {
  Serial.println("Request: GET /");

  // Build the HTML page as a string.
  String html = buildHtmlPage();

  // Send the response.
  // 200 = HTTP status "OK"
  // "text/html" = the content type (tells the browser it's HTML)
  server.send(200, "text/html", html);
}

// Handle the /on route.
void handleLedOn() {
  Serial.println("Request: GET /on");
  ledState = true;
  digitalWrite(LED_PIN, HIGH);

  // Redirect back to the main page so the user sees the updated state.
  server.sendHeader("Location", "/");
  server.send(303);  // 303 = "See Other" (redirect after POST/GET)
}

// Handle the /off route.
void handleLedOff() {
  Serial.println("Request: GET /off");
  ledState = false;
  digitalWrite(LED_PIN, LOW);

  // Redirect back to the main page.
  server.sendHeader("Location", "/");
  server.send(303);
}

// Handle the /toggle route.
void handleLedToggle() {
  Serial.println("Request: GET /toggle");
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  // Redirect back to the main page.
  server.sendHeader("Location", "/");
  server.send(303);
}

// Handle the /status route — returns JSON for programmatic access.
void handleStatus() {
  Serial.println("Request: GET /status");

  String json = "{";
  json += "\"led\": " + String(ledState ? "true" : "false") + ",";
  json += "\"uptime\": " + String(millis() / 1000) + ",";
  json += "\"rssi\": " + String(WiFi.RSSI());
  json += "}";

  server.send(200, "application/json", json);
}

// Handle unknown URLs — return a 404 error page.
void handleNotFound() {
  Serial.print("Request: 404 - ");
  Serial.println(server.uri());

  String message = "<!DOCTYPE html><html><body>";
  message += "<h1>404 - Page Not Found</h1>";
  message += "<p>The page <b>" + server.uri() + "</b> was not found.</p>";
  message += "<a href='/'>Go back to the main page</a>";
  message += "</body></html>";

  server.send(404, "text/html", message);
}

// ---------- HTML PAGE BUILDER ----------

// Build the main HTML page with inline CSS styling.
// The page shows the LED state and provides ON/OFF/Toggle buttons.
String buildHtmlPage() {
  // Determine the LED status text and color.
  String stateText = ledState ? "ON" : "OFF";
  String stateColor = ledState ? "#4CAF50" : "#f44336";  // Green or Red

  String html = "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  // The viewport meta tag makes the page mobile-friendly.
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32-S3 LED Control</title>";

  // --- Inline CSS ---
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body {";
  html += "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
  html += "  background: #1a1a2e; color: #eee; min-height: 100vh;";
  html += "  display: flex; justify-content: center; align-items: center;";
  html += "}";
  html += ".container {";
  html += "  text-align: center; padding: 40px; max-width: 400px; width: 90%;";
  html += "  background: #16213e; border-radius: 16px;";
  html += "  box-shadow: 0 8px 32px rgba(0,0,0,0.3);";
  html += "}";
  html += "h1 { font-size: 1.5em; margin-bottom: 8px; }";
  html += ".subtitle { color: #888; font-size: 0.9em; margin-bottom: 24px; }";
  html += ".status {";
  html += "  font-size: 2em; font-weight: bold; margin: 20px 0;";
  html += "  color: " + stateColor + ";";
  html += "}";
  html += ".led-indicator {";
  html += "  width: 60px; height: 60px; border-radius: 50%; margin: 0 auto 24px;";
  html += "  background: " + stateColor + ";";
  html += "  box-shadow: 0 0 20px " + stateColor + ";";
  html += "  transition: all 0.3s;";
  html += "}";
  html += ".buttons { display: flex; gap: 12px; justify-content: center; flex-wrap: wrap; }";
  html += "a.btn {";
  html += "  display: inline-block; padding: 14px 32px; border-radius: 8px;";
  html += "  text-decoration: none; font-size: 1.1em; font-weight: bold;";
  html += "  transition: transform 0.1s, box-shadow 0.1s; cursor: pointer;";
  html += "}";
  html += "a.btn:active { transform: scale(0.95); }";
  html += ".btn-on { background: #4CAF50; color: white; }";
  html += ".btn-off { background: #f44336; color: white; }";
  html += ".btn-toggle { background: #2196F3; color: white; }";
  html += ".info { margin-top: 24px; font-size: 0.8em; color: #666; }";
  html += "</style>";
  html += "</head>";

  // --- HTML Body ---
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>ESP32-S3 LED Control</h1>";
  html += "<p class='subtitle'>XIAO ESP32-S3 Sense</p>";
  html += "<div class='led-indicator'></div>";
  html += "<div class='status'>LED is " + stateText + "</div>";
  html += "<div class='buttons'>";
  html += "<a class='btn btn-on' href='/on'>ON</a>";
  html += "<a class='btn btn-off' href='/off'>OFF</a>";
  html += "<a class='btn btn-toggle' href='/toggle'>TOGGLE</a>";
  html += "</div>";
  html += "<p class='info'>IP: " + WiFi.localIP().toString() + " | RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "</div>";
  html += "</body></html>";

  return html;
}
