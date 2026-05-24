/*
 * CYD_App_Launcher.ino
 * One-screen PulseSensor dashboard for ESP32 CYD (Cheap Yellow Display).
 *
 * Hardware:
 *   Board:        ESP32-2432S028 (CYD)
 *   Display:      ILI9341 320x240 TFT
 *   PulseSensor:  signal wire on GPIO 35 (found with AnalogPinScanner)
 *   RGB LED:      Red=4, Green=16, Blue=17 (active-low onboard CYD LED)
 *
 * This sketch intentionally stays in one file for Arduino IDE beginners.
 *
 * PulseSensorPlayground functions used, following the library resources:
 *   analogReadResolution(10)  -> match PulseSensorPlayground's 0..1023 math
 *   getLatestSample()         -> live waveform
 *   sawStartOfBeat()          -> one-shot beat event
 *   isInsideBeat()            -> inside/outside the detected beat window
 *   getBeatsPerMinute()       -> BPM readout
 *   getInterBeatIntervalMs()  -> IBI readout
 *   getPulseAmplitude()       -> signal quality helper
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#if __has_include(<XPT2046_Touchscreen_TT.h>)
#include <XPT2046_Touchscreen_TT.h>
#else
#include <XPT2046_Touchscreen.h>
#endif
#include <esp_arduino_version.h>
#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>

// ===== CYD PINS =====

#define PULSE_PIN 35
#define BACKLIGHT 21
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17
#define SPEAKER_PIN 26

#define LED_RED_PWM_CH 0
#define LED_GREEN_PWM_CH 1
#define LED_BLUE_PWM_CH 2
#define SPEAKER_PWM_CH 3

#define TOUCH_IRQ 36
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
#define TOUCH_SCLK 25
#define TOUCH_CS 33

// ===== PULSESENSOR SETTINGS =====

#define PULSE_THRESHOLD 550
#define NO_BEAT_TIMEOUT 3000
#define MIN_QUALIFIED_BPM 40
#define MAX_QUALIFIED_BPM 180
#define MIN_QUALIFIED_IBI 333
#define MAX_QUALIFIED_IBI 1500
#define MIN_QUALIFIED_AMPLITUDE 20
#define SIGNAL_QUALITY_STEPS 12
#define LOCK_QUALITY_STEPS 10
#define LOCK_QUALIFIED_BEATS 4
#define REARM_SIGNAL_RANGE 120
#define REARM_NO_BEAT_MS 2200
#define REARM_COOLDOWN_MS 3500
#define SIGNAL_COACH_FLAT_RANGE 90
#define SIGNAL_COACH_FLAT_AMPLITUDE 12
#define SIGNAL_COACH_STEADY_AMPLITUDE MIN_QUALIFIED_AMPLITUDE
#define AMPLITUDE_METER_MAX 120

// ===== BEAT TONE SETTINGS =====

#define SPEAKER_BITS 10
#define BEAT_CHIME_STEP_COUNT 4
#define HEART_MIN_SIZE 8
#define HEART_MAX_SIZE 15
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_START 1
#define SCREEN_ROTATION_DEFAULT 1
#define SCREEN_ROTATION_COUNT 4

// ===== APP SHELL =====

#define APP_VERSION "0.3.0-app-shell"
#define APP_FIRMWARE_DATE "2026-05-24"
#define TOOLBAR_BUTTON_WIDTH 44
#define TOOLBAR_BUTTON_HEIGHT 28
#define APP_BUTTON_WIDTH TOOLBAR_BUTTON_WIDTH
#define APP_BUTTON_HEIGHT TOOLBAR_BUTTON_HEIGHT
#define APP_BUTTON_GAP 2
#define SETTINGS_TEXT_SIZE 2
#define SETTINGS_ROW_H 40
#define SETTINGS_ROW_COUNT 9
#define SETTINGS_SCROLL_BUTTON_W TOOLBAR_BUTTON_WIDTH
#define SETTINGS_SCROLL_BUTTON_H TOOLBAR_BUTTON_HEIGHT
#define PLACEHOLDER_STEP_MS 35
#define CONTROL_TOUCH_PAD 8

// ===== TOUCH CALIBRATION =====

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800

// ===== SCREEN LAYOUT =====

#define LANDSCAPE_WIDTH 320
#define LANDSCAPE_HEIGHT 240
#define CONTROL_BUTTON_SIZE 22

// ===== COLORS (RGB565) =====

#define COLOR_BG 0x0000
#define COLOR_PANEL 0x0841
#define COLOR_PANEL_DARK 0x0400
#define COLOR_GRID 0x39E7
#define COLOR_GRID_SOFT 0x2945
#define COLOR_TEXT 0xFFFF
#define COLOR_MUTED COLOR_TEXT
#define COLOR_CYAN 0x07FF
#define COLOR_CYAN_DARK 0x0452
#define COLOR_TEAL 0x05F3
#define COLOR_ACQUIRE_BLUE 0x3DFF
#define COLOR_LOCK_GREEN 0x07E0
#define COLOR_RED 0xF800
#define COLOR_RED_DARK 0x6000
#define COLOR_SCREEN_BEAT COLOR_CYAN
#define COLOR_HIGH_VIS_YELLOW 0xFFF2
#define COLOR_AMBER COLOR_HIGH_VIS_YELLOW
#define COLOR_SIGNAL_YELLOW COLOR_HIGH_VIS_YELLOW

enum SignalCoachState {
  COACH_SIGNAL_SEARCH,
  COACH_TOO_FLAT,
  COACH_HOLD_STEADY,
  COACH_GOOD_WAVE,
  COACH_LOCKING,
  COACH_QUALIFIED
};

enum AppId {
  APP_PULSE,
  APP_PLACEHOLDER_1,
  APP_PLACEHOLDER_2,
  APP_SETTINGS,
  APP_COUNT
};

// ===== GLOBAL OBJECTS =====

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
PulseSensorPlayground pulseSensor;

const uint16_t BEAT_CHIME_FREQUENCIES[BEAT_CHIME_STEP_COUNT] = {262, 392, 523, 659};
const uint8_t BEAT_CHIME_DUTIES[BEAT_CHIME_STEP_COUNT] = {56, 42, 30, 18};
const uint16_t BEAT_CHIME_DURATIONS_MS[BEAT_CHIME_STEP_COUNT] = {58, 66, 82, 118};
const uint8_t SCREEN_ROTATIONS[SCREEN_ROTATION_COUNT] = {1, 0, 3, 2};

const uint16_t SIGNAL_HARMONY_FREQUENCIES[] = {523, 659, 784, 988, 1175};
const uint8_t SIGNAL_HARMONY_DUTIES[] = {48, 40, 30};
const uint16_t SIGNAL_HARMONY_DURATIONS_MS[] = {72, 84, 128};

// ===== LIVE SENSOR STATE =====

int currentSignal = 512;
int displayBPM = 0;
int displayIBI = 0;
int pulseAmplitude = 0;
int minSignal = 512;
int maxSignal = 512;

unsigned long lastBeatTime = 0;
unsigned long lastQualifiedBeatTime = 0;
unsigned long lastGraphDraw = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastDetectorRearmTime = 0;
unsigned long lastControlTouchTime = 0;
unsigned long lastSignalHarmonyTime = 0;

bool lockedSignal = false;
bool previousLockedSignal = false;
bool pulseSensorReady = false;
bool insideBeatWindow = false;
int signalQuality = 0;
int qualifiedBeatStreak = 0;
int clippedSampleScore = 0;
int rearmCount = 0;

bool dashboardDrawn = false;
int previousDisplayBPM = -1;
int previousDisplayIBI = -1;
int previousPulseAmplitude = -1;
int previousSignalQuality = -1;
int previousRearmCount = -1;
int previousSignalCoachState = -1;
bool previousDashboardLockedSignal = false;

// ===== BEAT TONE STATE =====

bool beatTonePlaying = false;
uint8_t beatChimeStep = 0;
unsigned long beatChimeNextStepTime = 0;
bool beatHeartNeedsRedraw = true;
uint8_t speakerVolume = VOLUME_START;
uint8_t screenRotation = SCREEN_ROTATION_DEFAULT;
uint8_t screenRotationIndex = 0;

bool signalHarmonyPlaying = false;
uint8_t signalHarmonyStep = 0;
uint8_t signalHarmonyBaseNote = 0;
unsigned long signalHarmonyNextStepTime = 0;

// ===== GRAPH STATE =====

int graphX = 0;
int lastGraphY = 104;

// ===== LAYOUT STATE =====

bool portraitLayout = false;
int screenWidth = LANDSCAPE_WIDTH;
int screenHeight = LANDSCAPE_HEIGHT;
int headerHeight = 42;
int heartCenterX = 160;
int heartCenterY = 22;

int graphLeft = 8;
int graphTop = 48;
int graphWidth = 304;
int graphHeight = 112;

int bpmPanelX = 8;
int bpmPanelY = 170;
int bpmPanelW = 102;
int bpmPanelH = 62;
int ibiPanelX = 118;
int ibiPanelY = 170;
int ibiPanelW = 102;
int ibiPanelH = 62;
int signalPanelX = 228;
int signalPanelY = 170;
int signalPanelW = 84;
int signalPanelH = 62;

int rotateButtonX = 294;
int rotateButtonY = 9;
int rotateButtonWidth = TOOLBAR_BUTTON_WIDTH;
int rotateButtonHeight = TOOLBAR_BUTTON_HEIGHT;
int volumeLabelX = 186;
int volumeMinusX = 214;
int volumeValueX = 242;
int volumePlusX = 266;
int volumeY = 9;
int volumeButtonSize = CONTROL_BUTTON_SIZE;

// ===== REAR LED FADE STATE =====

struct RearLedColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

const RearLedColor REAR_LED_OFF = {0, 0, 0};
const RearLedColor REAR_LED_HEARTBEAT = {255, 0, 0};
const RearLedColor REAR_LED_LOCKING = {255, 255, 0};

int ledBrightness = 0;
bool ledPulseActive = false;
unsigned long ledPulseStartTime = 0;
int rearLedBrightness = 0;
RearLedColor rearLedPulseColor = REAR_LED_HEARTBEAT;
RearLedColor heartbeatLedColor = REAR_LED_HEARTBEAT;
bool rearLedPulseActive = false;
unsigned long rearLedPulseStartTime = 0;
bool beatLedEnabled = true;
#define LED_UPDATE_MS 10
#define LED_PEAK_HOLD_MS 90
#define LED_FADE_MS 620

// ===== APP SHELL STATE =====

AppId currentApp = APP_PULSE;
bool appNeedsRedraw = true;
int appPrevButtonX = 122;
int appNextButtonX = 146;
int appSettingsButtonX = 170;
int appButtonY = 9;

int settingsVolMinusX = 150;
int settingsVolPlusX = 202;
int settingsRotateX = 150;
int settingsLedX = 150;
int settingsColorRedX = 150;
int settingsColorYellowX = 178;
int settingsColorCyanX = 206;
int settingsScrollUpX = 226;
int settingsScrollDownX = 272;
int settingsScrollButtonW = SETTINGS_SCROLL_BUTTON_W;
int settingsScrollButtonY = 208;
int settingsScrollY = 0;

int placeholderX = 24;
int placeholderY = 90;
int placeholderLastX = -1;
int placeholderLastY = -1;
int placeholderDx = 2;
int placeholderDy = 2;
unsigned long lastPlaceholderMove = 0;

// ===== FORWARD DECLARATIONS =====

void setup();
void loop();
void setupLED();
void setRearLedRaw(bool redOn, bool greenOn, bool blueOn);
void setRearLedColor(RearLedColor color);
void setRearLedPulseBrightness(int brightness);
int ledPulseEnvelopeBrightness(unsigned long age);
void updateLED();
void setupSpeaker();
void setupTouch();
void readTouchControls();
void mapTouchPoint(const TS_Point& point, int16_t* x, int16_t* y);
bool handleAppNavTouch(int16_t x, int16_t y);
bool handleRotateTouch(int16_t x, int16_t y);
bool handleVolumeTouch(int16_t x, int16_t y);
bool handleSettingsTouch(int16_t x, int16_t y);
bool handleSettingsScrollTouch(int16_t x, int16_t y);
int buttonCenterX(int x, int size);
int midpointBetween(int leftCenter, int rightCenter);
int settingsContentTop();
int settingsContentBottom();
int settingsContentHeight();
int settingsMaxScroll();
int settingsRowScreenY(int rowIndex);
void clampSettingsScroll();
void scrollSettingsBy(int delta);
void switchApp(AppId app);
void nextApp();
void previousApp();
void rotateScreen();
void applyScreenRotation();
void configureLayout();
void resetDashboardState();
void resetPlaceholderState();
void cydLedcAttach(uint8_t pin, uint8_t channel, uint32_t frequency, uint8_t resolution);
void cydLedcWrite(uint8_t pin, uint8_t channel, uint32_t duty);
void cydLedcWriteTone(uint8_t pin, uint8_t channel, uint32_t frequency);
uint16_t scaledChimeDuty(uint8_t step);
void startBeatChime();
void updateBeatChime();
uint16_t scaledSignalHarmonyDuty(uint8_t duty);
void startSignalHarmony(int quality);
void updateSignalHarmony();
void stopSignalHarmony();
void triggerRearLedPulse(RearLedColor color);
void triggerBeatEffects();
void setupPulseSensor();
void readPulseSensor();
bool isQualifiedBeat(int bpm, int ibi, int amplitude);
void updateClippingScore();
int signalCoachState();
const char* signalCoachText();
int amplitudeMeterSegments(int amplitude);
void maybeRearmDetector();
void rearmPulseDetector(const char* reason);
void updateSignalRange();
void drawActiveApp();
void drawStaticScreen();
void drawDashboardIfChanged();
void drawHeader();
void drawAppNavControls();
void drawAppButton(int x, int y, const char* label, bool active);
void drawSettingsScreen();
void drawSettingsRow(int rowIndex, int y, const char* label, const char* value);
uint16_t settingsRowBackground(int rowIndex);
void drawSettingsButton(int x, int y, int w, const char* label, bool active);
void drawSettingsSwatch(int x, int y, uint16_t color, bool active);
void drawSettingsScrollControls();
void drawPlaceholderApp(const char* title, const char* message);
void drawPlaceholderText(const char* message, uint16_t color);
void drawRotateControl();
void drawVolumeControl();
void drawGraphFrame();
void drawGraphColumnBackground(int localX);
void drawThresholdMarker(int localX);
int signalToGraphY(int signal);
void drawWaveform();
void drawPanels();
void drawMetricPanel(int x, int y, int w, int h, const char* label, int value, const char* unit, bool valid);
uint16_t metricPanelBackground(const char* label, bool valid);
void drawSignalPanel();
void drawQualitySegments(int x, int y);
void drawAmplitudeMeter(int x, int y, int amplitude);
void drawBeatHeart();
void fillHeartShape(int centerX, int centerY, int size, uint16_t color);
void drawCenteredText(const char* text, int x, int y, int w, int textSize, uint16_t color, uint16_t bg);
uint16_t liveTraceColor();
uint16_t blendRed(int brightness);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("CYD one-screen PulseSensor dashboard");

  setupLED();

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  tft.init();
  applyScreenRotation();
  tft.fillScreen(COLOR_BG);

  setupSpeaker();
  setupTouch();
  setupPulseSensor();
  drawActiveApp();
}

void loop() {
  readPulseSensor();
  readTouchControls();
  updateLED();
  updateBeatChime();
  updateSignalHarmony();

  if (currentApp == APP_PULSE) {
    drawBeatHeart();
    drawWaveform();
    drawDashboardIfChanged();
  } else if (currentApp == APP_PLACEHOLDER_1) {
    drawPlaceholderApp("App 2", "your app here");
  } else if (currentApp == APP_PLACEHOLDER_2) {
    drawPlaceholderApp("App 3", "your app here too");
  } else if (appNeedsRedraw) {
    drawSettingsScreen();
  }

  if (millis() - lastSerialPrint >= 500) {
    lastSerialPrint = millis();
    Serial.printf("signal=%d amp=%d bpm=%d ibi=%d locked=%d quality=%d\n",
                  currentSignal, pulseAmplitude, displayBPM, displayIBI,
                  lockedSignal ? 1 : 0, signalQuality);
  }
}

// ===== HARDWARE SETUP =====

void setupLED() {
  // The CYD RGB LED is active-low. Red and green use PWM for smooth
  // red/yellow pulse fades; blue stays raw-off for the dashboard.
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  setRearLedRaw(false, false, false);
  cydLedcAttach(LED_RED_PIN, LED_RED_PWM_CH, 5000, 8);
  cydLedcAttach(LED_GREEN_PIN, LED_GREEN_PWM_CH, 5000, 8);
  setRearLedColor(REAR_LED_OFF);
}

void setRearLedRaw(bool redOn, bool greenOn, bool blueOn) {
  digitalWrite(LED_RED_PIN, redOn ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, greenOn ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN, blueOn ? LOW : HIGH);
}

void setRearLedColor(RearLedColor color) {
  color.red = constrain(color.red, 0, 255);
  color.green = constrain(color.green, 0, 255);
  color.blue = constrain(color.blue, 0, 255);
  digitalWrite(LED_BLUE_PIN, color.blue > 0 ? LOW : HIGH);
  cydLedcWrite(LED_RED_PIN, LED_RED_PWM_CH, 255 - color.red);
  cydLedcWrite(LED_GREEN_PIN, LED_GREEN_PWM_CH, 255 - color.green);
}

void setRearLedPulseBrightness(int brightness) {
  brightness = constrain(brightness, 0, 255);
  RearLedColor color = {
    (uint8_t)((rearLedPulseColor.red * brightness) / 255),
    (uint8_t)((rearLedPulseColor.green * brightness) / 255),
    (uint8_t)((rearLedPulseColor.blue * brightness) / 255)
  };
  setRearLedColor(color);
}

int ledPulseEnvelopeBrightness(unsigned long age) {
  if (age <= LED_PEAK_HOLD_MS) return 255;

  unsigned long fadeAge = age - LED_PEAK_HOLD_MS;
  if (fadeAge >= LED_FADE_MS) return 0;

  uint32_t progress = (fadeAge * 255UL) / LED_FADE_MS;
  uint32_t smooth = (progress * progress * (765UL - (2UL * progress))) / (255UL * 255UL);
  return 255 - smooth;
}

void updateLED() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  if (now - lastUpdate < LED_UPDATE_MS) return;
  lastUpdate = now;

  if (rearLedPulseActive) {
    unsigned long rearAge = now - rearLedPulseStartTime;
    rearLedBrightness = ledPulseEnvelopeBrightness(rearAge);
    if (rearLedBrightness == 0) {
      rearLedPulseActive = false;
    }
  }

  if (ledPulseActive) {
    unsigned long age = now - ledPulseStartTime;
    ledBrightness = ledPulseEnvelopeBrightness(age);
    if (ledBrightness == 0) {
      ledPulseActive = false;
    }
  }

  setRearLedPulseBrightness(rearLedBrightness);
}

void setupSpeaker() {
  cydLedcAttach(SPEAKER_PIN, SPEAKER_PWM_CH, BEAT_CHIME_FREQUENCIES[0], SPEAKER_BITS);
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
}

void setupTouch() {
  touchSpi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSpi);
  touch.setRotation(1);
}

void readTouchControls() {
  if (!touch.touched()) return;
  if (millis() - lastControlTouchTime < 180) return;

  TS_Point point = touch.getPoint();
  int16_t x;
  int16_t y;
  mapTouchPoint(point, &x, &y);

  if (handleAppNavTouch(x, y) ||
      handleRotateTouch(x, y) ||
      (currentApp == APP_SETTINGS && handleSettingsTouch(x, y))) {
    lastControlTouchTime = millis();
  }
}

void mapTouchPoint(const TS_Point& point, int16_t* x, int16_t* y) {
  int16_t landscapeX = constrain(map(point.x, TOUCH_MIN_X, TOUCH_MAX_X, 1, LANDSCAPE_WIDTH), 0, LANDSCAPE_WIDTH - 1);
  int16_t landscapeY = constrain(map(point.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 1, LANDSCAPE_HEIGHT), 0, LANDSCAPE_HEIGHT - 1);
  int16_t mappedX = landscapeX;
  int16_t mappedY = landscapeY;

  if (screenRotation == 0) {
    mappedX = LANDSCAPE_HEIGHT - 1 - landscapeY;
    mappedY = landscapeX;
  } else if (screenRotation == 2) {
    mappedX = landscapeY;
    mappedY = LANDSCAPE_WIDTH - 1 - landscapeX;
  } else if (screenRotation == 3) {
    mappedX = LANDSCAPE_WIDTH - 1 - landscapeX;
    mappedY = LANDSCAPE_HEIGHT - 1 - landscapeY;
  }

  *x = constrain(mappedX, 0, screenWidth - 1);
  *y = constrain(mappedY, 0, screenHeight - 1);
}

bool handleAppNavTouch(int16_t x, int16_t y) {
  if (y < appButtonY - CONTROL_TOUCH_PAD || y > appButtonY + APP_BUTTON_HEIGHT + CONTROL_TOUCH_PAD) return false;

  int appPrevCenter = buttonCenterX(appPrevButtonX, APP_BUTTON_WIDTH);
  int appNextCenter = buttonCenterX(appNextButtonX, APP_BUTTON_WIDTH);
  int appSettingsCenter = buttonCenterX(appSettingsButtonX, APP_BUTTON_WIDTH);
  int rotateCenter = buttonCenterX(rotateButtonX, rotateButtonWidth);
  int appPrevNextBoundary = midpointBetween(appPrevCenter, appNextCenter);
  int appNextSettingsBoundary = midpointBetween(appNextCenter, appSettingsCenter);
  int appSettingsRotateBoundary = midpointBetween(appSettingsCenter, rotateCenter);

  if (x < appPrevButtonX - CONTROL_TOUCH_PAD || x > appSettingsRotateBoundary) return false;

  if (x <= appPrevNextBoundary) {
    previousApp();
    return true;
  }

  if (x <= appNextSettingsBoundary) {
    nextApp();
    return true;
  }

  switchApp(currentApp == APP_SETTINGS ? APP_PULSE : APP_SETTINGS);
  return true;
}

bool handleRotateTouch(int16_t x, int16_t y) {
  if (y < rotateButtonY - CONTROL_TOUCH_PAD || y > rotateButtonY + rotateButtonHeight + CONTROL_TOUCH_PAD) return false;

  int appSettingsCenter = buttonCenterX(appSettingsButtonX, APP_BUTTON_WIDTH);
  int rotateCenter = buttonCenterX(rotateButtonX, rotateButtonWidth);
  int appSettingsRotateBoundary = midpointBetween(appSettingsCenter, rotateCenter);
  if (x <= appSettingsRotateBoundary || x > rotateButtonX + rotateButtonWidth + CONTROL_TOUCH_PAD) return false;

  rotateScreen();
  return true;
}

int buttonCenterX(int x, int size) {
  return x + size / 2;
}

int midpointBetween(int leftCenter, int rightCenter) {
  return (leftCenter + rightCenter) / 2;
}

bool handleVolumeTouch(int16_t x, int16_t y) {
  if (y < volumeY - CONTROL_TOUCH_PAD || y > volumeY + volumeButtonSize + CONTROL_TOUCH_PAD) return false;

  if (x >= volumeMinusX - CONTROL_TOUCH_PAD && x <= volumeMinusX + volumeButtonSize + CONTROL_TOUCH_PAD) {
    if (speakerVolume > VOLUME_MIN) speakerVolume--;
  } else if (x >= volumePlusX - CONTROL_TOUCH_PAD && x <= volumePlusX + volumeButtonSize + CONTROL_TOUCH_PAD) {
    if (speakerVolume < VOLUME_MAX) speakerVolume++;
  } else {
    return false;
  }

  drawVolumeControl();
  if (beatTonePlaying) {
    cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, scaledChimeDuty(beatChimeStep));
  }
  return true;
}

bool handleSettingsTouch(int16_t x, int16_t y) {
  if (handleSettingsScrollTouch(x, y)) return true;

  int volumeRowY = settingsRowScreenY(0);
  if (volumeRowY >= settingsContentTop() && volumeRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= volumeRowY - CONTROL_TOUCH_PAD && y <= volumeRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD) {
    if (x >= settingsVolMinusX - CONTROL_TOUCH_PAD && x <= settingsVolMinusX + TOOLBAR_BUTTON_WIDTH + CONTROL_TOUCH_PAD) {
      if (speakerVolume > VOLUME_MIN) speakerVolume--;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }
    if (x >= settingsVolPlusX - CONTROL_TOUCH_PAD && x <= settingsVolPlusX + TOOLBAR_BUTTON_WIDTH + CONTROL_TOUCH_PAD) {
      if (speakerVolume < VOLUME_MAX) speakerVolume++;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }
  }

  int rotationRowY = settingsRowScreenY(1);
  if (rotationRowY >= settingsContentTop() && rotationRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= rotationRowY - CONTROL_TOUCH_PAD && y <= rotationRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD &&
      x >= settingsRotateX - CONTROL_TOUCH_PAD && x <= settingsRotateX + 86 + CONTROL_TOUCH_PAD) {
    rotateScreen();
    return true;
  }

  int ledRowY = settingsRowScreenY(4);
  if (ledRowY >= settingsContentTop() && ledRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= ledRowY - CONTROL_TOUCH_PAD && y <= ledRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD &&
      x >= settingsLedX - CONTROL_TOUCH_PAD && x <= settingsLedX + 86 + CONTROL_TOUCH_PAD) {
    beatLedEnabled = !beatLedEnabled;
    if (!beatLedEnabled) setRearLedColor(REAR_LED_OFF);
    appNeedsRedraw = true;
    drawSettingsScreen();
    return true;
  }

  int colorRowY = settingsRowScreenY(5);
  if (colorRowY >= settingsContentTop() && colorRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= colorRowY - CONTROL_TOUCH_PAD && y <= colorRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD) {
    int redCenter = buttonCenterX(settingsColorRedX, 34);
    int yellowCenter = buttonCenterX(settingsColorYellowX, 34);
    int cyanCenter = buttonCenterX(settingsColorCyanX, 34);
    int redYellowBoundary = midpointBetween(redCenter, yellowCenter);
    int yellowCyanBoundary = midpointBetween(yellowCenter, cyanCenter);
    if (x < settingsColorRedX - CONTROL_TOUCH_PAD || x > settingsColorCyanX + 34 + CONTROL_TOUCH_PAD) {
      return false;
    }

    if (x <= redYellowBoundary) {
      heartbeatLedColor = REAR_LED_HEARTBEAT;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }
    if (x <= yellowCyanBoundary) {
      heartbeatLedColor = REAR_LED_LOCKING;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }

    heartbeatLedColor = RearLedColor{0, 255, 255};
    appNeedsRedraw = true;
    drawSettingsScreen();
    return true;
  }

  return false;
}

bool handleSettingsScrollTouch(int16_t x, int16_t y) {
  if (y < settingsScrollButtonY - CONTROL_TOUCH_PAD ||
      y > settingsScrollButtonY + SETTINGS_SCROLL_BUTTON_H + CONTROL_TOUCH_PAD) {
    return false;
  }

  if (x >= settingsScrollUpX - CONTROL_TOUCH_PAD &&
      x <= settingsScrollUpX + settingsScrollButtonW + CONTROL_TOUCH_PAD) {
    scrollSettingsBy(-SETTINGS_ROW_H);
    drawSettingsScreen();
    return true;
  }

  if (x >= settingsScrollDownX - CONTROL_TOUCH_PAD &&
      x <= settingsScrollDownX + settingsScrollButtonW + CONTROL_TOUCH_PAD) {
    scrollSettingsBy(SETTINGS_ROW_H);
    drawSettingsScreen();
    return true;
  }

  return false;
}

int settingsContentTop() {
  return headerHeight + 4;
}

int settingsContentBottom() {
  return settingsScrollButtonY - 3;
}

int settingsContentHeight() {
  return SETTINGS_ROW_COUNT * SETTINGS_ROW_H;
}

int settingsMaxScroll() {
  int viewportHeight = max(1, settingsContentBottom() - settingsContentTop());
  return max(0, settingsContentHeight() - viewportHeight);
}

int settingsRowScreenY(int rowIndex) {
  return settingsContentTop() + (rowIndex * SETTINGS_ROW_H) - settingsScrollY;
}

void clampSettingsScroll() {
  settingsScrollY = constrain(settingsScrollY, 0, settingsMaxScroll());
}

void scrollSettingsBy(int delta) {
  settingsScrollY += delta;
  clampSettingsScroll();
}

void switchApp(AppId app) {
  if (app >= APP_COUNT) return;
  bool enteringSettings = currentApp != APP_SETTINGS && app == APP_SETTINGS;
  currentApp = app;
  stopSignalHarmony();
  dashboardDrawn = false;
  appNeedsRedraw = true;
  if (enteringSettings) settingsScrollY = 0;
  resetPlaceholderState();
  drawActiveApp();
}

void nextApp() {
  uint8_t next = currentApp == APP_SETTINGS ? APP_PULSE : (uint8_t)currentApp + 1;
  if (next > APP_PLACEHOLDER_2) next = APP_PULSE;
  switchApp((AppId)next);
}

void previousApp() {
  uint8_t previous = currentApp == APP_SETTINGS ? APP_PLACEHOLDER_2 : (uint8_t)currentApp;
  previous = previous == APP_PULSE ? APP_PLACEHOLDER_2 : previous - 1;
  switchApp((AppId)previous);
}

void rotateScreen() {
  screenRotationIndex = (screenRotationIndex + 1) % SCREEN_ROTATION_COUNT;
  screenRotation = SCREEN_ROTATIONS[screenRotationIndex];
  Serial.printf("screenRotation=%u\n", screenRotation);
  applyScreenRotation();
  resetDashboardState();
  drawActiveApp();
}

void applyScreenRotation() {
  tft.setRotation(screenRotation);
  configureLayout();
}

void configureLayout() {
  screenWidth = tft.width();
  screenHeight = tft.height();
  portraitLayout = screenHeight > screenWidth;

  rotateButtonWidth = TOOLBAR_BUTTON_WIDTH;
  rotateButtonHeight = TOOLBAR_BUTTON_HEIGHT;
  volumeButtonSize = CONTROL_BUTTON_SIZE;

  if (portraitLayout) {
    headerHeight = 74;
    heartCenterX = 24;
    heartCenterY = 52;

    graphLeft = 8;
    graphTop = 82;
    graphWidth = screenWidth - 16;
    graphHeight = 146;

    bpmPanelX = 8;
    bpmPanelY = 240;
    bpmPanelW = 68;
    bpmPanelH = 72;
    ibiPanelX = 84;
    ibiPanelY = 240;
    ibiPanelW = 68;
    ibiPanelH = 72;
    signalPanelX = 160;
    signalPanelY = 240;
    signalPanelW = 72;
    signalPanelH = 72;

    rotateButtonX = screenWidth - rotateButtonWidth - 4;
    rotateButtonY = 4;
    volumePlusX = rotateButtonX - volumeButtonSize - 8;
    volumeValueX = volumePlusX - 24;
    volumeMinusX = volumeValueX - volumeButtonSize - 2;
    volumeLabelX = volumeMinusX - 28;
    volumeY = 4;

    appSettingsButtonX = rotateButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appButtonY = 4;
  } else {
    headerHeight = 42;
    heartCenterX = 112;
    heartCenterY = 22;

    graphLeft = 8;
    graphTop = 48;
    graphWidth = screenWidth - 16;
    graphHeight = 112;

    bpmPanelX = 8;
    bpmPanelY = 170;
    bpmPanelW = 102;
    bpmPanelH = 62;
    ibiPanelX = 118;
    ibiPanelY = 170;
    ibiPanelW = 102;
    ibiPanelH = 62;
    signalPanelX = 228;
    signalPanelY = 170;
    signalPanelW = 84;
    signalPanelH = 62;

    rotateButtonX = screenWidth - rotateButtonWidth - 4;
    rotateButtonY = 7;
    volumePlusX = rotateButtonX - volumeButtonSize - 6;
    volumeValueX = volumePlusX - 24;
    volumeMinusX = volumeValueX - volumeButtonSize - 2;
    volumeLabelX = volumeMinusX - 28;
    volumeY = 9;

    appSettingsButtonX = rotateButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appButtonY = 7;
  }

  settingsVolMinusX = screenWidth - (TOOLBAR_BUTTON_WIDTH * 2) - APP_BUTTON_GAP - 4;
  settingsVolPlusX = settingsVolMinusX + TOOLBAR_BUTTON_WIDTH + APP_BUTTON_GAP;
  settingsRotateX = screenWidth - 90;
  settingsLedX = settingsRotateX;
  settingsColorRedX = screenWidth - 118;
  settingsColorYellowX = settingsColorRedX + 40;
  settingsColorCyanX = settingsColorYellowX + 40;
  settingsScrollButtonW = (screenWidth - APP_BUTTON_GAP) / 2;
  settingsScrollUpX = 0;
  settingsScrollDownX = settingsScrollButtonW + APP_BUTTON_GAP;
  settingsScrollButtonY = screenHeight - SETTINGS_SCROLL_BUTTON_H - 2;
  clampSettingsScroll();
}

void resetDashboardState() {
  dashboardDrawn = false;
  previousDisplayBPM = -1;
  previousDisplayIBI = -1;
  previousPulseAmplitude = -1;
  previousSignalQuality = -1;
  previousRearmCount = -1;
  previousSignalCoachState = -1;
  previousDashboardLockedSignal = false;
  beatHeartNeedsRedraw = true;
  graphX = 0;
  lastGraphY = signalToGraphY(currentSignal);
  lastGraphDraw = 0;
}

void resetPlaceholderState() {
  placeholderX = screenWidth / 5;
  placeholderY = headerHeight + 32;
  placeholderLastX = -1;
  placeholderLastY = -1;
  placeholderDx = 2;
  placeholderDy = 2;
  lastPlaceholderMove = 0;
}

uint16_t scaledChimeDuty(uint8_t step) {
  step = min<uint8_t>(step, BEAT_CHIME_STEP_COUNT - 1);
  return (BEAT_CHIME_DUTIES[step] * speakerVolume) / VOLUME_MAX;
}

void startBeatChime() {
  stopSignalHarmony();
  beatChimeStep = 0;
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, BEAT_CHIME_FREQUENCIES[beatChimeStep]);
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, scaledChimeDuty(beatChimeStep));
  beatChimeNextStepTime = millis() + BEAT_CHIME_DURATIONS_MS[beatChimeStep];
  beatTonePlaying = true;
}

void updateBeatChime() {
  if (!beatTonePlaying) return;
  if ((long)(millis() - beatChimeNextStepTime) < 0) return;

  beatChimeStep++;
  if (beatChimeStep < BEAT_CHIME_STEP_COUNT) {
    cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, BEAT_CHIME_FREQUENCIES[beatChimeStep]);
    cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, scaledChimeDuty(beatChimeStep));
    beatChimeNextStepTime = millis() + BEAT_CHIME_DURATIONS_MS[beatChimeStep];
    return;
  }

  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  beatTonePlaying = false;
}

uint16_t scaledSignalHarmonyDuty(uint8_t duty) {
  if (speakerVolume == 0) return 0;
  return max<uint16_t>((duty * speakerVolume) / VOLUME_MAX, 4);
}

void startSignalHarmony(int quality) {
  if (speakerVolume == 0 || beatTonePlaying || signalHarmonyPlaying) return;
  if (millis() - lastSignalHarmonyTime < 180) return;

  signalHarmonyBaseNote = constrain(map(quality, 1, SIGNAL_QUALITY_STEPS, 0, 2), 0, 2);
  signalHarmonyStep = 0;
  signalHarmonyPlaying = true;
  lastSignalHarmonyTime = millis();

  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, SIGNAL_HARMONY_FREQUENCIES[signalHarmonyBaseNote]);
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, scaledSignalHarmonyDuty(SIGNAL_HARMONY_DUTIES[signalHarmonyStep]));
  signalHarmonyNextStepTime = millis() + SIGNAL_HARMONY_DURATIONS_MS[signalHarmonyStep];
}

void updateSignalHarmony() {
  if (!signalHarmonyPlaying) return;
  if ((long)(millis() - signalHarmonyNextStepTime) < 0) return;

  signalHarmonyStep++;
  if (signalHarmonyStep < 3) {
    uint8_t note = min<uint8_t>(signalHarmonyBaseNote + signalHarmonyStep, 4);
    cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, SIGNAL_HARMONY_FREQUENCIES[note]);
    cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, scaledSignalHarmonyDuty(SIGNAL_HARMONY_DUTIES[signalHarmonyStep]));
    signalHarmonyNextStepTime = millis() + SIGNAL_HARMONY_DURATIONS_MS[signalHarmonyStep];
    return;
  }

  stopSignalHarmony();
}

void stopSignalHarmony() {
  if (!signalHarmonyPlaying) return;
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  signalHarmonyPlaying = false;
}

void cydLedcAttach(uint8_t pin, uint8_t channel, uint32_t frequency, uint8_t resolution) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, frequency, resolution);
#else
  ledcSetup(channel, frequency, resolution);
  ledcAttachPin(pin, channel);
#endif
}

void cydLedcWrite(uint8_t pin, uint8_t channel, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

void cydLedcWriteTone(uint8_t pin, uint8_t channel, uint32_t frequency) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(pin, frequency);
#else
  ledcWriteTone(channel, frequency);
#endif
}

void triggerRearLedPulse(RearLedColor color) {
  rearLedPulseColor = color;
  rearLedPulseStartTime = millis();
  rearLedPulseActive = true;
  rearLedBrightness = 255;
  setRearLedPulseBrightness(rearLedBrightness);
}

void triggerBeatEffects() {
  if (beatLedEnabled) {
    triggerRearLedPulse(heartbeatLedColor);
  }
  ledPulseStartTime = millis();
  ledPulseActive = true;
  ledBrightness = 255;
  beatHeartNeedsRedraw = true;
  startBeatChime();
}

void setupPulseSensor() {
  // PulseSensorPlayground's detector and ESP32 example expect 10-bit samples.
  // ESP32 defaults to 12-bit, which can make the raw waveform look great while
  // the library's threshold math waits in the wrong scale.
  analogReadResolution(10);

  pulseSensor.analogInput(PULSE_PIN);
  pulseSensor.setThreshold(PULSE_THRESHOLD);
  pulseSensorReady = pulseSensor.begin();

  if (!pulseSensorReady) {
    Serial.println("PulseSensor initialization failed");
  }
}

// ===== SENSOR AND BEAT LOGIC =====

void readPulseSensor() {
  currentSignal = pulseSensor.getLatestSample();
  pulseAmplitude = pulseSensor.getPulseAmplitude();
  insideBeatWindow = pulseSensor.isInsideBeat();
  updateClippingScore();
  updateSignalRange();
  maybeRearmDetector();

  if (pulseSensor.sawStartOfBeat()) {
    int bpm = pulseSensor.getBeatsPerMinute();
    int ibi = pulseSensor.getInterBeatIntervalMs();
    bool qualified = isQualifiedBeat(bpm, ibi, pulseAmplitude);
    int previousQuality = signalQuality;

    lastBeatTime = millis();

    if (qualified) {
      displayBPM = bpm;
      displayIBI = ibi;
      lastQualifiedBeatTime = millis();
      qualifiedBeatStreak++;
      if (qualifiedBeatStreak > LOCK_QUALIFIED_BEATS) qualifiedBeatStreak = LOCK_QUALIFIED_BEATS;
      signalQuality = qualifiedBeatStreak * 3;
      if (signalQuality > SIGNAL_QUALITY_STEPS) signalQuality = SIGNAL_QUALITY_STEPS;
    } else {
      qualifiedBeatStreak = 0;
      signalQuality = 0;
    }

    lockedSignal = qualifiedBeatStreak >= LOCK_QUALIFIED_BEATS;
    if (signalQuality > previousQuality && !lockedSignal) {
      startSignalHarmony(signalQuality);
    }

    if (qualified) {
      if (lockedSignal) {
        triggerBeatEffects();
      } else {
        triggerRearLedPulse(REAR_LED_LOCKING);
      }
    }
  }

  if (millis() - lastQualifiedBeatTime > NO_BEAT_TIMEOUT) {
    lockedSignal = false;
    signalQuality = 0;
    qualifiedBeatStreak = 0;
    displayBPM = 0;
    displayIBI = 0;
  }
}

bool isQualifiedBeat(int bpm, int ibi, int amplitude) {
  if (bpm < MIN_QUALIFIED_BPM || bpm > MAX_QUALIFIED_BPM) return false;
  if (ibi < MIN_QUALIFIED_IBI || ibi > MAX_QUALIFIED_IBI) return false;
  if (amplitude < MIN_QUALIFIED_AMPLITUDE) return false;
  if (maxSignal - minSignal < SIGNAL_COACH_FLAT_RANGE) return false;
  if (clippedSampleScore > 18) return false;
  return true;
}

void updateClippingScore() {
  bool clipped = currentSignal <= 8 || currentSignal >= 1015;

  if (clipped) {
    clippedSampleScore += 8;
    if (clippedSampleScore > 100) clippedSampleScore = 100;
    return;
  }

  if (clippedSampleScore > 0) clippedSampleScore--;
}

int signalCoachState() {
  int liveRange = maxSignal - minSignal;

  if (lockedSignal) return COACH_QUALIFIED;
  if (liveRange < SIGNAL_COACH_FLAT_RANGE || pulseAmplitude < SIGNAL_COACH_FLAT_AMPLITUDE) {
    return COACH_TOO_FLAT;
  }
  if (pulseAmplitude < SIGNAL_COACH_STEADY_AMPLITUDE) return COACH_HOLD_STEADY;
  if (signalQuality >= LOCK_QUALITY_STEPS / 2) return COACH_LOCKING;
  if (liveRange >= REARM_SIGNAL_RANGE) return COACH_GOOD_WAVE;
  return COACH_SIGNAL_SEARCH;
}

const char* signalCoachText() {
  switch (signalCoachState()) {
    case COACH_QUALIFIED:
      return "QUALIFIED BEAT";
    case COACH_TOO_FLAT:
      return "TOO FLAT";
    case COACH_HOLD_STEADY:
      return "HOLD STEADY";
    case COACH_GOOD_WAVE:
      return "GOOD WAVE";
    case COACH_LOCKING:
      return "LOCKING";
    default:
      return "SIGNAL SEARCH";
  }
}

int amplitudeMeterSegments(int amplitude) {
  amplitude = constrain(amplitude, 0, AMPLITUDE_METER_MAX);
  return map(amplitude, 0, AMPLITUDE_METER_MAX, 0, 10);
}

void maybeRearmDetector() {
  unsigned long now = millis();
  int liveRange = maxSignal - minSignal;
  bool signalLooksAlive = liveRange >= REARM_SIGNAL_RANGE;
  bool detectorIsQuiet = (now - lastBeatTime) >= REARM_NO_BEAT_MS;
  bool rearmCooledDown = (now - lastDetectorRearmTime) >= REARM_COOLDOWN_MS;

  if (!lockedSignal && signalLooksAlive && detectorIsQuiet && rearmCooledDown) {
    rearmPulseDetector("alive signal without beat event");
  }
}

void rearmPulseDetector(const char* reason) {
  Serial.print("Re-arming PulseSensor detector: ");
  Serial.println(reason);

  pulseSensor.pause();
  delay(8);
  pulseSensor.resume();

  lastDetectorRearmTime = millis();
  lastBeatTime = millis();
  signalQuality = 0;
  qualifiedBeatStreak = 0;
  displayBPM = 0;
  displayIBI = 0;
  lockedSignal = false;
  rearmCount++;
}

void updateSignalRange() {
  static unsigned long lastDecay = 0;

  if (millis() - lastDecay >= 100) {
    lastDecay = millis();
    minSignal = min(minSignal + 4, currentSignal);
    maxSignal = max(maxSignal - 4, currentSignal);
  }

  minSignal = min(minSignal, currentSignal);
  maxSignal = max(maxSignal, currentSignal);

  if (maxSignal - minSignal < 80) {
    int center = currentSignal;
    minSignal = center - 40;
    maxSignal = center + 40;
  }
}

// ===== STATIC UI =====

void drawActiveApp() {
  if (currentApp == APP_PULSE) {
    drawStaticScreen();
  } else if (currentApp == APP_SETTINGS) {
    drawSettingsScreen();
  } else if (currentApp == APP_PLACEHOLDER_1) {
    tft.fillScreen(COLOR_BG);
    appNeedsRedraw = true;
    drawPlaceholderApp("App 2", "your app here");
  } else if (currentApp == APP_PLACEHOLDER_2) {
    tft.fillScreen(COLOR_BG);
    appNeedsRedraw = true;
    drawPlaceholderApp("App 3", "your app here too");
  }
}

void drawStaticScreen() {
  tft.fillScreen(COLOR_BG);
  drawGraphFrame();
  drawDashboardIfChanged();
}

void drawDashboardIfChanged() {
  int coach = signalCoachState();
  bool statusChanged = !dashboardDrawn ||
                       lockedSignal != previousDashboardLockedSignal ||
                       coach != previousSignalCoachState;
  bool panelsChanged = statusChanged ||
                       displayBPM != previousDisplayBPM ||
                       displayIBI != previousDisplayIBI ||
                       signalQuality != previousSignalQuality;

  if (statusChanged) {
    drawHeader();
  }

  if (panelsChanged) {
    drawPanels();
  }

  if (statusChanged || panelsChanged) {
    dashboardDrawn = true;
    previousDashboardLockedSignal = lockedSignal;
    previousLockedSignal = lockedSignal;
    previousDisplayBPM = displayBPM;
    previousDisplayIBI = displayIBI;
    previousPulseAmplitude = pulseAmplitude;
    previousSignalQuality = signalQuality;
    previousRearmCount = rearmCount;
    previousSignalCoachState = coach;
  }
}

void drawHeader() {
  tft.fillRect(0, 0, screenWidth, headerHeight, COLOR_BG);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, COLOR_GRID);
  beatHeartNeedsRedraw = true;

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(portraitLayout ? 52 : 10, portraitLayout ? 38 : 8);
  tft.print("PulseSensor.com");
  drawAppNavControls();
  drawRotateControl();

  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(portraitLayout ? 62 : 10, portraitLayout ? 58 : 25);
  tft.print(signalCoachText());
}

void drawAppNavControls() {
  drawAppButton(appPrevButtonX, appButtonY, "<", false);
  drawAppButton(appNextButtonX, appButtonY, ">", false);
  drawAppButton(appSettingsButtonX, appButtonY, "*", currentApp == APP_SETTINGS);
}

void drawAppButton(int x, int y, const char* label, bool active) {
  uint16_t fill = active ? COLOR_CYAN_DARK : COLOR_PANEL;
  uint16_t outline = active ? COLOR_CYAN : COLOR_GRID;
  tft.fillRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 10, APP_BUTTON_WIDTH, 1, COLOR_TEXT, fill);
}

void drawSettingsScreen() {
  appNeedsRedraw = false;
  clampSettingsScroll();
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, screenWidth, headerHeight, COLOR_BG);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, COLOR_GRID);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 38 : 8);
  tft.print("Settings ");
  tft.print(APP_FIRMWARE_DATE);
  drawAppNavControls();
  drawRotateControl();

  char volumeText[12];
  snprintf(volumeText, sizeof(volumeText), "%u/10", speakerVolume);
  int rowY = settingsRowScreenY(0);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(0, rowY, "Volume", volumeText);
    int buttonY = rowY + 8;
    drawSettingsButton(settingsVolMinusX, buttonY, TOOLBAR_BUTTON_WIDTH, "-", false);
    drawSettingsButton(settingsVolPlusX, buttonY, TOOLBAR_BUTTON_WIDTH, "+", false);
  }

  char rotationText[14];
  snprintf(rotationText, sizeof(rotationText), "rot %u", screenRotation);
  rowY = settingsRowScreenY(1);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(1, rowY, "Rotation", rotationText);
    drawSettingsButton(settingsRotateX, rowY + 8, 86, "ROT", false);
  }

  rowY = settingsRowScreenY(2);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(2, rowY, "WiFi", "setup later");
  }

  rowY = settingsRowScreenY(3);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(3, rowY, "Bluetooth", "setup later");
  }

  rowY = settingsRowScreenY(4);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(4, rowY, "LED Control", beatLedEnabled ? "beat pulse" : "off");
    drawSettingsButton(settingsLedX, rowY + 8, 86, beatLedEnabled ? "BEAT" : "OFF", beatLedEnabled);
  }

  rowY = settingsRowScreenY(5);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(5, rowY, "Color", "tap");
    int swatchY = rowY + 8;
    drawSettingsSwatch(settingsColorRedX, swatchY, COLOR_RED, heartbeatLedColor.red > 0 && heartbeatLedColor.green == 0);
    drawSettingsSwatch(settingsColorYellowX, swatchY, COLOR_SIGNAL_YELLOW, heartbeatLedColor.red > 0 && heartbeatLedColor.green > 0);
    drawSettingsSwatch(settingsColorCyanX, swatchY, COLOR_CYAN, heartbeatLedColor.blue > 0);
  }

  rowY = settingsRowScreenY(6);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(6, rowY, "About", "PulseSensor CYD");
  }

  rowY = settingsRowScreenY(7);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(7, rowY, "Version", APP_VERSION);
  }

  rowY = settingsRowScreenY(8);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(8, rowY, "Firmware", APP_FIRMWARE_DATE);
  }

  drawSettingsScrollControls();
}

void drawSettingsRow(int rowIndex, int y, const char* label, const char* value) {
  uint16_t bg = settingsRowBackground(rowIndex);
  tft.fillRect(0, y, screenWidth, SETTINGS_ROW_H, bg);
  tft.drawFastHLine(0, y + SETTINGS_ROW_H - 1, screenWidth, COLOR_BG);

  tft.setTextSize(SETTINGS_TEXT_SIZE);
  tft.setTextColor(COLOR_BG, bg);
  tft.setCursor(10, y + 2);
  tft.print(label);
  tft.setTextColor(COLOR_BG, bg);
  tft.setCursor(10, y + 21);
  tft.print(value);
}

uint16_t settingsRowBackground(int rowIndex) {
  return rowIndex % 2 == 0 ? COLOR_SIGNAL_YELLOW : COLOR_LOCK_GREEN;
}

void drawSettingsButton(int x, int y, int w, const char* label, bool active) {
  uint16_t fill = active ? COLOR_CYAN_DARK : COLOR_PANEL;
  uint16_t outline = active ? COLOR_CYAN : COLOR_GRID;
  tft.fillRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 6, w, 2, COLOR_TEXT, fill);
}

void drawSettingsSwatch(int x, int y, uint16_t color, bool active) {
  uint16_t outline = active ? COLOR_TEXT : COLOR_GRID;
  tft.fillRoundRect(x, y, 34, TOOLBAR_BUTTON_HEIGHT, 4, color);
  tft.drawRoundRect(x, y, 34, TOOLBAR_BUTTON_HEIGHT, 4, outline);
  if (active) {
    tft.drawRoundRect(x + 2, y + 2, 30, TOOLBAR_BUTTON_HEIGHT - 4, 3, COLOR_BG);
  }
}

void drawSettingsScrollControls() {
  bool canScrollUp = settingsScrollY > 0;
  bool canScrollDown = settingsScrollY < settingsMaxScroll();
  tft.fillRect(0, settingsScrollButtonY - 3, screenWidth, SETTINGS_SCROLL_BUTTON_H + 7, COLOR_BG);
  tft.drawFastHLine(0, settingsScrollButtonY - 4, screenWidth, COLOR_GRID);
  drawSettingsButton(settingsScrollUpX, settingsScrollButtonY, settingsScrollButtonW, "^", canScrollUp);
  drawSettingsButton(settingsScrollDownX, settingsScrollButtonY, settingsScrollButtonW, "v", canScrollDown);
}

void drawPlaceholderApp(const char* title, const char* message) {
  if (appNeedsRedraw) {
    appNeedsRedraw = false;
    tft.fillScreen(COLOR_BG);
    tft.fillRect(0, 0, screenWidth, headerHeight, COLOR_BG);
    tft.drawFastHLine(0, headerHeight - 1, screenWidth, COLOR_GRID);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 38 : 8);
    tft.print(title);
    drawAppNavControls();
    drawRotateControl();
    resetPlaceholderState();
  }

  if (millis() - lastPlaceholderMove < PLACEHOLDER_STEP_MS) return;
  lastPlaceholderMove = millis();

  int textW = strlen(message) * 12;
  int textH = 16;
  int minY = headerHeight + 8;
  int maxX = max(0, screenWidth - textW - 4);
  int maxY = max(minY, screenHeight - textH - 8);

  if (placeholderLastX >= 0) {
    tft.fillRect(placeholderLastX - 2, placeholderLastY - 2, textW + 4, textH + 4, COLOR_BG);
  }

  placeholderX += placeholderDx;
  placeholderY += placeholderDy;

  if (placeholderX <= 4 || placeholderX >= maxX) {
    placeholderDx = -placeholderDx;
    placeholderX = constrain(placeholderX, 4, maxX);
  }
  if (placeholderY <= minY || placeholderY >= maxY) {
    placeholderDy = -placeholderDy;
    placeholderY = constrain(placeholderY, minY, maxY);
  }

  drawPlaceholderText(message, currentApp == APP_PLACEHOLDER_1 ? COLOR_CYAN : COLOR_SIGNAL_YELLOW);
}

void drawPlaceholderText(const char* message, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_BG);
  tft.setCursor(placeholderX, placeholderY);
  tft.print(message);
  placeholderLastX = placeholderX;
  placeholderLastY = placeholderY;
}

void drawRotateControl() {
  tft.fillRoundRect(rotateButtonX, rotateButtonY, rotateButtonWidth, rotateButtonHeight, 4, COLOR_PANEL);
  tft.drawRoundRect(rotateButtonX, rotateButtonY, rotateButtonWidth, rotateButtonHeight, 4, COLOR_GRID);

  int cx = rotateButtonX + rotateButtonWidth / 2;
  int cy = rotateButtonY + rotateButtonHeight / 2;

  tft.drawCircle(cx, cy, 7, COLOR_CYAN);
  tft.drawCircle(cx, cy, 6, COLOR_CYAN_DARK);
  tft.fillRect(cx - 8, cy - 9, 8, 7, COLOR_PANEL);
  tft.drawFastHLine(cx - 1, cy - 7, 7, COLOR_CYAN);
  tft.drawFastVLine(cx + 5, cy - 7, 6, COLOR_CYAN);
  tft.fillTriangle(cx + 2, cy - 9,
                   cx + 8, cy - 7,
                   cx + 5, cy - 2,
                   COLOR_CYAN);
}

void drawVolumeControl() {
  char volumeText[4];
  snprintf(volumeText, sizeof(volumeText), "%u", speakerVolume);

  tft.fillRect(volumeLabelX, volumeY - 5, rotateButtonX - volumeLabelX - 2, volumeButtonSize + 14, COLOR_BG);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setCursor(volumeLabelX, volumeY + 8);
  tft.print("VOL");

  tft.fillRoundRect(volumeMinusX, volumeY, volumeButtonSize, volumeButtonSize, 4, COLOR_PANEL);
  tft.drawRoundRect(volumeMinusX, volumeY, volumeButtonSize, volumeButtonSize, 4, COLOR_GRID);
  tft.fillRoundRect(volumePlusX, volumeY, volumeButtonSize, volumeButtonSize, 4, COLOR_PANEL);
  tft.drawRoundRect(volumePlusX, volumeY, volumeButtonSize, volumeButtonSize, 4, COLOR_GRID);

  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawCenteredText("-", volumeMinusX, volumeY + 5, volumeButtonSize, 2, COLOR_TEXT, COLOR_PANEL);
  drawCenteredText("+", volumePlusX, volumeY + 5, volumeButtonSize, 2, COLOR_TEXT, COLOR_PANEL);
  drawCenteredText(volumeText, volumeValueX, volumeY + 7, 22, 1, COLOR_TEXT, COLOR_BG);
}

void drawGraphFrame() {
  tft.fillRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, COLOR_PANEL_DARK);
  tft.drawRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, COLOR_GRID);
  tft.fillRect(graphLeft, graphTop, graphWidth, graphHeight, COLOR_BG);

  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;

  for (int x = 0; x <= graphWidth; x += verticalGridStep) {
    tft.drawFastVLine(graphLeft + x, graphTop, graphHeight, COLOR_GRID_SOFT);
  }
  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawFastHLine(graphLeft, graphTop + y, graphWidth, COLOR_GRID_SOFT);
  }
  for (int x = 0; x < graphWidth; x += 6) {
    drawThresholdMarker(x);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.setCursor(graphLeft + 6, graphTop + 5);
  tft.print("LIVE LINE");

  tft.setCursor(graphLeft + graphWidth - 48, graphTop + 5);
  tft.print("THR ");
  tft.print(PULSE_THRESHOLD);
}

void drawGraphColumnBackground(int localX) {
  int screenX = graphLeft + localX;
  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;
  tft.drawFastVLine(screenX, graphTop, graphHeight, COLOR_BG);

  if (localX % verticalGridStep == 0) {
    tft.drawFastVLine(screenX, graphTop, graphHeight, COLOR_GRID_SOFT);
  }

  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawPixel(screenX, graphTop + y, COLOR_GRID_SOFT);
  }

  drawThresholdMarker(localX);
}

void drawThresholdMarker(int localX) {
  int y = signalToGraphY(PULSE_THRESHOLD);

  if (localX % 6 == 0) {
    tft.drawPixel(graphLeft + localX, y, COLOR_SCREEN_BEAT);
  }
}

int signalToGraphY(int signal) {
  if (minSignal == maxSignal) {
    return graphTop + graphHeight / 2;
  }

  int y = map(signal, minSignal, maxSignal, graphTop + graphHeight - 8, graphTop + 8);
  return constrain(y, graphTop + 8, graphTop + graphHeight - 8);
}

// ===== LIVE GRAPH =====

void drawWaveform() {
  if (millis() - lastGraphDraw < 20) return;
  lastGraphDraw = millis();

  int y = signalToGraphY(currentSignal);

  drawGraphColumnBackground(graphX);
  drawGraphColumnBackground((graphX + 1) % graphWidth);
  drawGraphColumnBackground((graphX + 2) % graphWidth);

  uint16_t waveColor = liveTraceColor();

  if (graphX > 0) {
    tft.drawLine(graphLeft + graphX - 1, lastGraphY, graphLeft + graphX, y, waveColor);
    tft.drawPixel(graphLeft + graphX, y - 1, waveColor);
    tft.drawPixel(graphLeft + graphX, y + 1, waveColor);
  }

  if (ledBrightness > 180) {
    tft.fillCircle(graphLeft + graphX, y, 3, COLOR_SCREEN_BEAT);
  }

  lastGraphY = y;
  graphX++;

  if (graphX >= graphWidth) {
    graphX = 0;
    lastGraphY = y;
    drawGraphFrame();
  }
}

// ===== DASHBOARD PANELS =====

void drawPanels() {
  drawMetricPanel(bpmPanelX, bpmPanelY, bpmPanelW, bpmPanelH, "BPM", displayBPM, "", lockedSignal);
  drawMetricPanel(ibiPanelX, ibiPanelY, ibiPanelW, ibiPanelH, "IBI", displayIBI, "ms", lockedSignal);
  drawSignalPanel();
}

void drawMetricPanel(int x, int y, int w, int h, const char* label, int value, const char* unit, bool valid) {
  uint16_t panelBg = metricPanelBackground(label, valid);
  tft.fillRoundRect(x, y, w, h, 6, panelBg);
  tft.drawRoundRect(x, y, w, h, 6, COLOR_BG);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_BG, panelBg);
  tft.setCursor(x + 8, y + 8);
  tft.print(label);

  uint8_t valueTextSize = portraitLayout ? 3 : 4;
  if (portraitLayout && strcmp(label, "IBI") == 0 && value >= 1000) {
    valueTextSize = 2;
  }
  tft.setTextSize(valueTextSize);
  tft.setTextColor(COLOR_BG, panelBg);
  tft.setCursor(x + 8, y + (portraitLayout ? 30 : 25));

  if (valid) {
    if (strcmp(label, "BPM") == 0) {
      tft.printf(portraitLayout ? "%2d" : "%3d", value);
    } else {
      tft.setTextSize(portraitLayout ? valueTextSize : 3);
      tft.printf(portraitLayout ? "%2d" : "%3d", value);
    }
  } else {
    tft.print("--");
  }

  if (valid && unit[0] != '\0') {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG, panelBg);
    tft.setCursor(x + (portraitLayout ? 43 : 72), y + h - 17);
    tft.print(unit);
  }
}

uint16_t metricPanelBackground(const char* label, bool valid) {
  if (strcmp(label, "BPM") == 0) {
    return valid ? COLOR_LOCK_GREEN : COLOR_SIGNAL_YELLOW;
  }
  return valid ? COLOR_SIGNAL_YELLOW : COLOR_LOCK_GREEN;
}

void drawSignalPanel() {
  char pinLabel[8];

  snprintf(pinLabel, sizeof(pinLabel), "GPIO%d", PULSE_PIN);

  uint16_t panelBg = lockedSignal ? COLOR_LOCK_GREEN : COLOR_SIGNAL_YELLOW;
  tft.fillRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, panelBg);
  tft.drawRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, COLOR_BG);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_BG, panelBg);
  tft.setCursor(signalPanelX + 8, signalPanelY + 8);
  tft.print(portraitLayout ? "SIG" : "SIG ");
  if (!portraitLayout) {
    tft.print(pinLabel);
  }

  if (portraitLayout) {
    tft.setCursor(signalPanelX + 8, signalPanelY + 20);
    tft.print(pinLabel);
    drawQualitySegments(signalPanelX + 8, signalPanelY + 34);
  } else {
    drawQualitySegments(signalPanelX + 9, signalPanelY + 28);
  }
}

void drawQualitySegments(int x, int y) {
  const int segmentW = portraitLayout ? 3 : 4;
  const int segmentGap = 2;
  const int segmentH = portraitLayout ? 8 : 14;
  for (int i = 0; i < SIGNAL_QUALITY_STEPS; i++) {
    uint16_t color = COLOR_GRID;
    if (i < signalQuality) {
      color = COLOR_BG;
    }
    tft.fillRect(x + i * (segmentW + segmentGap), y, segmentW, segmentH, color);
  }
}

void drawAmplitudeMeter(int x, int y, int amplitude) {
  int segments = amplitudeMeterSegments(amplitude);
  int displayAmplitude = constrain(amplitude, 0, 999);

  for (int i = 0; i < 10; i++) {
    uint16_t color = COLOR_GRID;
    if (i < segments) {
      color = segments >= 7 ? COLOR_TEAL : COLOR_AMBER;
    }
    tft.fillRect(x + i * 4, y, 3, 7, color);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 43, y);
  tft.printf("A%03d", displayAmplitude);
}

void drawBeatHeart() {
  static unsigned long lastDraw = 0;
  static int lastSize = -1;
  static int lastBrightness = -1;

  if (!beatHeartNeedsRedraw && millis() - lastDraw < 30 && lastSize >= 0) return;

  int size = map(ledBrightness, 0, 255, HEART_MIN_SIZE, HEART_MAX_SIZE);
  size = constrain(size, HEART_MIN_SIZE, HEART_MAX_SIZE);

  if (!beatHeartNeedsRedraw && size == lastSize && ledBrightness == lastBrightness) return;

  const int centerX = heartCenterX;
  const int centerY = heartCenterY;
  const int clearX = centerX - HEART_MAX_SIZE - 4;
  const int clearY = centerY - HEART_MAX_SIZE - 4;
  const int clearW = (HEART_MAX_SIZE + 4) * 2 + 1;
  const int clearH = HEART_MAX_SIZE * 2 + 7;

  uint16_t heartColor = blendRed(ledBrightness);
  uint16_t outlineColor = COLOR_SCREEN_BEAT;
  tft.fillRect(clearX, clearY, clearW, clearH, COLOR_BG);
  fillHeartShape(centerX, centerY, size + 2, outlineColor);
  fillHeartShape(centerX, centerY, size, heartColor);

  lastDraw = millis();
  lastSize = size;
  lastBrightness = ledBrightness;
  beatHeartNeedsRedraw = false;
}

void fillHeartShape(int centerX, int centerY, int size, uint16_t color) {
  tft.fillCircle(centerX - size / 2, centerY - size / 3, size / 2, color);
  tft.fillCircle(centerX + size / 2, centerY - size / 3, size / 2, color);
  tft.fillTriangle(centerX - size, centerY - size / 4,
                   centerX + size, centerY - size / 4,
                   centerX, centerY + size, color);
}

void drawCenteredText(const char* text, int x, int y, int w, int textSize, uint16_t color, uint16_t bg) {
  int charW = 6 * textSize;
  int textW = strlen(text) * charW;
  int cursorX = x + max(0, (w - textW) / 2);
  tft.setTextSize(textSize);
  tft.setTextColor(color, bg);
  tft.setCursor(cursorX, y);
  tft.print(text);
}

uint16_t liveTraceColor() {
  return lockedSignal ? COLOR_TEXT : COLOR_CYAN;
}

uint16_t blendRed(int brightness) {
  brightness = constrain(brightness, 0, 255);
  if (brightness < 20) return COLOR_RED_DARK;
  if (brightness < 120) return 0xA800;
  return COLOR_RED;
}
