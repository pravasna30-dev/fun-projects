/*
 * Lesson 11: MQTT IoT Dashboard
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * This sketch connects to a public MQTT broker, publishes simulated
 * sensor data every 5 seconds, and subscribes to a topic to control
 * the built-in LED remotely.
 *
 * Libraries required:
 *   - PubSubClient by Nick O'Leary (install via Library Manager)
 *
 * MQTT Broker: broker.hivemq.com (free, public, no auth required)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ─── Configuration ───────────────────────────────────────────────────────────

// Replace these with your WiFi credentials
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// MQTT broker settings
const char* mqttBroker = "broker.hivemq.com";
const int   mqttPort   = 1883;

// Built-in LED pin on the XIAO ESP32-S3
const int LED_PIN = 21;

// How often to publish sensor data (milliseconds)
const unsigned long PUBLISH_INTERVAL = 5000;

// ─── Global Objects ──────────────────────────────────────────────────────────

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// Topic strings (built dynamically from device ID)
String deviceId;
String topicTemperature;
String topicUptime;
String topicHeap;
String topicLedControl;
String topicLedStatus;

// Timing
unsigned long lastPublishTime = 0;

// ─── Helper: Generate a Unique Device ID from MAC Address ────────────────────

String getDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);

  char id[20];
  // Use the last 3 bytes of the MAC address for a short, unique suffix
  snprintf(id, sizeof(id), "esp32_%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(id);
}

// ─── WiFi Setup ──────────────────────────────────────────────────────────────

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

// ─── MQTT Callback ──────────────────────────────────────────────────────────
// This function is called automatically whenever a message arrives
// on any topic we have subscribed to.

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert the payload bytes to a String for easy comparison
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Check if this is the LED control topic
  if (String(topic) == topicLedControl) {
    if (message == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED turned ON");
      // Publish the new LED status (retained so new subscribers see it)
      mqttClient.publish(topicLedStatus.c_str(), "ON", true);
    } else if (message == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED turned OFF");
      mqttClient.publish(topicLedStatus.c_str(), "OFF", true);
    } else {
      Serial.println("Unknown command. Use ON or OFF.");
    }
  }
}

// ─── MQTT Reconnect ─────────────────────────────────────────────────────────
// If the connection to the broker drops, this function attempts to reconnect.
// After reconnecting, it re-subscribes to topics (subscriptions are lost on
// disconnect).

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");

    // Attempt to connect with our device ID as the client identifier.
    // The last two parameters set a "Last Will and Testament" (LWT) message:
    // if the ESP32 disconnects unexpectedly, the broker publishes "offline"
    // to the status topic on our behalf.
    String willTopic = deviceId + "/status";
    if (mqttClient.connect(deviceId.c_str(), willTopic.c_str(), 0, true, "offline")) {
      Serial.println(" connected!");

      // Publish an "online" status (retained)
      mqttClient.publish(willTopic.c_str(), "online", true);

      // Subscribe to the LED control topic with QoS 0
      mqttClient.subscribe(topicLedControl.c_str());
      Serial.print("Subscribed to: ");
      Serial.println(topicLedControl);

    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ─── Publish Sensor Data ────────────────────────────────────────────────────

void publishSensorData() {
  // Simulated temperature: random value between 25.0 and 35.0
  float temperature = 25.0 + (random(0, 100) / 10.0);
  char tempStr[10];
  dtostrf(temperature, 4, 1, tempStr);

  // Uptime in seconds
  unsigned long uptimeSeconds = millis() / 1000;
  char uptimeStr[12];
  snprintf(uptimeStr, sizeof(uptimeStr), "%lu", uptimeSeconds);

  // Free heap memory in bytes
  uint32_t freeHeap = ESP.getFreeHeap();
  char heapStr[12];
  snprintf(heapStr, sizeof(heapStr), "%u", freeHeap);

  // Publish each value to its own topic
  mqttClient.publish(topicTemperature.c_str(), tempStr);
  mqttClient.publish(topicUptime.c_str(), uptimeStr);
  mqttClient.publish(topicHeap.c_str(), heapStr);

  // Print to serial for debugging
  Serial.println("─── Published ───");
  Serial.print("  Temperature: ");
  Serial.print(tempStr);
  Serial.println(" C");
  Serial.print("  Uptime:      ");
  Serial.print(uptimeStr);
  Serial.println(" sec");
  Serial.print("  Free Heap:   ");
  Serial.print(heapStr);
  Serial.println(" bytes");
}

// ─── Setup ──────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Lesson 11: MQTT IoT Dashboard ===");
  Serial.println();

  // Configure the built-in LED as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Generate a unique device ID from the MAC address
  deviceId = getDeviceId();
  Serial.print("Device ID: ");
  Serial.println(deviceId);

  // Build topic strings
  // Using a unique prefix avoids collisions on the public broker
  topicTemperature = deviceId + "/sensors/temperature";
  topicUptime      = deviceId + "/sensors/uptime";
  topicHeap        = deviceId + "/sensors/heap";
  topicLedControl  = deviceId + "/led/control";
  topicLedStatus   = deviceId + "/led/status";

  Serial.println("Topics:");
  Serial.print("  Publish:   ");
  Serial.println(topicTemperature);
  Serial.print("  Publish:   ");
  Serial.println(topicUptime);
  Serial.print("  Publish:   ");
  Serial.println(topicHeap);
  Serial.print("  Subscribe: ");
  Serial.println(topicLedControl);
  Serial.println();

  // Connect to WiFi
  setupWiFi();

  // Configure the MQTT client
  mqttClient.setServer(mqttBroker, mqttPort);
  mqttClient.setCallback(mqttCallback);

  // Seed the random number generator for simulated temperature
  randomSeed(analogRead(0));
}

// ─── Loop ───────────────────────────────────────────────────────────────────

void loop() {
  // Ensure we are connected to the MQTT broker
  if (!mqttClient.connected()) {
    reconnect();
  }

  // The PubSubClient library needs loop() called regularly to:
  //  - Process incoming messages
  //  - Send keepalive pings to the broker
  //  - Maintain the TCP connection
  mqttClient.loop();

  // Publish sensor data at the defined interval
  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = now;
    publishSensorData();
  }
}
