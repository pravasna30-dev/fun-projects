/*
 * Lesson 8: OLED Display (SSD1306)
 * Board: Seeed Studio XIAO ESP32-S3 Sense
 *
 * Demonstrates I2C communication with a 128x64 SSD1306 OLED display.
 * The sketch runs through several demos:
 *   1. I2C bus scan to detect connected devices
 *   2. Text display with different sizes
 *   3. Shape drawing (rectangles, circles, lines, triangles)
 *   4. Bouncing ball animation
 *   5. Live dashboard showing uptime and free memory
 *
 * Required Libraries (install via Library Manager):
 *   - "Adafruit SSD1306" by Adafruit
 *   - "Adafruit GFX Library" by Adafruit
 *   - "Adafruit BusIO" by Adafruit (auto-installed as dependency)
 *
 * Wiring:
 *   OLED VCC -> 3V3
 *   OLED GND -> GND
 *   OLED SCL -> D5
 *   OLED SDA -> D4
 */

// ---------------------------------------------------------------------------
// Library Includes
// ---------------------------------------------------------------------------
#include <Wire.h>               // Arduino I2C library
#include <Adafruit_GFX.h>       // Graphics primitives (text, shapes, bitmaps)
#include <Adafruit_SSD1306.h>   // SSD1306 OLED hardware driver

// ---------------------------------------------------------------------------
// Display Configuration
// ---------------------------------------------------------------------------
#define SCREEN_WIDTH  128        // OLED display width in pixels
#define SCREEN_HEIGHT 64         // OLED display height in pixels
#define OLED_RESET    -1         // Reset pin (-1 = share Arduino reset pin)
#define SCREEN_ADDRESS 0x3C      // I2C address (0x3C for most modules)

// Create the display object.
// Parameters: width, height, I2C bus reference, reset pin.
// The display object maintains an internal framebuffer (1024 bytes)
// that all drawing functions write to. Call display() to push it out.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------------------------------------------------------------------
// Animation state for the bouncing ball
// ---------------------------------------------------------------------------
int ballX = 10, ballY = 10;     // Ball position
int ballDX = 2, ballDY = 1;     // Ball velocity (pixels per frame)
const int BALL_RADIUS = 4;      // Ball size

// ---------------------------------------------------------------------------
// Dashboard state
// ---------------------------------------------------------------------------
unsigned long dashboardStart = 0;  // When the dashboard phase started

// ---------------------------------------------------------------------------
// Function: scanI2C
// Scans all 127 possible I2C addresses and prints found devices.
// This is incredibly useful for debugging wiring issues.
// ---------------------------------------------------------------------------
void scanI2C() {
  Serial.println("Scanning I2C bus...");
  int deviceCount = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  Found device at address 0x%02X", addr);
      if (addr == 0x3C || addr == 0x3D) {
        Serial.print(" (likely SSD1306 OLED)");
      }
      Serial.println();
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("  No I2C devices found! Check wiring.");
  } else {
    Serial.printf("  Found %d device(s).\n", deviceCount);
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
// Function: demoText
// Shows text at various sizes and positions to demonstrate the GFX library.
// ---------------------------------------------------------------------------
void demoText() {
  // --- Screen 1: Hello message ---
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Size 1 = 6x8 pixels per character (the default monospace font)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Hello, XIAO ESP32-S3!");

  // Size 2 = 12x16 pixels per character (2x scaled)
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println("OLED");

  // Size 1 again for details
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("128x64 SSD1306");
  display.setCursor(0, 52);
  display.printf("I2C addr: 0x%02X", SCREEN_ADDRESS);

  // Push the framebuffer to the display
  display.display();
  delay(3000);

  // --- Screen 2: Font information ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Text Sizes:");
  display.println();

  display.setTextSize(1);
  display.println("Size 1: 21 chars/line");

  display.setTextSize(2);
  display.println("Size 2");

  display.setTextSize(3);
  display.println("Sz 3");

  display.display();
  delay(3000);
}

// ---------------------------------------------------------------------------
// Function: demoShapes
// Draws various geometric shapes to show the GFX drawing primitives.
// ---------------------------------------------------------------------------
void demoShapes() {
  // --- Screen 1: Rectangles ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Rectangles:");

  // Outline rectangle
  display.drawRect(0, 12, 40, 25, SSD1306_WHITE);

  // Filled rectangle
  display.fillRect(50, 12, 40, 25, SSD1306_WHITE);

  // Rounded rectangle
  display.drawRoundRect(0, 42, 40, 20, 5, SSD1306_WHITE);

  // Filled rounded rectangle
  display.fillRoundRect(50, 42, 40, 20, 5, SSD1306_WHITE);

  display.display();
  delay(2500);

  // --- Screen 2: Circles ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Circles:");

  // Draw circles of increasing size
  for (int r = 5; r <= 25; r += 5) {
    display.drawCircle(64, 38, r, SSD1306_WHITE);
  }

  // One filled circle
  display.fillCircle(110, 38, 12, SSD1306_WHITE);

  display.display();
  delay(2500);

  // --- Screen 3: Lines and triangles ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Lines & Triangles:");

  // Fan of lines from the bottom-left corner
  for (int x = 0; x < 128; x += 10) {
    display.drawLine(0, 63, x, 12, SSD1306_WHITE);
  }

  // Triangle
  display.drawTriangle(90, 15, 70, 55, 110, 55, SSD1306_WHITE);
  display.fillTriangle(95, 20, 80, 50, 110, 50, SSD1306_WHITE);

  display.display();
  delay(2500);
}

// ---------------------------------------------------------------------------
// Function: demoAnimation
// Runs a bouncing ball animation for a few seconds to show real-time
// rendering capability.
// ---------------------------------------------------------------------------
void demoAnimation() {
  display.setTextSize(1);

  unsigned long startTime = millis();
  int frameCount = 0;

  // Run the animation for 5 seconds
  while (millis() - startTime < 5000) {
    display.clearDisplay();

    // Draw a border
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

    // Draw the ball
    display.fillCircle(ballX, ballY, BALL_RADIUS, SSD1306_WHITE);

    // Update ball position
    ballX += ballDX;
    ballY += ballDY;

    // Bounce off walls (accounting for the ball's radius and border)
    if (ballX <= BALL_RADIUS + 1 || ballX >= SCREEN_WIDTH - BALL_RADIUS - 1) {
      ballDX = -ballDX;
    }
    if (ballY <= BALL_RADIUS + 1 || ballY >= SCREEN_HEIGHT - BALL_RADIUS - 1) {
      ballDY = -ballDY;
    }

    // Show FPS counter
    frameCount++;
    float fps = frameCount / ((millis() - startTime) / 1000.0);
    display.setCursor(4, 4);
    display.printf("FPS: %.0f", fps);

    display.display();

    // Small delay to control animation speed
    delay(16);  // ~60 FPS target
  }
}

// ---------------------------------------------------------------------------
// Function: drawDashboard
// Draws a real-time information dashboard that updates continuously.
// Shows uptime, free memory, and a progress bar.
// ---------------------------------------------------------------------------
void drawDashboard() {
  display.clearDisplay();

  // ---- Title bar ----
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);  // Black text on white background
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.println("XIAO Dashboard");
  display.setTextColor(SSD1306_WHITE);  // Reset to white text

  // ---- Uptime ----
  unsigned long totalSec = millis() / 1000;
  unsigned long hrs  = totalSec / 3600;
  unsigned long mins = (totalSec % 3600) / 60;
  unsigned long secs = totalSec % 60;

  display.setCursor(0, 16);
  display.printf("Uptime: %02lu:%02lu:%02lu", hrs, mins, secs);

  // ---- Free heap memory ----
  uint32_t freeHeap = ESP.getFreeHeap();
  display.setCursor(0, 28);
  display.printf("Free RAM: %lu B", freeHeap);

  // ---- Free PSRAM (if available) ----
  if (ESP.getFreePsram() > 0) {
    display.setCursor(0, 38);
    display.printf("PSRAM: %lu KB", ESP.getFreePsram() / 1024);
  }

  // ---- Progress bar (60-second cycle) ----
  unsigned long elapsed = (millis() - dashboardStart) % 60000;  // 0-59999
  int barWidth = map(elapsed, 0, 60000, 0, 120);

  display.setCursor(0, 50);
  display.print("60s:");
  display.drawRect(28, 50, 96, 10, SSD1306_WHITE);       // Outline
  display.fillRect(30, 52, min(barWidth, 92), 6, SSD1306_WHITE);  // Fill

  // ---- Bouncing dot in the corner (shows it's alive) ----
  int dotY = 38 + (int)(6.0 * sin(millis() / 200.0));
  display.fillCircle(120, dotY, 2, SSD1306_WHITE);

  // Push framebuffer to the OLED
  display.display();
}

// ---------------------------------------------------------------------------
// setup() - Runs once at startup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==========================================");
  Serial.println("  Lesson 8: OLED Display (SSD1306)");
  Serial.println("  XIAO ESP32-S3 Sense");
  Serial.println("==========================================");
  Serial.println();

  // Initialize I2C bus
  // On XIAO ESP32-S3: SDA = D4 (GPIO 6), SCL = D5 (GPIO 7)
  Wire.begin();

  // Scan the I2C bus for connected devices
  scanI2C();

  // Initialize the OLED display
  // SSD1306_SWITCHCAPVCC tells the driver to generate the display voltage
  // internally from the 3.3V supply using a charge pump circuit.
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[ERROR] SSD1306 initialization failed!");
    Serial.println("Check wiring and I2C address.");
    // Halt — no point continuing without a display
    while (true) {
      delay(1000);
    }
  }

  Serial.println("OLED initialized successfully.");
  Serial.printf("Display: %dx%d pixels | Address: 0x%02X\n",
                SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS);
  Serial.println();

  // Clear any initial display buffer content
  display.clearDisplay();
  display.display();
  delay(500);

  // --- Run through the demos ---
  Serial.println("Running text demo...");
  demoText();

  Serial.println("Running shapes demo...");
  demoShapes();

  Serial.println("Running animation demo...");
  demoAnimation();

  Serial.println("Entering dashboard mode...");
  Serial.println("(Dashboard will update continuously)");
  Serial.println();

  dashboardStart = millis();
}

// ---------------------------------------------------------------------------
// loop() - Runs repeatedly after setup()
// ---------------------------------------------------------------------------
void loop() {
  // Continuously update the dashboard
  drawDashboard();

  // Small delay to avoid hammering the I2C bus
  // At ~10 FPS the dashboard is responsive and easy to read
  delay(100);
}
