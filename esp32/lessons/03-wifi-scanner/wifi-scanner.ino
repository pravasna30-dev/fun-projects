/*
 * Lesson 3: Wi-Fi Scanner
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch scans for nearby Wi-Fi networks and displays:
 *   - Network name (SSID)
 *   - Signal strength (RSSI) with a visual bar
 *   - Encryption type (Open, WPA2, WPA3, etc.)
 *   - Wi-Fi channel
 *
 * Results are sorted by signal strength (strongest first).
 * The scan repeats every 10 seconds.
 *
 * No extra hardware required. The ESP32-S3 has built-in Wi-Fi.
 */

// Include the Wi-Fi library. This comes with the ESP32 board package.
// It provides all the functions we need to scan for and connect to networks.
#include <WiFi.h>

// ---------- CONSTANTS ----------

// How long to wait between scans, in milliseconds.
const unsigned long SCAN_INTERVAL = 10000;  // 10 seconds

// Built-in LED for visual feedback during scanning.
const int LED_PIN = 21;

// ---------- SETUP ----------

void setup() {
  // Start serial communication.
  Serial.begin(115200);
  delay(1000);

  // Configure LED pin.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Print welcome banner.
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Lesson 3: Wi-Fi Scanner");
  Serial.println("  Board: XIAO ESP32-S3 Sense");
  Serial.println("========================================");
  Serial.println();

  // Set Wi-Fi to Station mode.
  // There are several Wi-Fi modes:
  //   WIFI_STA  = Station mode (connects TO a network, like your phone does)
  //   WIFI_AP   = Access Point mode (creates its own network)
  //   WIFI_AP_STA = Both at the same time
  // For scanning, we use Station mode.
  WiFi.mode(WIFI_STA);

  // Disconnect from any previously connected network.
  // We just want to scan, not connect.
  WiFi.disconnect();
  delay(100);

  Serial.println("Wi-Fi initialized in Station mode.");
  Serial.println("Starting network scan...");
  Serial.println();
}

// ---------- LOOP ----------

void loop() {
  // Turn LED on to indicate scanning is in progress.
  digitalWrite(LED_PIN, HIGH);

  // Perform a Wi-Fi scan.
  // WiFi.scanNetworks() is a BLOCKING call — it pauses your program
  // until the scan is complete (usually 1-3 seconds).
  // It returns the number of networks found, or a negative number on error.
  Serial.println("Scanning...");
  int networkCount = WiFi.scanNetworks();

  // Turn LED off — scan is complete.
  digitalWrite(LED_PIN, LOW);

  // Check the result.
  if (networkCount == 0) {
    Serial.println("No networks found.");
  } else if (networkCount < 0) {
    Serial.println("Scan failed! Error code: " + String(networkCount));
  } else {
    // We found networks! Display them.
    Serial.printf("Found %d network(s):\n\n", networkCount);

    // Sort networks by RSSI (signal strength), strongest first.
    // WiFi.scanNetworks() doesn't guarantee order, so we sort manually.
    // We'll create an array of indices and sort that.
    int indices[networkCount];
    for (int i = 0; i < networkCount; i++) {
      indices[i] = i;
    }

    // Simple bubble sort by RSSI (descending — strongest first).
    for (int i = 0; i < networkCount - 1; i++) {
      for (int j = 0; j < networkCount - i - 1; j++) {
        if (WiFi.RSSI(indices[j]) < WiFi.RSSI(indices[j + 1])) {
          // Swap.
          int temp = indices[j];
          indices[j] = indices[j + 1];
          indices[j + 1] = temp;
        }
      }
    }

    // Print table header.
    Serial.println("  #  | RSSI  | Signal     | Ch | Encryption   | SSID");
    Serial.println("-----|-------|------------|----|--------------|-----------------------------");

    // Print each network.
    for (int i = 0; i < networkCount; i++) {
      int idx = indices[i];

      // Get network details.
      String ssid = WiFi.SSID(idx);
      int32_t rssi = WiFi.RSSI(idx);
      int channel = WiFi.channel(idx);
      wifi_auth_mode_t encType = WiFi.encryptionType(idx);

      // If SSID is empty, it's a hidden network.
      if (ssid.length() == 0) {
        ssid = "(hidden)";
      }

      // Build a visual signal strength bar.
      String signalBar = getSignalBar(rssi);

      // Get a human-readable encryption type.
      String encryption = getEncryptionType(encType);

      // Print the row, formatted into neat columns.
      Serial.printf("  %2d | %4d  | %-10s | %2d | %-12s | %s\n",
                     i + 1, rssi, signalBar.c_str(), channel,
                     encryption.c_str(), ssid.c_str());
    }

    Serial.println();

    // Print signal strength legend.
    printSignalLegend();
  }

  // Clean up scan results from memory.
  // This is important — scan results consume RAM.
  WiFi.scanDelete();

  // Print separator and wait before the next scan.
  Serial.printf("\nNext scan in %lu seconds...\n", SCAN_INTERVAL / 1000);
  Serial.println("========================================\n");

  delay(SCAN_INTERVAL);
}

// ---------- HELPER FUNCTIONS ----------

// Convert an RSSI value to a visual bar made of block characters.
// RSSI ranges:
//   -30 dBm = Amazing (you're basically touching the router)
//   -50 dBm = Excellent
//   -60 dBm = Good
//   -70 dBm = Fair
//   -80 dBm = Weak
//   -90 dBm = Barely usable
String getSignalBar(int32_t rssi) {
  // Map RSSI to a 0-5 bar scale.
  int bars;
  if (rssi >= -50) bars = 5;       // Excellent
  else if (rssi >= -60) bars = 4;  // Good
  else if (rssi >= -70) bars = 3;  // Fair
  else if (rssi >= -80) bars = 2;  // Weak
  else if (rssi >= -90) bars = 1;  // Very weak
  else bars = 0;                   // Unusable

  // Build the bar string using block characters.
  String bar = "";
  for (int i = 0; i < bars; i++) {
    bar += "##";
  }
  // Fill remaining space with dots for visual consistency.
  for (int i = bars; i < 5; i++) {
    bar += "..";
  }
  return bar;
}

// Convert the wifi_auth_mode_t enum to a human-readable string.
// This tells you what kind of security the network uses.
String getEncryptionType(wifi_auth_mode_t encType) {
  switch (encType) {
    case WIFI_AUTH_OPEN:
      return "Open";            // No password required — not secure!
    case WIFI_AUTH_WEP:
      return "WEP";             // Very old, easily cracked — avoid.
    case WIFI_AUTH_WPA_PSK:
      return "WPA";             // Older security standard.
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";            // Current standard — good security.
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";        // Supports both WPA and WPA2 clients.
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2-Ent";        // Enterprise — uses username/password or certificates.
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";            // Newest standard — best security.
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/WPA3";       // Supports both WPA2 and WPA3 clients.
    default:
      return "Unknown";
  }
}

// Print a legend explaining signal strength levels.
void printSignalLegend() {
  Serial.println("Signal Strength Legend:");
  Serial.println("  ########## (-50 or better) = Excellent");
  Serial.println("  ########.. (-60 to -51)    = Good");
  Serial.println("  ######.... (-70 to -61)    = Fair");
  Serial.println("  ####...... (-80 to -71)    = Weak");
  Serial.println("  ##........ (-90 to -81)    = Very weak");
  Serial.println("  .......... (below -90)     = Unusable");
}
