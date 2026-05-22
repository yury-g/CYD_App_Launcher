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
int rotateButtonSize = CONTROL_BUTTON_SIZE;
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
bool rearLedPulseActive = false;
unsigned long rearLedPulseStartTime = 0;
#define LED_UPDATE_MS 10
#define LED_PEAK_HOLD_MS 90
#define LED_FADE_MS 620

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
bool handleRotateTouch(int16_t x, int16_t y);
bool handleVolumeTouch(int16_t x, int16_t y);
void rotateScreen();
void applyScreenRotation();
void configureLayout();
void resetDashboardState();
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
void drawStaticScreen();
void drawDashboardIfChanged();
void drawHeader();
void drawRotateControl();
void drawVolumeControl();
void drawGraphFrame();
void drawGraphColumnBackground(int localX);
void drawThresholdMarker(int localX);
int signalToGraphY(int signal);
void drawWaveform();
void drawPanels();
void drawMetricPanel(int x, int y, int w, int h, const char* label, int value, const char* unit, bool valid);
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
  drawStaticScreen();
}

void loop() {
  readPulseSensor();
  readTouchControls();
  updateLED();
  updateBeatChime();
  updateSignalHarmony();
  drawBeatHeart();
  drawWaveform();
  drawDashboardIfChanged();

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

  if (handleRotateTouch(x, y) || handleVolumeTouch(x, y)) {
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

bool handleRotateTouch(int16_t x, int16_t y) {
  if (y < rotateButtonY - 8 || y > rotateButtonY + rotateButtonSize + 8) return false;
  if (x < rotateButtonX - 8 || x > rotateButtonX + rotateButtonSize + 8) return false;

  rotateScreen();
  return true;
}

bool handleVolumeTouch(int16_t x, int16_t y) {
  if (y < volumeY - 8 || y > volumeY + volumeButtonSize + 8) return false;

  if (x >= volumeMinusX - 8 && x <= volumeMinusX + volumeButtonSize + 8) {
    if (speakerVolume > VOLUME_MIN) speakerVolume--;
  } else if (x >= volumePlusX - 8 && x <= volumePlusX + volumeButtonSize + 8) {
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

void rotateScreen() {
  screenRotationIndex = (screenRotationIndex + 1) % SCREEN_ROTATION_COUNT;
  screenRotation = SCREEN_ROTATIONS[screenRotationIndex];
  Serial.printf("screenRotation=%u\n", screenRotation);
  applyScreenRotation();
  resetDashboardState();
  drawStaticScreen();
}

void applyScreenRotation() {
  tft.setRotation(screenRotation);
  configureLayout();
}

void configureLayout() {
  screenWidth = tft.width();
  screenHeight = tft.height();
  portraitLayout = screenHeight > screenWidth;

  rotateButtonSize = CONTROL_BUTTON_SIZE;
  volumeButtonSize = CONTROL_BUTTON_SIZE;

  if (portraitLayout) {
    headerHeight = 58;
    heartCenterX = 40;
    heartCenterY = 42;

    graphLeft = 8;
    graphTop = 66;
    graphWidth = screenWidth - 16;
    graphHeight = 162;

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

    rotateButtonX = screenWidth - rotateButtonSize - 4;
    rotateButtonY = 4;
    volumePlusX = rotateButtonX - volumeButtonSize - 8;
    volumeValueX = volumePlusX - 24;
    volumeMinusX = volumeValueX - volumeButtonSize - 2;
    volumeLabelX = volumeMinusX - 28;
    volumeY = 4;
  } else {
    headerHeight = 42;
    heartCenterX = screenWidth / 2;
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

    rotateButtonX = screenWidth - rotateButtonSize - 4;
    rotateButtonY = 9;
    volumePlusX = rotateButtonX - volumeButtonSize - 6;
    volumeValueX = volumePlusX - 24;
    volumeMinusX = volumeValueX - volumeButtonSize - 2;
    volumeLabelX = volumeMinusX - 28;
    volumeY = 9;
  }
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
  triggerRearLedPulse(REAR_LED_HEARTBEAT);
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
  tft.setCursor(10, portraitLayout ? 7 : 8);
  tft.print("PulseSensor.com");
  drawVolumeControl();
  drawRotateControl();

  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(portraitLayout ? 62 : 10, portraitLayout ? 47 : 25);
  tft.print(signalCoachText());
}

void drawRotateControl() {
  tft.fillRoundRect(rotateButtonX, rotateButtonY, rotateButtonSize, rotateButtonSize, 4, COLOR_PANEL);
  tft.drawRoundRect(rotateButtonX, rotateButtonY, rotateButtonSize, rotateButtonSize, 4, COLOR_GRID);

  int cx = rotateButtonX + rotateButtonSize / 2;
  int cy = rotateButtonY + rotateButtonSize / 2;

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
  tft.fillRoundRect(x, y, w, h, 6, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 6, valid ? COLOR_TEAL : COLOR_GRID);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 8, y + 8);
  tft.print(label);

  uint8_t valueTextSize = portraitLayout ? 3 : 4;
  if (portraitLayout && strcmp(label, "IBI") == 0 && value >= 1000) {
    valueTextSize = 2;
  }
  tft.setTextSize(valueTextSize);
  tft.setTextColor(valid ? COLOR_TEXT : COLOR_MUTED, COLOR_PANEL);
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
    tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
    tft.setCursor(x + (portraitLayout ? 43 : 72), y + h - 17);
    tft.print(unit);
  }
}

void drawSignalPanel() {
  char pinLabel[8];

  snprintf(pinLabel, sizeof(pinLabel), "GPIO%d", PULSE_PIN);

  tft.fillRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, COLOR_PANEL);
  tft.drawRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, lockedSignal ? COLOR_LOCK_GREEN : COLOR_SIGNAL_YELLOW);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
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
      color = lockedSignal ? COLOR_LOCK_GREEN : COLOR_SIGNAL_YELLOW;
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
