# Lesson 4: Web Server + LED Control

**Difficulty:** Beginner-Intermediate

**Estimated Time:** 30-45 minutes

---

## What You'll Learn

- How to connect the ESP32-S3 to your home Wi-Fi network
- What a web server is and how HTTP requests work
- How to create a web server on the ESP32-S3
- How to serve HTML pages with inline CSS styling
- How to control hardware (the LED) from a web browser on your phone or computer
- What routes, status codes, and redirects are

## What You'll Need

| Item | Quantity | Notes |
|------|----------|-------|
| Seeed Studio XIAO ESP32-S3 Sense | 1 | Built-in LED on pin 21 |
| USB-C cable | 1 | Data-capable |
| Computer with Arduino IDE | 1 | |
| Wi-Fi network | 1 | Your home network with known SSID and password |
| Phone or second computer | 1 | To access the web page (optional — you can use the same computer) |

No extra hardware components needed.

---

## Step-by-Step Instructions

### 1. Configure Your Wi-Fi Credentials

Before uploading, you **must** edit the sketch to use your actual Wi-Fi network:

1. Open `web-server-led.ino` in the Arduino IDE.
2. Find these two lines near the top:
   ```cpp
   const char* WIFI_SSID     = "YOUR_SSID";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   ```
3. Replace `YOUR_SSID` with your Wi-Fi network name (case-sensitive).
4. Replace `YOUR_PASSWORD` with your Wi-Fi password.

For example:
```cpp
const char* WIFI_SSID     = "MyHomeNetwork";
const char* WIFI_PASSWORD = "supersecret123";
```

> **Security note:** Be careful not to share or commit this file with your actual password in it.

### 2. Upload the Sketch

1. Select **XIAO_ESP32S3** as your board and `/dev/cu.usbmodem101` as your port.
2. Click **Upload** and wait for "Done uploading."

### 3. Find Your Board's IP Address

1. Open the Serial Monitor at **115200** baud.
2. Watch the connection progress (dots appearing as it connects).
3. Once connected, you will see a message like:
   ```
   Wi-Fi connected!
   IP Address: 192.168.1.42
   ```
4. **Write down this IP address.** You will type it into your browser.

### 4. Open the Web Page

1. Open a web browser on your computer or phone.
2. Make sure your computer/phone is on the **same Wi-Fi network** as the board.
3. Type the IP address into the address bar: `http://192.168.1.42` (use your actual IP).
4. You should see a styled page with the LED status and ON/OFF/TOGGLE buttons.

### 5. Control the LED

1. Click the **ON** button. The LED on the board should light up, and the web page updates to show "LED is ON" with a green indicator.
2. Click the **OFF** button. The LED turns off and the page shows red.
3. Click **TOGGLE** to flip the state.
4. Watch the Serial Monitor — you'll see each request logged.

### 6. Try from Your Phone

Open the same IP address in your phone's browser (while connected to the same Wi-Fi). You can now control the LED from across the room.

---

## The Code

### Connecting to Wi-Fi

```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
}
```

`WiFi.begin()` starts the connection process. It runs in the background while we poll `WiFi.status()` in a loop. Once the status becomes `WL_CONNECTED`, we're on the network and have been assigned an IP address by the router.

### Creating the Web Server

```cpp
WebServer server(80);
```

This creates a web server that listens on **port 80**, the default for HTTP. When a browser navigates to an IP address without specifying a port, it uses port 80 by default.

### Defining Routes

```cpp
server.on("/", handleRoot);
server.on("/on", handleLedOn);
server.on("/off", handleLedOff);
```

A **route** maps a URL path to a handler function. When someone visits `http://192.168.1.42/on`, the server calls `handleLedOn()`. The `/` route is the "home page."

### Handling Requests

```cpp
void handleLedOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}
```

Each handler function does three things:
1. Performs the action (turn LED on/off).
2. Sends an HTTP redirect back to the home page using status code 303.
3. The browser then loads the home page again, showing the updated state.

### Serving HTML

```cpp
server.send(200, "text/html", html);
```

The `send()` function takes three arguments:
- **200** — the HTTP status code ("OK, here's your page").
- **"text/html"** — the content type (tells the browser to render it as HTML).
- **html** — the actual content (our HTML string).

### The Main Loop

```cpp
void loop() {
  server.handleClient();
}
```

This single line is essential. It checks for incoming HTTP requests and routes them to the correct handler. Without it, the server will never respond.

---

## How It Works

### What is a Web Server?

A **web server** is a program that waits for requests from clients (usually web browsers) and responds with content (usually HTML pages). When you type a URL into your browser:

1. Your browser sends an **HTTP request** to the server.
2. The server processes the request.
3. The server sends back an **HTTP response** containing the web page.

Normally, web servers run on powerful computers in data centers. But the ESP32-S3 is powerful enough to run a simple web server too.

### What is HTTP?

**HTTP** (HyperText Transfer Protocol) is the language that browsers and web servers use to communicate. An HTTP request has:

- **Method:** GET (retrieve data), POST (send data), etc.
- **Path:** The URL path, like `/on` or `/off`.
- **Headers:** Metadata like content type, browser info, etc.

An HTTP response has:

- **Status code:** 200 (OK), 303 (Redirect), 404 (Not Found), etc.
- **Headers:** Metadata like content type, redirect location, etc.
- **Body:** The actual content (HTML, JSON, etc.).

### How the Browser Talks to the Board

```
[Your Phone]  ---Wi-Fi--->  [Your Router]  ---Wi-Fi--->  [ESP32-S3]
   Browser                                                Web Server
     |                                                       |
     |  GET /on HTTP/1.1                                     |
     |------------------------------------------------------>|
     |                                                       |
     |  HTTP/1.1 303 See Other                               |
     |  Location: /                                          |
     |<------------------------------------------------------|
     |                                                       |
     |  GET / HTTP/1.1                                       |
     |------------------------------------------------------>|
     |                                                       |
     |  HTTP/1.1 200 OK                                      |
     |  Content-Type: text/html                              |
     |  <html>...</html>                                     |
     |<------------------------------------------------------|
```

### Why Do We Redirect?

After turning the LED on/off, we don't send the HTML page directly. Instead, we send a **303 redirect** back to `/`. This prevents the "resubmit form" problem — if the user refreshes the page, the browser reloads `/` instead of re-sending `/on`.

### IP Addresses

Every device on a network has an **IP address** — a unique number that identifies it, like a phone number. Your router assigns IP addresses to devices. The ESP32's IP (like `192.168.1.42`) is only accessible from within your local network.

---

## Try It Out

1. Upload the sketch with your Wi-Fi credentials.
2. Open the Serial Monitor and note the IP address.
3. Open the IP in a browser on the same network.
4. Click ON, OFF, and TOGGLE. Verify the LED changes.
5. Watch the Serial Monitor for request logs.
6. Try visiting `http://<IP>/status` in your browser. You'll see a JSON response.
7. Try visiting `http://<IP>/nonexistent` to see the 404 error page.
8. Try accessing the page from your phone.

---

## Challenges

1. **Add a brightness slider.** Use the ESP32's `analogWrite()` (actually `ledcWrite()` on ESP32) to control LED brightness via PWM. Add an HTML range slider (`<input type="range">`) and create a new route like `/brightness?value=128` to set the level.

2. **Add auto-refresh.** Add a `<meta http-equiv="refresh" content="5">` tag to the HTML to make the page automatically reload every 5 seconds. Display the uptime on the page to confirm it's refreshing.

3. **Add multiple LED patterns.** Create buttons for "Blink", "Pulse", and "SOS" patterns. When clicked, the board runs the pattern. Hint: you'll need to use `millis()` instead of `delay()` to avoid blocking the web server.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **"Failed to connect to Wi-Fi"** | Double-check SSID and password (case-sensitive). Make sure it's a 2.4 GHz network, not 5 GHz only. Move closer to the router. |
| **Connected but can't open the page** | Ensure your computer/phone is on the **same Wi-Fi network**. Check the IP address is correct. Try `http://` not `https://`. |
| **Page loads but buttons don't work** | Check the Serial Monitor for error messages. Make sure `server.handleClient()` is in your `loop()`. |
| **IP address keeps changing** | Your router assigns dynamic IPs. You can set a static IP in your router's settings (DHCP reservation) using the board's MAC address. |
| **Page loads very slowly** | The ESP32 is building the HTML string in memory. If the string is too large, it can slow down. Keep pages simple. |
| **Wi-Fi disconnects after a while** | Add a reconnection check in `loop()`: `if (WiFi.status() != WL_CONNECTED) { connectToWiFi(); }` |

---

## What's Next

So far we've only used outputs (driving the LED). The next lesson introduces **inputs** — reading a physical button and using hardware interrupts to respond instantly to button presses.

[Next: Lesson 5 - Button & Interrupts](../05-button-and-interrupts/)
