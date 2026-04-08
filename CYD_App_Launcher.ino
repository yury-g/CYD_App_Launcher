/*
 * CYD_App_Launcher.ino
 * Student Version - 5 PulseSensor applications for ESP32 CYD (Cheap Yellow Display)
 * 
 * Hardware: ESP32-2432S028R (CYD)
 * Display: ILI9341 320x240 TFT with XPT2046 touch
 * Sensor: PulseSensor on GPIO 36
 * RGB LED: Red=4, Green=16, Blue=17 (onboard CYD LED)
 * 
 * Apps:
 * 0: Heartbeat - scrolling waveform with auto-scaling, BPM, RGB LED flash
 * 1: Breathing - sine wave circle, 8s inhale/exhale cycle
 * 2: Relaxation - large BPM in color circle, red to teal gradient
 * 3: HRV - Poincare plot with RMSSD/SDNN/IBI/BPM metrics
 * 4: BreathFFT - FFT on IBI data to detect breathing rate
 */

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>

// Pin definitions
#define TOUCH_CS 33
#define PULSE_PIN 36
#define BACKLIGHT 21
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17

// Touch calibration (raw values from experimentation)
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3800

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Waveform drawing constants
#define WAVEFORM_TOP 40
#define WAVEFORM_HEIGHT 140
#define WAVEFORM_THICKNESS 4
#define NO_BEAT_TIMEOUT 3000  // ms without beat = no finger detected

// Color palette (student-friendly, not branded)
#define COLOR_BG 0x0000        // Black
#define COLOR_TEXT 0xFFFF      // White
#define COLOR_GRID 0x2104      // Dark grey
#define COLOR_WAVE 0x07FF      // Cyan
#define COLOR_BEAT 0xF800      // Red
#define COLOR_INHALE 0x07E0    // Green
#define COLOR_EXHALE 0x001F    // Blue

// Global objects
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS);
PulseSensorPlayground pulseSensor;

// Global state
int currentApp = 0;
int currentBPM = 0;
int currentIBI = 0;
int currentSignal = 512;
unsigned long lastBeatTime = 0;
bool fingerPresent = false;

// Waveform auto-scaling
int minSignal = 2048;
int maxSignal = 2048;

// RGB LED brightness
int ledBrightness = 0;
#define LED_FADE_SPEED 15

// Forward declarations for all apps
void runHeartbeat();
void runBreathing();
void runRelaxation();
void runHRV();
void runBreathFFT();

// Helper function declarations
void setupLED();
void setLED(int brightness);
void updateLED();
void updateMinMax();
void drawHeart(int x, int y, uint16_t color);
void drawBPM(int bpm);
void drawIBI(int ibi);
TS_Point getTouchPoint();

void setup() {
  Serial.begin(115200);
  
  // Initialize TFT
  tft.init();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(COLOR_BG);
  
  // Initialize touch
  touch.begin();
  touch.setRotation(1);
  
  // Initialize backlight
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
  
  // Initialize RGB LED
  setupLED();
  
  // Initialize PulseSensor
  pulseSensor.analogInput(PULSE_PIN);
  pulseSensor.setThreshold(550);
  if (!pulseSensor.begin()) {
    Serial.println("PulseSensor initialization failed!");
  }
  
  // Draw initial menu
  drawMenu();
}

void loop() {
  // Check for touch to switch apps
  if (touch.touched()) {
    TS_Point p = getTouchPoint();
    if (p.y < 40) {  // Top menu bar touched
      int appIndex = p.x / 64;  // 5 apps, each 64px wide
      if (appIndex != currentApp && appIndex < 5) {
        currentApp = appIndex;
        tft.fillScreen(COLOR_BG);
        drawMenu();
      }
    }
  }
  
  // Read sensor
  currentSignal = pulseSensor.getLatestSample();
  
  // Check for beat (one-shot trigger)
  if (pulseSensor.sawStartOfBeat()) {
    currentBPM = pulseSensor.getBeatsPerMinute();
    currentIBI = pulseSensor.getInterBeatIntervalMs();
    lastBeatTime = millis();
    fingerPresent = true;
    ledBrightness = 255;  // Flash LED on beat
  }
  
  // No-finger timeout: 3 seconds without beat = reset display
  if (fingerPresent && (millis() - lastBeatTime > NO_BEAT_TIMEOUT)) {
    fingerPresent = false;
    currentBPM = 0;
    currentIBI = 0;
  }
  
  // Update LED fade
  updateLED();
  
  // Run current app
  switch (currentApp) {
    case 0: runHeartbeat(); break;
    case 1: runBreathing(); break;
    case 2: runRelaxation(); break;
    case 3: runHRV(); break;
    case 4: runBreathFFT(); break;
  }
}

// ===== RGB LED FUNCTIONS (copied from PulseSensor_CYD) =====

void setupLED() {
  // ESP32 Arduino Core 3.x API: ledcAttach(pin, freq, resolution)
  ledcAttach(LED_RED_PIN, 5000, 8);    // 5kHz PWM, 8-bit resolution
  ledcAttach(LED_GREEN_PIN, 5000, 8);
  ledcAttach(LED_BLUE_PIN, 5000, 8);
  setLED(0);
}

void setLED(int brightness) {
  // CYD RGB LED is active-low (0 = full on, 255 = off)
  int pwmValue = 255 - brightness;
  ledcWrite(LED_RED_PIN, pwmValue);
  ledcWrite(LED_GREEN_PIN, 255);  // Green off
  ledcWrite(LED_BLUE_PIN, 255);   // Blue off
}

void updateLED() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10) {
    lastUpdate = millis();
    if (ledBrightness > 0) {
      ledBrightness -= LED_FADE_SPEED;
      if (ledBrightness < 0) ledBrightness = 0;
      setLED(ledBrightness);
    }
  }
}

// ===== WAVEFORM AUTO-SCALING (copied from PulseSensor_CYD) =====

void updateMinMax() {
  // Decay min/max toward 2048 (ADC midpoint) to prevent stuck scaling
  static unsigned long lastDecay = 0;
  if (millis() - lastDecay > 100) {
    lastDecay = millis();
    minSignal = min(minSignal + 5, 2048);
    maxSignal = max(maxSignal - 5, 2048);
  }
  
  // Track actual signal range
  minSignal = min(minSignal, currentSignal);
  maxSignal = max(maxSignal, currentSignal);
}

// ===== HELPER FUNCTIONS =====

void drawHeart(int x, int y, uint16_t color) {
  // Two circles + triangle = heart shape
  tft.fillCircle(x - 12, y - 8, 14, color);
  tft.fillCircle(x + 12, y - 8, 14, color);
  tft.fillTriangle(x - 26, y - 2, x + 26, y - 2, x, y + 28, color);
}

void drawBPM(int bpm) {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(10, 200);
  tft.printf("BPM: %3d", bpm);
}

void drawIBI(int ibi) {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(160, 200);
  tft.printf("IBI: %4d", ibi);
}

TS_Point getTouchPoint() {
  TS_Point p = touch.getPoint();
  // Map raw touch coordinates to screen coordinates
  int x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_WIDTH);
  int y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_HEIGHT);
  x = constrain(x, 0, SCREEN_WIDTH - 1);
  y = constrain(y, 0, SCREEN_HEIGHT - 1);
  return TS_Point(x, y, p.z);
}

void drawMenu() {
  // Top menu bar with 5 app buttons
  const char* labels[] = {"BEAT", "BREATH", "RELAX", "HRV", "FFT"};
  for (int i = 0; i < 5; i++) {
    uint16_t color = (i == currentApp) ? COLOR_BEAT : COLOR_GRID;
    tft.fillRect(i * 64, 0, 64, 30, color);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(i * 64 + 10, 12);
    tft.print(labels[i]);
  }
}

// ===== APP 0: HEARTBEAT (scrolling waveform with auto-scaling) =====

void runHeartbeat() {
  static int x = 0;
  static int lastY = WAVEFORM_TOP + WAVEFORM_HEIGHT / 2;
  static unsigned long lastDraw = 0;
  
  // Update auto-scaling
  updateMinMax();
  
  // Draw waveform at ~60 FPS
  if (millis() - lastDraw > 16) {
    lastDraw = millis();
    
    // Map signal to screen Y coordinate
    int y = map(currentSignal, minSignal, maxSignal, WAVEFORM_TOP + WAVEFORM_HEIGHT - 1, WAVEFORM_TOP + 1);
    y = constrain(y, WAVEFORM_TOP + 1, WAVEFORM_TOP + WAVEFORM_HEIGHT - 1);
    
    // Draw thick waveform line (4px vertical thickness)
    for (int dy = -WAVEFORM_THICKNESS/2; dy < WAVEFORM_THICKNESS/2; dy++) {
      int drawY = y + dy;
      if (drawY >= WAVEFORM_TOP && drawY < WAVEFORM_TOP + WAVEFORM_HEIGHT) {
        tft.drawPixel(x, drawY, COLOR_WAVE);
      }
    }
    
    // Draw connecting line from previous point
    tft.drawLine(x - 1, lastY, x, y, COLOR_WAVE);
    lastY = y;
    
    // Draw cursor line 4 pixels ahead (visual "writing cursor")
    int cursorX = (x + 4) % SCREEN_WIDTH;
    tft.drawFastVLine(cursorX, WAVEFORM_TOP + 1, WAVEFORM_HEIGHT - 2, COLOR_GRID);
    
    // Advance X position
    x++;
    if (x >= SCREEN_WIDTH) {
      x = 0;
      lastY = WAVEFORM_TOP + WAVEFORM_HEIGHT / 2;
    }
  }
  
  // Update BPM/IBI display
  static int lastDisplayBPM = -1;
  static int lastDisplayIBI = -1;
  if (currentBPM != lastDisplayBPM) {
    drawBPM(currentBPM);
    lastDisplayBPM = currentBPM;
  }
  if (currentIBI != lastDisplayIBI) {
    drawIBI(currentIBI);
    lastDisplayIBI = currentIBI;
  }
  
  // Draw beat indicator (heart icon flashes on beat)
  static bool lastBeat = false;
  bool beatNow = (ledBrightness > 200);
  if (beatNow != lastBeat) {
    uint16_t color = beatNow ? COLOR_BEAT : COLOR_BG;
    drawHeart(280, 210, color);
    lastBeat = beatNow;
  }
}

// ===== APP 1: BREATHING (sine wave circle, 8s inhale/exhale) =====

void runBreathing() {
  static unsigned long lastUpdate = 0;
  static float phase = 0.0;
  
  if (millis() - lastUpdate > 50) {  // 20 FPS
    lastUpdate = millis();
    
    // 8-second breathing cycle (4s inhale, 4s exhale)
    phase += TWO_PI / (8000.0 / 50.0);
    if (phase > TWO_PI) phase -= TWO_PI;
    
    // Calculate circle radius (50 to 100 pixels)
    int radius = 50 + 50 * (0.5 + 0.5 * sin(phase));
    
    // Draw circle
    tft.fillScreen(COLOR_BG);
    drawMenu();
    
    uint16_t color = (sin(phase) > 0) ? COLOR_INHALE : COLOR_EXHALE;
    tft.fillCircle(SCREEN_WIDTH / 2, 140, radius, color);
    
    // Draw instruction text
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(80, 200);
    tft.print(sin(phase) > 0 ? "INHALE " : "EXHALE");
  }
}

// ===== APP 2: RELAXATION (large BPM in color circle, red to teal) =====

void runRelaxation() {
  static int lastBPM = -1;
  
  if (currentBPM != lastBPM) {
    tft.fillScreen(COLOR_BG);
    drawMenu();
    
    // Map BPM to color (red = high, teal = low)
    int bpm = constrain(currentBPM, 40, 120);
    int hue = map(bpm, 120, 40, 0, 180);  // Red (0) to Teal (180)
    
    // Simple HSV to RGB conversion for hue only
    uint16_t color;
    if (hue < 60) {
      color = tft.color565(255, hue * 4, 0);
    } else if (hue < 120) {
      color = tft.color565(255 - (hue - 60) * 4, 255, 0);
    } else {
      color = tft.color565(0, 255, (hue - 120) * 4);
    }
    
    // Draw large circle
    tft.fillCircle(SCREEN_WIDTH / 2, 140, 80, color);
    
    // Draw BPM text
    tft.setTextSize(4);
    tft.setTextColor(COLOR_BG, color);
    char buf[8];
    sprintf(buf, "%3d", currentBPM);
    int textWidth = strlen(buf) * 24;
    tft.setCursor((SCREEN_WIDTH - textWidth) / 2, 120);
    tft.print(buf);
    
    lastBPM = currentBPM;
  }
}

// ===== APP 3: HRV (Poincare plot with metrics) =====

#define MAX_HRV_POINTS 50
int hrvIBIs[MAX_HRV_POINTS];
int hrvIndex = 0;
int hrvCount = 0;

void runHRV() {
  static unsigned long lastUpdate = 0;
  
  // Add new IBI to buffer on each beat
  if (pulseSensor.sawStartOfBeat() && currentIBI > 0) {
    hrvIBIs[hrvIndex] = currentIBI;
    hrvIndex = (hrvIndex + 1) % MAX_HRV_POINTS;
    if (hrvCount < MAX_HRV_POINTS) hrvCount++;
  }
  
  // Redraw every 500ms
  if (millis() - lastUpdate > 500 && hrvCount > 5) {
    lastUpdate = millis();
    
    tft.fillScreen(COLOR_BG);
    drawMenu();
    
    // Calculate HRV metrics
    float sumIBI = 0, sumSqDiff = 0;
    int validPoints = 0;
    
    for (int i = 1; i < hrvCount; i++) {
      sumIBI += hrvIBIs[i];
      float diff = hrvIBIs[i] - hrvIBIs[i-1];
      sumSqDiff += diff * diff;
      validPoints++;
    }
    
    float avgIBI = sumIBI / validPoints;
    float rmssd = sqrt(sumSqDiff / validPoints);
    
    // Calculate SDNN (standard deviation)
    float sumSqDeviation = 0;
    for (int i = 0; i < hrvCount; i++) {
      float dev = hrvIBIs[i] - avgIBI;
      sumSqDeviation += dev * dev;
    }
    float sdnn = sqrt(sumSqDeviation / hrvCount);
    
    // Draw Poincare plot (left half of screen)
    tft.drawRect(10, 40, 140, 140, COLOR_GRID);
    for (int i = 1; i < hrvCount; i++) {
      int x = map(hrvIBIs[i-1], 400, 1200, 10, 150);
      int y = map(hrvIBIs[i], 400, 1200, 180, 40);
      x = constrain(x, 10, 150);
      y = constrain(y, 40, 180);
      tft.drawPixel(x, y, COLOR_WAVE);
    }
    
    // Draw metrics (right half of screen)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(170, 60);
    tft.printf("RMSSD: %.1f", rmssd);
    tft.setCursor(170, 80);
    tft.printf("SDNN: %.1f", sdnn);
    tft.setCursor(170, 100);
    tft.printf("IBI: %d", currentIBI);
    tft.setCursor(170, 120);
    tft.printf("BPM: %d", currentBPM);
    tft.setCursor(170, 140);
    tft.printf("N: %d", hrvCount);
  }
}

// ===== APP 4: BREATH FFT (FFT on IBI to detect breathing rate) =====

#define FFT_SIZE 32
float fftInput[FFT_SIZE];
float fftMagnitude[FFT_SIZE / 2];
int fftIndex = 0;

void runBreathFFT() {
  static unsigned long lastUpdate = 0;
  
  // Add new IBI to FFT buffer on each beat
  if (pulseSensor.sawStartOfBeat() && currentIBI > 0) {
    fftInput[fftIndex] = currentIBI;
    fftIndex = (fftIndex + 1) % FFT_SIZE;
  }
  
  // Redraw every 1000ms
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    tft.fillScreen(COLOR_BG);
    drawMenu();
    
    // Simple FFT approximation (magnitude spectrum)
    // For a real FFT, you'd use a library like arduinoFFT
    // This is a simplified visualization showing IBI variance
    
    for (int i = 0; i < FFT_SIZE / 2; i++) {
      float sum = 0;
      for (int j = 0; j < FFT_SIZE; j++) {
        float angle = TWO_PI * i * j / FFT_SIZE;
        sum += fftInput[j] * cos(angle);
      }
      fftMagnitude[i] = abs(sum) / FFT_SIZE;
    }
    
    // Find peak frequency (breathing rate)
    int peakBin = 0;
    float peakMag = 0;
    for (int i = 1; i < FFT_SIZE / 2; i++) {  // Skip DC bin
      if (fftMagnitude[i] > peakMag) {
        peakMag = fftMagnitude[i];
        peakBin = i;
      }
    }
    
    // Estimate breathing rate (assuming ~1 beat per second sample rate)
    float breathRate = peakBin * 60.0 / FFT_SIZE;  // breaths per minute
    
    // Draw spectrum bars
    int barWidth = SCREEN_WIDTH / (FFT_SIZE / 2);
    for (int i = 0; i < FFT_SIZE / 2; i++) {
      int barHeight = constrain(fftMagnitude[i] / 2, 0, 120);
      uint16_t color = (i == peakBin) ? COLOR_BEAT : COLOR_WAVE;
      tft.fillRect(i * barWidth, 160 - barHeight, barWidth - 2, barHeight, color);
    }
    
    // Draw breathing rate estimate
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(60, 180);
    tft.printf("Breathing: %.1f/min", breathRate);
  }
}
