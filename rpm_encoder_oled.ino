/*
 * RPM Meter using Rotary Encoder + 0.96" OLED Display
 * Target: ESP32 (any 38-pin or 30-pin variant)
 *
 * Wiring:
 *   Rotary Encoder
 *     CLK (A)  --> GPIO 34
 *     DT  (B)  --> GPIO 35
 *     VCC      --> 3.3V
 *     GND      --> GND
 *
 *   OLED SSD1306 (0.96", I2C)
 *     SDA      --> GPIO 21
 *     SCL      --> GPIO 22
 *     VCC      --> 3.3V
 *     GND      --> GND
 *
 * Libraries required (install via Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *   - Wire (built-in)
 *
 * Formula:
 *   RPM = (pulse_count_in_window / PPR) * (60000 / sample_ms)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Pin Definitions ──────────────────────────────────────────────────────────
#define ENCODER_CLK   34        // Channel A (interrupt-capable on ESP32)
#define ENCODER_DT    35        // Channel B (used for direction detection)

// ─── OLED Configuration ───────────────────────────────────────────────────────
#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define OLED_RESET    -1        // No reset pin; share Arduino reset
#define OLED_ADDRESS  0x3C      // Default I2C address (try 0x3D if nothing shows)

// ─── Encoder / RPM Settings ───────────────────────────────────────────────────
#define PPR           20        // Pulses Per Revolution of your encoder disc
                                // Common values: 20, 100, 200, 400, 600
                                // Adjust this to match your physical encoder

#define SAMPLE_MS     200       // Sampling window in milliseconds
                                // Lower  = faster updates, more noise
                                // Higher = smoother reading, slower response
                                // 100-500ms works well in most use cases

#define RPM_MAX       9999      // Clamp display ceiling

// ─── Globals ─────────────────────────────────────────────────────────────────
volatile long     pulseCount    = 0;   // Incremented inside ISR
volatile bool     dirForward    = true;
unsigned long     lastSample    = 0;
float             rpm           = 0.0;
float             rpmSmoothed   = 0.0;
const float       ALPHA         = 0.3f; // Low-pass filter coefficient (0–1)
                                        // Lower = smoother but slower
long              totalRevs     = 0;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ─── Interrupt Service Routine ────────────────────────────────────────────────
// Called on every RISING edge of Channel A.
// Channel B is sampled at that instant to determine direction.
void IRAM_ATTR encoderISR() {
  pulseCount++;
  dirForward = (digitalRead(ENCODER_DT) == HIGH);
}

// ─── OLED Draw Helpers ────────────────────────────────────────────────────────

// Draws a simple bar gauge at the bottom of the screen.
// val: current RPM,  maxVal: scale maximum
void drawBar(float val, float maxVal) {
  int barX  = 0;
  int barY  = 56;
  int barW  = OLED_WIDTH;
  int barH  = 6;

  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  int fill = (int)((val / maxVal) * (barW - 2));
  fill = constrain(fill, 0, barW - 2);
  if (fill > 0) {
    display.fillRect(barX + 1, barY + 1, fill, barH - 2, SSD1306_WHITE);
  }
}

// Draws the main RPM screen layout.
void drawRPMScreen(float rpmVal, bool forward, long revCount) {
  display.clearDisplay();

  // ── Title bar ──
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("  RPM METER  v1.0");
  display.drawLine(0, 9, OLED_WIDTH - 1, 9, SSD1306_WHITE);

  // ── Large RPM value ──
  display.setTextSize(3);
  int rpmDisplay = (int)constrain(rpmVal, 0, RPM_MAX);
  String rpmStr  = String(rpmDisplay);

  // Right-align the number
  int charWidth  = 18; // approx pixels per char at size 3
  int strPixels  = rpmStr.length() * charWidth;
  int cursorX    = (OLED_WIDTH - strPixels) / 2;
  display.setCursor(cursorX, 16);
  display.print(rpmStr);

  // ── "RPM" unit label ──
  display.setTextSize(1);
  display.setCursor(100, 42);
  display.print("RPM");

  // ── Direction indicator ──
  display.setCursor(0, 42);
  display.print(forward ? "CW " : "CCW");

  // ── Total revolutions (small) ──
  display.setCursor(0, 42);
  // Overwrite direction with rev counter on left
  display.setCursor(36, 42);
  display.print("R:");
  display.print(revCount % 10000); // keep it short

  // ── Bar gauge ──
  drawBar(rpmVal, RPM_MAX);

  display.display();
}

// Splash screen shown once at startup.
void drawSplash() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 8);
  display.print("RPM Meter");
  display.setTextSize(1);
  display.setCursor(8, 32);
  display.print("ESP32 + Encoder");
  display.setCursor(20, 44);
  display.print("SSD1306 OLED");
  display.setCursor(28, 56);
  display.print("PPR: ");
  display.print(PPR);
  display.display();
  delay(2000);
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("RPM Meter starting...");

  // Encoder pins — internal pull-ups, encoder is active-low on most modules
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT,  INPUT_PULLUP);

  // Attach interrupt on rising edge of CLK (Channel A)
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, RISING);

  // Initialise OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("SSD1306 not found. Check wiring and I2C address.");
    while (true) { delay(1000); } // halt
  }
  display.setRotation(0);
  display.cp437(true);

  drawSplash();

  lastSample = millis();
  Serial.println("Ready.");
}

// ─── Main Loop ───────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_MS) {
    // Atomically snapshot and reset the counter
    noInterrupts();
    long count = pulseCount;
    pulseCount  = 0;
    interrupts();

    // Convert to RPM
    // pulses_per_second = count / (SAMPLE_MS / 1000.0)
    // rps = pulses_per_second / PPR
    // rpm = rps * 60
    float rawRPM = ((float)count / PPR) * (60000.0f / SAMPLE_MS);
    rawRPM = constrain(rawRPM, 0.0f, (float)RPM_MAX);

    // Exponential moving average (low-pass smoothing)
    rpmSmoothed = ALPHA * rawRPM + (1.0f - ALPHA) * rpmSmoothed;
    rpm = rpmSmoothed;

    // Accumulate total revolutions
    totalRevs += count / PPR;

    lastSample = now;

    // Update display
    drawRPMScreen(rpm, dirForward, totalRevs);

    // Serial output for Serial Plotter / logging
    Serial.print("RPM: ");
    Serial.print((int)rpm);
    Serial.print("  Dir: ");
    Serial.print(dirForward ? "CW" : "CCW");
    Serial.print("  Revs: ");
    Serial.println(totalRevs);
  }

  // Small yield so the Wi-Fi/BT stack doesn't starve (good practice on ESP32)
  delay(5);
}
