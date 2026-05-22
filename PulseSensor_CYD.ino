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
#define HEART_CENTER_X 160
#define HEART_CENTER_Y 22
#define HEART_MIN_SIZE 8
#define HEART_MAX_SIZE 15
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_START 1
#define SCREEN_ROTATION_DEFAULT 1
#define SCREEN_ROTATION_FLIPPED 3

// ===== TOUCH CALIBRATION =====

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800

// ===== SCREEN LAYOUT =====

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define GRAPH_X 8
#define GRAPH_Y 48
#define GRAPH_W 304
#define GRAPH_H 112

#define PANEL_Y 170
#define PANEL_H 62

#define ROTATE_BUTTON_X 190
#define ROTATE_BUTTON_Y 9
#define ROTATE_BUTTON_SIZE 22
#define VOL_LABEL_X 216
#define VOL_MINUS_X 240
#define VOL_VALUE_X 268
#define VOL_PLUS_X 294
#define VOL_Y 9
#define VOL_BUTTON_SIZE 22

// ===== COLORS (RGB565) =====

#define COLOR_BG 0x0000
#define COLOR_PANEL 0x0841
#define COLOR_PANEL_DARK 0x0400
#define COLOR_GRID 0x18E3
#define COLOR_GRID_SOFT 0x10A2
#define COLOR_TEXT 0xFFFF
#define COLOR_MUTED 0x8C71
#define COLOR_CYAN 0x07FF
#define COLOR_CYAN_DARK 0x0452
#define COLOR_TEAL 0x05F3
#define COLOR_RED 0xF800
#define COLOR_RED_DARK 0x6000
#define COLOR_SCREEN_BEAT COLOR_CYAN
#define COLOR_AMBER 0xFBE0

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

bool signalHarmonyPlaying = false;
uint8_t signalHarmonyStep = 0;
uint8_t signalHarmonyBaseNote = 0;
unsigned long signalHarmonyNextStepTime = 0;

// ===== GRAPH STATE =====

int graphX = 0;
int lastGraphY = GRAPH_Y + GRAPH_H / 2;

// ===== RED LED FADE STATE =====

int ledBrightness = 0;
#define LED_FADE_SPEED 12

// ===== FORWARD DECLARATIONS =====

void setup();
void loop();
void setupLED();
void setRedLED(int brightness);
void updateLED();
void setupSpeaker();
void setupTouch();
void readTouchControls();
void mapTouchPoint(const TS_Point& point, int16_t* x, int16_t* y);
bool handleRotateTouch(int16_t x, int16_t y);
bool handleVolumeTouch(int16_t x, int16_t y);
void rotateScreen();
void applyScreenRotation();
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
void drawMetricPanel(int x, const char* label, int value, const char* unit, bool valid);
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

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  tft.init();
  applyScreenRotation();
  tft.fillScreen(COLOR_BG);

  setupLED();
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
  // The CYD RGB LED is active-low.
  cydLedcAttach(LED_RED_PIN, LED_RED_PWM_CH, 5000, 8);
  cydLedcAttach(LED_GREEN_PIN, LED_GREEN_PWM_CH, 5000, 8);
  cydLedcAttach(LED_BLUE_PIN, LED_BLUE_PWM_CH, 5000, 8);

  setRedLED(0);
  cydLedcWrite(LED_GREEN_PIN, LED_GREEN_PWM_CH, 255);
  cydLedcWrite(LED_BLUE_PIN, LED_BLUE_PWM_CH, 255);
}

void setRedLED(int brightness) {
  brightness = constrain(brightness, 0, 255);
  cydLedcWrite(LED_RED_PIN, LED_RED_PWM_CH, 255 - brightness);
}

void updateLED() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 10) return;

  lastUpdate = millis();
  if (ledBrightness > 0) {
    ledBrightness -= LED_FADE_SPEED;
    if (ledBrightness < 0) ledBrightness = 0;
  }
  setRedLED(ledBrightness);
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
  int16_t mappedX = constrain(map(point.x, TOUCH_MIN_X, TOUCH_MAX_X, 1, SCREEN_WIDTH), 0, SCREEN_WIDTH - 1);
  int16_t mappedY = constrain(map(point.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 1, SCREEN_HEIGHT), 0, SCREEN_HEIGHT - 1);

  if (screenRotation == SCREEN_ROTATION_FLIPPED) {
    mappedX = SCREEN_WIDTH - 1 - mappedX;
    mappedY = SCREEN_HEIGHT - 1 - mappedY;
  }

  *x = mappedX;
  *y = mappedY;
}

bool handleRotateTouch(int16_t x, int16_t y) {
  if (y < ROTATE_BUTTON_Y - 8 || y > ROTATE_BUTTON_Y + ROTATE_BUTTON_SIZE + 8) return false;
  if (x < ROTATE_BUTTON_X - 8 || x > ROTATE_BUTTON_X + ROTATE_BUTTON_SIZE + 8) return false;

  rotateScreen();
  return true;
}

bool handleVolumeTouch(int16_t x, int16_t y) {
  if (y < VOL_Y - 8 || y > VOL_Y + VOL_BUTTON_SIZE + 8) return false;

  if (x >= VOL_MINUS_X - 8 && x <= VOL_MINUS_X + VOL_BUTTON_SIZE + 8) {
    if (speakerVolume > VOLUME_MIN) speakerVolume--;
  } else if (x >= VOL_PLUS_X - 8 && x <= VOL_PLUS_X + VOL_BUTTON_SIZE + 8) {
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
  screenRotation = screenRotation == SCREEN_ROTATION_DEFAULT ? SCREEN_ROTATION_FLIPPED : SCREEN_ROTATION_DEFAULT;
  Serial.printf("screenRotation=%u\n", screenRotation);
  applyScreenRotation();
  resetDashboardState();
  drawStaticScreen();
}

void applyScreenRotation() {
  tft.setRotation(screenRotation);
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

void triggerBeatEffects() {
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

    // Blink/fade the rear red LED only after the beat is qualified.
    if (lockedSignal && qualified) {
      triggerBeatEffects();
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
  tft.fillRect(0, 0, SCREEN_WIDTH, 42, COLOR_BG);
  tft.drawFastHLine(0, 41, SCREEN_WIDTH, COLOR_GRID);
  beatHeartNeedsRedraw = true;

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(10, 8);
  tft.print("PulseSensor.com");
  drawRotateControl();
  drawVolumeControl();

  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(10, 25);
  tft.print(signalCoachText());
}

void drawRotateControl() {
  tft.fillRoundRect(ROTATE_BUTTON_X, ROTATE_BUTTON_Y, ROTATE_BUTTON_SIZE, ROTATE_BUTTON_SIZE, 4, COLOR_PANEL);
  tft.drawRoundRect(ROTATE_BUTTON_X, ROTATE_BUTTON_Y, ROTATE_BUTTON_SIZE, ROTATE_BUTTON_SIZE, 4, COLOR_GRID);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN, COLOR_PANEL);
  tft.setCursor(ROTATE_BUTTON_X + 6, ROTATE_BUTTON_Y + 3);
  tft.print("R");
  tft.drawFastHLine(ROTATE_BUTTON_X + 5, ROTATE_BUTTON_Y + 14, 12, COLOR_CYAN);
  tft.drawFastVLine(ROTATE_BUTTON_X + 16, ROTATE_BUTTON_Y + 10, 5, COLOR_CYAN);
  tft.fillTriangle(ROTATE_BUTTON_X + 16, ROTATE_BUTTON_Y + 8,
                   ROTATE_BUTTON_X + 20, ROTATE_BUTTON_Y + 12,
                   ROTATE_BUTTON_X + 16, ROTATE_BUTTON_Y + 16,
                   COLOR_CYAN);
}

void drawVolumeControl() {
  char volumeText[4];
  snprintf(volumeText, sizeof(volumeText), "%u", speakerVolume);

  tft.fillRect(VOL_LABEL_X, 4, SCREEN_WIDTH - VOL_LABEL_X, 34, COLOR_BG);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setCursor(VOL_LABEL_X, 17);
  tft.print("VOL");

  tft.fillRoundRect(VOL_MINUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_PANEL);
  tft.drawRoundRect(VOL_MINUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_GRID);
  tft.fillRoundRect(VOL_PLUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_PANEL);
  tft.drawRoundRect(VOL_PLUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_GRID);

  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawCenteredText("-", VOL_MINUS_X, VOL_Y + 5, VOL_BUTTON_SIZE, 2, COLOR_TEXT, COLOR_PANEL);
  drawCenteredText("+", VOL_PLUS_X, VOL_Y + 5, VOL_BUTTON_SIZE, 2, COLOR_TEXT, COLOR_PANEL);
  drawCenteredText(volumeText, VOL_VALUE_X, VOL_Y + 7, 26, 1, COLOR_TEXT, COLOR_BG);
}

void drawGraphFrame() {
  tft.fillRoundRect(GRAPH_X - 2, GRAPH_Y - 2, GRAPH_W + 4, GRAPH_H + 4, 6, COLOR_PANEL_DARK);
  tft.drawRoundRect(GRAPH_X - 2, GRAPH_Y - 2, GRAPH_W + 4, GRAPH_H + 4, 6, COLOR_GRID);
  tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, COLOR_BG);

  for (int x = 0; x <= GRAPH_W; x += 38) {
    tft.drawFastVLine(GRAPH_X + x, GRAPH_Y, GRAPH_H, COLOR_GRID_SOFT);
  }
  for (int y = 0; y <= GRAPH_H; y += 28) {
    tft.drawFastHLine(GRAPH_X, GRAPH_Y + y, GRAPH_W, COLOR_GRID_SOFT);
  }
  for (int x = 0; x < GRAPH_W; x += 6) {
    drawThresholdMarker(x);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.setCursor(GRAPH_X + 6, GRAPH_Y + 5);
  tft.print("LIVE LINE");

  tft.setCursor(GRAPH_X + GRAPH_W - 48, GRAPH_Y + 5);
  tft.print("THR ");
  tft.print(PULSE_THRESHOLD);
}

void drawGraphColumnBackground(int localX) {
  int screenX = GRAPH_X + localX;
  tft.drawFastVLine(screenX, GRAPH_Y, GRAPH_H, COLOR_BG);

  if (localX % 38 == 0) {
    tft.drawFastVLine(screenX, GRAPH_Y, GRAPH_H, COLOR_GRID_SOFT);
  }

  for (int y = 0; y <= GRAPH_H; y += 28) {
    tft.drawPixel(screenX, GRAPH_Y + y, COLOR_GRID_SOFT);
  }

  drawThresholdMarker(localX);
}

void drawThresholdMarker(int localX) {
  int y = signalToGraphY(PULSE_THRESHOLD);

  if (localX % 6 == 0) {
    tft.drawPixel(GRAPH_X + localX, y, COLOR_SCREEN_BEAT);
  }
}

int signalToGraphY(int signal) {
  if (minSignal == maxSignal) {
    return GRAPH_Y + GRAPH_H / 2;
  }

  int y = map(signal, minSignal, maxSignal, GRAPH_Y + GRAPH_H - 8, GRAPH_Y + 8);
  return constrain(y, GRAPH_Y + 8, GRAPH_Y + GRAPH_H - 8);
}

// ===== LIVE GRAPH =====

void drawWaveform() {
  if (millis() - lastGraphDraw < 20) return;
  lastGraphDraw = millis();

  int y = signalToGraphY(currentSignal);

  drawGraphColumnBackground(graphX);
  drawGraphColumnBackground((graphX + 1) % GRAPH_W);
  drawGraphColumnBackground((graphX + 2) % GRAPH_W);

  uint16_t waveColor = liveTraceColor();

  if (graphX > 0) {
    tft.drawLine(GRAPH_X + graphX - 1, lastGraphY, GRAPH_X + graphX, y, waveColor);
    tft.drawPixel(GRAPH_X + graphX, y - 1, waveColor);
    tft.drawPixel(GRAPH_X + graphX, y + 1, waveColor);
  }

  if (ledBrightness > 180) {
    tft.fillCircle(GRAPH_X + graphX, y, 3, COLOR_SCREEN_BEAT);
  }

  lastGraphY = y;
  graphX++;

  if (graphX >= GRAPH_W) {
    graphX = 0;
    lastGraphY = y;
    drawGraphFrame();
  }
}

// ===== DASHBOARD PANELS =====

void drawPanels() {
  drawMetricPanel(8, "BPM", displayBPM, "", lockedSignal);
  drawMetricPanel(118, "IBI", displayIBI, "ms", lockedSignal);
  drawSignalPanel();
}

void drawMetricPanel(int x, const char* label, int value, const char* unit, bool valid) {
  const int w = 102;
  tft.fillRoundRect(x, PANEL_Y, w, PANEL_H, 6, COLOR_PANEL);
  tft.drawRoundRect(x, PANEL_Y, w, PANEL_H, 6, valid ? COLOR_TEAL : COLOR_GRID);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 10, PANEL_Y + 9);
  tft.print(label);

  tft.setTextSize(4);
  tft.setTextColor(valid ? COLOR_TEXT : COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 10, PANEL_Y + 25);

  if (valid) {
    if (strcmp(label, "BPM") == 0) {
      tft.printf("%3d", value);
    } else {
      tft.setTextSize(3);
      tft.printf("%3d", value);
    }
  } else {
    tft.print("--");
  }

  if (valid && unit[0] != '\0') {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
    tft.setCursor(x + 72, PANEL_Y + 45);
    tft.print(unit);
  }
}

void drawSignalPanel() {
  const int x = 228;
  const int w = 84;
  char pinLabel[8];

  snprintf(pinLabel, sizeof(pinLabel), "GPIO%d", PULSE_PIN);

  tft.fillRoundRect(x, PANEL_Y, w, PANEL_H, 6, COLOR_PANEL);
  tft.drawRoundRect(x, PANEL_Y, w, PANEL_H, 6, lockedSignal ? COLOR_SCREEN_BEAT : COLOR_GRID);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 9, PANEL_Y + 8);
  tft.print("SIG ");
  tft.print(pinLabel);

  drawQualitySegments(x + 9, PANEL_Y + 28);
}

void drawQualitySegments(int x, int y) {
  const int segmentW = 4;
  const int segmentGap = 2;
  for (int i = 0; i < SIGNAL_QUALITY_STEPS; i++) {
    uint16_t color = COLOR_GRID;
    if (i < signalQuality) {
      color = i < LOCK_QUALITY_STEPS ? COLOR_AMBER : COLOR_TEAL;
    }
    tft.fillRect(x + i * (segmentW + segmentGap), y, segmentW, 14, color);
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

  const int centerX = HEART_CENTER_X;
  const int centerY = HEART_CENTER_Y;
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
