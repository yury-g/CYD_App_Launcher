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
 * Cross-machine resume helper: tools/flash_current_favorite.py protects the
 * known favorite 0.4.24-front-id flash path for future internal sessions.
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
#define PULSE_DYNAMIC_THRESHOLD_MIN 180
#define PULSE_DYNAMIC_THRESHOLD_MAX 900
#define MIN_QUALIFIED_BPM 40
#define MAX_QUALIFIED_BPM 180
#define MIN_QUALIFIED_IBI 333
#define MAX_QUALIFIED_IBI 1500
#define MIN_QUALIFIED_AMPLITUDE 20
#define SIGNAL_QUALITY_STEPS 12
#define LOCK_QUALITY_STEPS 10
#define LOCK_QUALIFIED_BEATS 4
#define LOCK_GRACE_BAD_BEATS 4
#define LOCK_HOLD_GRACE_MS 4200
#define ACQUISITION_CADENCE_TOLERANCE_PERCENT 35
#define ACQUISITION_CADENCE_MIN_IBI_PERCENT 70
#define PEAK_TO_PEAK_EXPERIMENT 1
#define PEAK_TO_PEAK_MIN_RANGE 70
#define PEAK_TO_PEAK_STRONG_RANGE 135
#define PEAK_TO_PEAK_MIN_AMPLITUDE 10
#define PEAK_TO_PEAK_ACQUIRE_MIN_SCORE 5
#define PEAK_TO_PEAK_LOCKED_MIN_SCORE 4
#define PEAK_TO_PEAK_PRELOCK_CADENCE_MIN_STREAK 1
#define PEAK_TO_PEAK_FIRST_BEAT_SCORE 8
#define PEAK_TO_PEAK_LOCKED_IBI_TOLERANCE_PERCENT 35
#define PEAK_TO_PEAK_LOCKED_MIN_IBI_TOLERANCE_MS 180
#define PEAK_TO_PEAK_LOCKED_MIN_IBI_PERCENT 70
#define PEAK_RECOVERY_IBI_TOLERANCE_PERCENT 28
#define PEAK_RECOVERY_MIN_IBI_TOLERANCE_MS 120
#define PEAK_RECOVERY_MIN_RANGE 80
#define PEAK_RECOVERY_MIN_AMPLITUDE 12
#define LOCKED_CLIPPING_SCORE_LIMIT 100
#define REARM_SIGNAL_RANGE 120
#define REARM_NO_BEAT_MS 1600
#define REARM_COOLDOWN_MS 1600
#define SIGNAL_COACH_FLAT_RANGE 90
#define SIGNAL_COACH_FLAT_AMPLITUDE 12
#define SIGNAL_COACH_STEADY_AMPLITUDE MIN_QUALIFIED_AMPLITUDE
#define SIGNAL_ACQUISITION_MIN_RANGE 40
#define SIGNAL_ACQUISITION_FULL_RANGE 220
#define SIGNAL_ACQUISITION_MAX_SCORE_BEFORE_LOCK 11
#define SIGNAL_MOTION_ARTIFACT_RANGE 980
#define AMPLITUDE_METER_MAX 120

// ===== BEAT TONE SETTINGS =====

#define SPEAKER_BITS 10
#define BEAT_CHIME_STEP_COUNT 4
#define SIGNAL_HARMONY_NOTE_COUNT 8
#define SIGNAL_HARMONY_STEP_COUNT 4
#define APP3_CRAWL_FANFARE_STEP_COUNT 42
#define APP3_CRAWL_FANFARE_LOOP_START_STEP 0
#define APP3_CRAWL_FANFARE_LOOP_MS 15000
#define APP3_ORIGIN_CRAWL_LINE_COUNT 67
#define APP3_CRAWL_FRAME_MS 72
#define APP3_CRAWL_SPEED_MS 52
#define APP3_CRAWL_TEXT_SIZE 2
#define APP3_CRAWL_MIN_TEXT_SIZE 1
#define APP3_CRAWL_HORIZON_Y 8
#define PIN_SCANNER_PIN_COUNT 4
#define PIN_SCANNER_ADC_MAX_VALUE 1023
#define HOT_MOVEMENT_MIN 20
#define PIN_SCANNER_READ_MS 25
#define PIN_SCANNER_DRAW_MS 100
#define HEART_MIN_SIZE 8
#define HEART_MAX_SIZE 15
#define HEART_SPRITE_WIDTH 56
#define HEART_SPRITE_HEIGHT 42
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_START 1
#define SCREEN_ROTATION_DEFAULT 1
#define SCREEN_ROTATION_COUNT 4

// ===== APP SHELL =====

#ifndef APP_VERSION
#define APP_VERSION "0.4.41-snappy-lock"
#endif
#define APP_FIRMWARE_DATE "2026-05-24"
#define APP_BUILD_RAM_USAGE "RAM 7.3%"
#define APP_BUILD_FLASH_USAGE "Flash 28.9%"
#define TOOLBAR_BUTTON_WIDTH 44
#define TOOLBAR_BUTTON_HEIGHT 28
#define APP_BUTTON_WIDTH TOOLBAR_BUTTON_WIDTH
#define APP_BUTTON_HEIGHT TOOLBAR_BUTTON_HEIGHT
#define APP_BUTTON_GAP 2
#define SETTINGS_TEXT_SIZE 2
#define SETTINGS_ROW_H 40
#define SETTINGS_ROW_COUNT 12
#define SETTINGS_SCROLL_BUTTON_W TOOLBAR_BUTTON_WIDTH
#define SETTINGS_SCROLL_BUTTON_H TOOLBAR_BUTTON_HEIGHT
#define GRAPH_GRID_DOT_STEP 12
#define GRAPH_THRESHOLD_DOT_STEP 6
#define WAVEFORM_BEAT_MARKER_COUNT 12
#define WAVEFORM_BEAT_MARKER_RADIUS 7
#define WAVEFORM_CALC_MARKER_RADIUS 4
#define WAVEFORM_TRACE_HALF_THICKNESS 2
#define PLACEHOLDER_STEP_MS 35
#define CONTROL_TOUCH_PAD 8
#ifndef PERF_DIAGNOSTICS
#define PERF_DIAGNOSTICS 0
#endif
#define PERF_DIAGNOSTICS_MS 2000
#ifndef RAW_SIGNAL_DIAGNOSTICS
#define RAW_SIGNAL_DIAGNOSTICS 0
#endif
#define RAW_SIGNAL_DIAGNOSTICS_MS 20
#define CLIPPING_SCORE_DECAY_MS 20

// ===== TOUCH CALIBRATION =====

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800

// ===== SCREEN LAYOUT =====

#define LANDSCAPE_WIDTH 320
#define LANDSCAPE_HEIGHT 240

// ===== COLORS (RGB565) =====

#define COLOR_BG 0x0000
#define COLOR_PANEL 0x0841
#define COLOR_PANEL_DARK 0x0400
#define COLOR_GRID 0x39E7
#define COLOR_GRID_SOFT 0x2945
#define COLOR_GRAPH_GRID_GRAY 0x5AEB
#define COLOR_TEXT 0xFFFF
#define COLOR_MUTED COLOR_TEXT
#define COLOR_LOCK_GREEN 0x07E0
#define COLOR_RED 0xF800
#define COLOR_RED_DARK 0x6000
#define COLOR_SCREEN_BEAT COLOR_RED
#define COLOR_HIGH_VIS_YELLOW 0xFFF2
#define COLOR_AMBER COLOR_HIGH_VIS_YELLOW
#define COLOR_SIGNAL_YELLOW COLOR_HIGH_VIS_YELLOW
#define COLOR_LIGHT_BLUE 0x02F6
#define COLOR_LIGHT_BUTTON_FILL 0xE7DF
#define COLOR_LIGHT_GREEN 0x04A0
#define COLOR_LIGHT_AMBER 0xBC20
#define COLOR_LIGHT_INACTIVE 0x94B2
#define COLOR_LIGHT_NAV_FILL COLOR_LIGHT_GREEN

enum SignalCoachState {
  COACH_SIGNAL_SEARCH,
  COACH_CLIPPED,
  COACH_TOO_FLAT,
  COACH_HOLD_STEADY,
  COACH_GOOD_WAVE,
  COACH_LOCKING,
  COACH_QUALIFIED
};

enum AppId {
  APP_PULSE,
  APP_SETTINGS,
  APP_PIN_SCANNER,
  APP_PLACEHOLDER_1,
  APP_PLACEHOLDER_2,
  APP_COUNT
};

enum DisplayMode {
  DISPLAY_MONO_DARK,
  DISPLAY_MONO_LIGHT,
  DISPLAY_COLOR_DARK,
  DISPLAY_COLOR_LIGHT,
  DISPLAY_MODE_COUNT
};

// ===== GLOBAL OBJECTS =====

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite app3CrawlSprite = TFT_eSprite(&tft);
TFT_eSprite heartSprite = TFT_eSprite(&tft);
SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
PulseSensorPlayground pulseSensor;

const uint16_t BEAT_CHIME_FREQUENCIES[BEAT_CHIME_STEP_COUNT] = {262, 392, 523, 659};
const uint8_t BEAT_CHIME_DUTIES[BEAT_CHIME_STEP_COUNT] = {56, 42, 30, 18};
const uint16_t BEAT_CHIME_DURATIONS_MS[BEAT_CHIME_STEP_COUNT] = {58, 66, 82, 118};
const uint8_t SCREEN_ROTATIONS[SCREEN_ROTATION_COUNT] = {1, 0, 3, 2};

const uint16_t SIGNAL_HARMONY_FREQUENCIES[SIGNAL_HARMONY_NOTE_COUNT] = {
  523, 587, 659, 698, 784, 880, 988, 1175
};
const uint8_t SIGNAL_HARMONY_DUTIES[SIGNAL_HARMONY_STEP_COUNT] = {40, 34, 28, 22};
const uint16_t SIGNAL_HARMONY_DURATIONS_MS[SIGNAL_HARMONY_STEP_COUNT] = {64, 72, 88, 128};

const uint16_t APP3_CRAWL_FANFARE_FREQUENCIES[APP3_CRAWL_FANFARE_STEP_COUNT] = {
  196, 0, 294, 0, 392, 0, 523, 0, 659, 784, 0,
  196, 294, 392, 587, 392, 294, 220, 330, 440, 660, 0,
  196, 247, 330, 247, 392, 294, 494, 392, 330, 587, 0,
  220, 330, 440, 330, 554, 440, 660, 784, 0
};
const uint8_t APP3_CRAWL_FANFARE_DUTIES[APP3_CRAWL_FANFARE_STEP_COUNT] = {
  42, 0, 40, 0, 38, 0, 34, 0, 30, 26, 0,
  22, 18, 20, 18, 18, 16, 20, 18, 16, 14, 0,
  18, 14, 16, 14, 18, 14, 16, 14, 18, 14, 0,
  16, 14, 16, 14, 18, 14, 16, 12, 0
};
const uint16_t APP3_CRAWL_FANFARE_DURATIONS_MS[APP3_CRAWL_FANFARE_STEP_COUNT] = {
  520, 80, 420, 70, 520, 80, 640, 110, 360, 740, 180,
  320, 240, 280, 420, 260, 280, 360, 260, 320, 560, 220,
  240, 240, 360, 260, 360, 240, 520, 240, 360, 520, 240,
  320, 220, 320, 240, 520, 320, 640, 480, 1120
};

const char* const APP3_ORIGIN_CRAWL_LINES[APP3_ORIGIN_CRAWL_LINE_COUNT] = {
  "EPISODE PPG",
  "A TINY SENSOR",
  "FINDS THE BEAT",
  "",
  "From Brooklyn shops",
  "and Parsons classes,",
  "Joel Murphy",
  "and Yury Gitman",
  "built open hardware",
  "heart-rate sensing",
  "for makers.",
  "",
  "World Famous",
  "Electronics began",
  "as a Kickstarter",
  "project in 2012,",
  "then kept making",
  "PulseSensor",
  "and teaching it",
  "in public.",
  "",
  "OSHWA certified:",
  "Pulse Sensor Amped",
  "UID US000075",
  "August 30, 2017",
  "",
  "GitHub repo:",
  "github.com/",
  "WorldFamousElectronics/PulseSensorPlayground",
  "",
  "Repo likes (stars):",
  "249 stars, 207 forks",
  "as of May 24, 2026",
  "",
  "The sensor shines",
  "green light into",
  "capillary tissue",
  "and watches the",
  "returning brightness.",
  "Each pulse wave",
  "nudges the signal.",
  "",
  "Its origin is",
  "delightfully practical:",
  "breadboards,",
  "op-amps, filters,",
  "a phone-style",
  "light sensor,",
  "and a reverse-mount",
  "green LED",
  "made finger placement",
  "better.",
  "",
  "Now the signal",
  "lands here,",
  "on a Cheap Yellow",
  "Display:",
  "open, tiny, alive",
  "with code.",
  "",
  "Send feature requests,",
  "firmware update ideas,",
  "and wild classroom",
  "wishes.",
  "",
  "Thanks for supporting",
  "PulseSensor since 2012."
};

struct ScannerPin {
  const char* label;
  uint8_t pin;
  int value;
  int minValue;
  int maxValue;
  int movement;
};

struct BeatDecision {
  bool qualified;
  bool strictAccepted;
  bool peakToPeakAccepted;
  bool recovered;
  bool accepted;
  const char* acceptReason;
};

ScannerPin scannerPins[] = {
  {"P3  IO35", 35, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"IO22", 22, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"BL  IO21", 21, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"CN1 IO27", 27, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
};

// ===== LIVE SENSOR STATE =====

int currentSignal = 512;
int displayBPM = 0;
int displayIBI = 0;
int pulseAmplitude = 0;
int minSignal = 512;
int maxSignal = 512;
int activePulseThreshold = PULSE_THRESHOLD;

unsigned long lastBeatTime = 0;
unsigned long lastQualifiedBeatTime = 0;
unsigned long lastGraphDraw = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastRawDiagnosticPrint = 0;
unsigned long lastDetectorRearmTime = 0;
unsigned long lastControlTouchTime = 0;
unsigned long lastSignalHarmonyTime = 0;

bool lockedSignal = false;
bool previousLockedSignal = false;
bool pulseSensorReady = false;
bool pinScannerPulsePaused = false;
bool insideBeatWindow = false;
int signalQuality = 0;
int qualifiedBeatStreak = 0;
int unqualifiedBeatStreak = 0;
int peakToPeakScore = 0;
int clippedSampleScore = 0;
int rearmCount = 0;
bool clippingSinceRangeReset = false;
const char* lastLockDropReason = "none";
const char* lastBeatAcceptReason = "none";
bool rawDiagnosticsHeaderPrinted = false;
bool rawDiagnosticsBeatPending = false;
const char* rawDiagnosticsBeatAcceptReason = "none";

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
int lastSignalHarmonyQuality = 0;

bool app3CrawlFanfarePlaying = false;
uint8_t app3CrawlFanfareStep = 0;
unsigned long app3CrawlFanfareNextStepTime = 0;

// ===== GRAPH STATE =====

int graphX = 0;
int lastGraphY = 104;
int waveformBeatMarkerX[WAVEFORM_BEAT_MARKER_COUNT];
int waveformBeatMarkerY[WAVEFORM_BEAT_MARKER_COUNT];
int waveformBeatMarkerRadius[WAVEFORM_BEAT_MARKER_COUNT];
bool waveformBeatMarkerFilled[WAVEFORM_BEAT_MARKER_COUNT];
bool waveformBeatMarkerActive[WAVEFORM_BEAT_MARKER_COUNT];
int waveformBeatMarkerWrite = 0;
bool waveformBeatMarkerPending = false;
bool waveformBeatMarkerPendingAccepted = false;

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
DisplayMode displayMode = DISPLAY_COLOR_DARK;
bool appNeedsRedraw = true;
int appPrevButtonX = 122;
int appNextButtonX = 146;
int appSettingsButtonX = 170;
int appButtonY = 9;

int settingsVolMinusX = 150;
int settingsVolPlusX = 202;
int settingsRotateX = 150;
int settingsDisplayModeX = 150;
int settingsLedX = 150;
int settingsColorRedX = 150;
int settingsColorYellowX = 178;
int settingsColorGreenX = 206;
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
unsigned long app3CrawlStartTime = 0;
unsigned long lastApp3CrawlFrame = 0;
bool app3CrawlSpriteReady = false;
int app3CrawlSpriteW = 0;
int app3CrawlSpriteH = 0;
bool heartSpriteReady = false;
int scannerActiveIndex = -1;
unsigned long lastPinScannerRead = 0;
unsigned long lastPinScannerDraw = 0;

#if PERF_DIAGNOSTICS
unsigned long perfWindowStartMs = 0;
uint32_t perfLoopCount = 0;
uint32_t perfReadCount = 0;
uint32_t perfSignalChangeCount = 0;
uint32_t perfBeatEventCount = 0;
uint32_t perfLastReadStartUs = 0;
uint32_t perfMaxReadGapUs = 0;
uint32_t perfMaxReadUs = 0;
uint32_t perfMaxDrawUs = 0;
uint32_t perfMaxTotalAfterReadUs = 0;
const char* perfMaxDrawLabel = "none";
int perfPreviousSignal = -1;
#endif

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
bool handlePulseReacquireTouch(int16_t x, int16_t y);
bool handleSettingsTouch(int16_t x, int16_t y);
bool handleSettingsScrollTouch(int16_t x, int16_t y);
bool handleSettingsDisplayModeTouch(int16_t x, int16_t y);
bool handlePinScannerTouch(int16_t x, int16_t y);
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
void releaseApp3CrawlSprite();
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
uint16_t scaledApp3CrawlFanfareDuty(uint8_t duty);
void startApp3CrawlFanfare();
void updateApp3CrawlFanfare();
void stopApp3CrawlFanfare();
void playApp3CrawlFanfareStep();
void triggerRearLedPulse(RearLedColor color);
void triggerBeatEffects();
void setupPulseSensor();
void setupPinScanner();
void updateActivePinScannerReading();
void updatePinScannerAdcOwnership();
void pausePulseSensorForPinScanner();
void resumePulseSensorAfterPinScanner();
int detectorThresholdForCurrentRange();
void retunePulseDetectorThreshold();
bool isPinScannerRailed(int value);
bool isPinScannerAdcCapable(uint8_t pin);
const char* pinScannerStatusText(int index);
void readPulseSensor();
void printRawSignalDiagnostics();
#if PERF_DIAGNOSTICS
void notePerfReadStart(uint32_t readStartUs);
void notePerfReadEnd(uint32_t readStartUs, bool signalChanged);
void notePerfBeatEvent();
void notePerfDraw(const char* label, uint32_t drawStartUs);
void maybePrintPerfDiagnostics();
const char* currentAppName();
#endif
bool isQualifiedBeat(int bpm, int ibi, int amplitude, bool wasLocked);
bool isPlausibleBeatTiming(int bpm, int ibi);
bool isAcquisitionCadenceMatch(int ibi);
bool isLockedCadenceMatch(int ibi);
bool isPeakToPeakCadenceMatch(int ibi);
bool isPeakCadenceRecoveryBeat(int bpm, int ibi, int amplitude);
bool signalIsRecentlyClipped();
bool signalRangeIsMotionArtifact();
bool signalLooksCleanForAcquisition();
bool signalLooksCleanForBeat(bool wasLocked);
int peakToPeakScoreForCurrentSignal();
bool isPeakToPeakCandidateBeat(int bpm, int ibi, int amplitude, bool wasLocked);
BeatDecision decideBeat(int bpm, int ibi, int amplitude, bool wasLocked);
void updateClippingScore();
void resetSignalRangeWindow();
int acquisitionScoreForCurrentSignal();
void updateSignalAcquisitionScore();
void dropSignalLock(const char* reason);
int signalCoachState();
const char* signalCoachText();
int amplitudeMeterSegments(int amplitude);
void maybeRearmDetector();
void rearmPulseDetector(const char* reason);
void resetSignalAcquisitionWindow();
void updateSignalRange();
void drawActiveApp();
void drawStaticScreen();
void drawDashboardIfChanged();
void drawHeader();
void drawHeaderVersionIdentity();
void drawAppFrameHeader(const char* title, const char* subtitle, uint16_t bg, uint16_t subtitleColor);
void drawAppNavControls();
void drawAppButton(int x, int y, const char* label, bool active);
void drawSettingsScreen();
bool settingsRowVisible(int rowIndex, int* rowY);
void drawSettingsRow(int rowIndex, int y, const char* label, const char* value);
void drawSettingsControlRow(int rowIndex, int y, const char* label, const char* value);
int settingsTextWidth(const char* text, int textSize);
int settingsValueTextSize(const char* label, const char* value);
bool settingsValueNeedsCompactText(const char* label, const char* value);
int settingsRightAlignedValueX(const char* value, int textSize);
uint16_t settingsRowBackground(int rowIndex);
void drawSettingsButton(int x, int y, int w, const char* label, bool active);
void drawSettingsDisplayModeControl(int x, int y, int w);
void drawSettingsSwatch(int x, int y, uint16_t color, bool active);
void drawSettingsScrollControls();
void drawPlaceholderApp(const char* title, const char* message);
void drawPlaceholderText(const char* message, uint16_t color);
void drawApp3OriginCrawl();
void drawApp4PinScanner();
void drawPinScannerRow(int index, int y, int rowH, bool hot, bool fullRedraw);
bool ensureApp3CrawlSprite(int w, int h);
uint16_t app3Blend565(uint16_t from, uint16_t to, int amount);
void drawApp3CrawlLinePerspective(const char* line, int localY, uint16_t baseColor,
                                  uint16_t bg, int crawlTop, int crawlBottom);
void drawApp3CrawlLinePerspectiveDirect(const char* line, int localY, uint16_t baseColor,
                                        uint16_t bg, int crawlTop, int crawlBottom);
void drawApp3OriginCrawlDirectFallback(uint16_t bg, uint16_t gold, uint16_t dimGold,
                                       int crawlTop, int crawlBottom, int lineHeight,
                                       int baseY);
void drawApp3Starfield();
void drawRotateIcon(int x, int y, int w, int h, uint16_t color, uint16_t bg);
void drawDottedHLine(int x, int y, int w, uint16_t color, int step, int thickness);
void drawGraphFrame();
void refreshWaveformFrameForLockTransition();
void drawGraphLabels();
void drawSignalCoachStatus();
void drawGraphColumnBackground(int localX);
void drawThresholdMarker(int localX);
void drawBeatMarker(int x, int y);
void clearWaveformBeatMarkerAt(int localX);
void addWaveformBeatMarker(int localX, int y, bool accepted);
void redrawWaveformBeatMarkers();
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
void fillHeartShapeSprite(TFT_eSprite& sprite, int centerX, int centerY, int size, uint16_t color);
bool ensureHeartSprite();
void drawCenteredText(const char* text, int x, int y, int w, int textSize, uint16_t color, uint16_t bg);
uint16_t liveTraceColor();
uint16_t blendRed(int brightness);
void cycleDisplayMode();
const char* displayModeName();
uint16_t screenBgColor();
uint16_t panelBgColor();
uint16_t panelDarkColor();
uint16_t gridColor();
uint16_t gridSoftColor();
uint16_t graphGridColor();
uint16_t chromeTextColor();
uint16_t settingsTextColor();
uint16_t settingsValueTextColor();
uint16_t textColor();
uint16_t displayValueTextColor();
uint16_t buttonFillColor(bool active);
uint16_t buttonOutlineColor(bool active);
uint16_t buttonTextColor(bool active);
uint16_t signalSearchColor();
uint16_t signalLockColor();
uint16_t inactiveColor();
bool shouldDrawInactiveQualitySegments();
uint16_t beatColor();
uint16_t liveTraceColorForMode();
uint16_t pinScannerHotColor();
uint16_t pinScannerBarColor(bool hot);
uint16_t pinScannerRailColor();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("CYD one-screen PulseSensor dashboard");

  setupLED();

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  tft.init();
  applyScreenRotation();
  tft.fillScreen(screenBgColor());

  setupSpeaker();
  setupTouch();
  setupPulseSensor();
  drawActiveApp();
}

void loop() {
  if (!pinScannerPulsePaused) {
    readPulseSensor();
  }
#if PERF_DIAGNOSTICS
  uint32_t perfAfterReadStartUs = micros();
#endif
  readTouchControls();
  updateLED();
  updateBeatChime();
  updateSignalHarmony();
  updateApp3CrawlFanfare();

  if (currentApp == APP_PULSE) {
#if PERF_DIAGNOSTICS
    uint32_t drawStartUs = micros();
#endif
    drawBeatHeart();
#if PERF_DIAGNOSTICS
    notePerfDraw("heart", drawStartUs);
    drawStartUs = micros();
#endif
    drawWaveform();
#if PERF_DIAGNOSTICS
    notePerfDraw("wave", drawStartUs);
    drawStartUs = micros();
#endif
    drawDashboardIfChanged();
#if PERF_DIAGNOSTICS
    notePerfDraw("dashboard", drawStartUs);
#endif
  } else if (currentApp == APP_PIN_SCANNER) {
#if PERF_DIAGNOSTICS
    uint32_t drawStartUs = micros();
#endif
    updateActivePinScannerReading();
    drawApp4PinScanner();
#if PERF_DIAGNOSTICS
    notePerfDraw("scanner", drawStartUs);
#endif
  } else if (currentApp == APP_PLACEHOLDER_1) {
#if PERF_DIAGNOSTICS
    uint32_t drawStartUs = micros();
#endif
    drawPlaceholderApp("Your App Here", "your app here");
#if PERF_DIAGNOSTICS
    notePerfDraw("placeholder", drawStartUs);
#endif
  } else if (currentApp == APP_PLACEHOLDER_2) {
#if PERF_DIAGNOSTICS
    uint32_t drawStartUs = micros();
#endif
    drawApp3OriginCrawl();
#if PERF_DIAGNOSTICS
    notePerfDraw("origin", drawStartUs);
#endif
  } else if (appNeedsRedraw) {
#if PERF_DIAGNOSTICS
    uint32_t drawStartUs = micros();
#endif
    drawSettingsScreen();
#if PERF_DIAGNOSTICS
    notePerfDraw("settings", drawStartUs);
#endif
  }

#if PERF_DIAGNOSTICS
  uint32_t totalAfterReadUs = micros() - perfAfterReadStartUs;
  if (totalAfterReadUs > perfMaxTotalAfterReadUs) perfMaxTotalAfterReadUs = totalAfterReadUs;
  maybePrintPerfDiagnostics();
#endif

  printRawSignalDiagnostics();

  if (!RAW_SIGNAL_DIAGNOSTICS && millis() - lastSerialPrint >= 500) {
    lastSerialPrint = millis();
    if (currentApp == APP_PIN_SCANNER) {
      if (scannerActiveIndex < 0) {
        Serial.println("pinScanner=inactive");
      } else {
        Serial.printf("%s=%4d d%4d %s\n",
                      scannerPins[scannerActiveIndex].label,
                      scannerPins[scannerActiveIndex].value,
                      scannerPins[scannerActiveIndex].movement,
                      pinScannerStatusText(scannerActiveIndex));
      }
    } else {
      Serial.printf("signal=%d amp=%d bpm=%d ibi=%d locked=%d quality=%d p2p=%d range=%d clip=%d qStreak=%d badStreak=%d accept=%s drop=%s\n",
                    currentSignal, pulseAmplitude, displayBPM, displayIBI,
                    lockedSignal ? 1 : 0, signalQuality, peakToPeakScore,
                    maxSignal - minSignal, clippedSampleScore,
                    qualifiedBeatStreak, unqualifiedBeatStreak,
                    lastBeatAcceptReason,
                    lastLockDropReason);
    }
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

  if (!lockedSignal) {
    setRearLedColor(REAR_LED_LOCKING);
    return;
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
      (currentApp == APP_PULSE && handlePulseReacquireTouch(x, y)) ||
      (currentApp == APP_SETTINGS && handleSettingsTouch(x, y)) ||
      (currentApp == APP_PIN_SCANNER && handlePinScannerTouch(x, y))) {
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
  int appPrevNextBoundary = midpointBetween(appPrevCenter, appNextCenter);
  int appNextSettingsBoundary = midpointBetween(appNextCenter, appSettingsCenter);

  if (x < appPrevButtonX - CONTROL_TOUCH_PAD || x > appSettingsButtonX + APP_BUTTON_WIDTH + CONTROL_TOUCH_PAD) return false;

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

int buttonCenterX(int x, int size) {
  return x + size / 2;
}

int midpointBetween(int leftCenter, int rightCenter) {
  return (leftCenter + rightCenter) / 2;
}

bool handlePulseReacquireTouch(int16_t x, int16_t y) {
  if (currentApp != APP_PULSE) return false;
  if (y < headerHeight + CONTROL_TOUCH_PAD) return false;

  rearmPulseDetector("manual touch reacquire");
  resetSignalAcquisitionWindow();
  resetDashboardState();
  drawStaticScreen();
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

  if (handleSettingsDisplayModeTouch(x, y)) return true;

  int ledRowY = settingsRowScreenY(5);
  if (ledRowY >= settingsContentTop() && ledRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= ledRowY - CONTROL_TOUCH_PAD && y <= ledRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD &&
      x >= settingsLedX - CONTROL_TOUCH_PAD && x <= settingsLedX + 86 + CONTROL_TOUCH_PAD) {
    beatLedEnabled = !beatLedEnabled;
    if (!beatLedEnabled) setRearLedColor(REAR_LED_OFF);
    appNeedsRedraw = true;
    drawSettingsScreen();
    return true;
  }

  int colorRowY = settingsRowScreenY(6);
  if (colorRowY >= settingsContentTop() && colorRowY + SETTINGS_ROW_H <= settingsContentBottom() &&
      y >= colorRowY - CONTROL_TOUCH_PAD && y <= colorRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD) {
    int redCenter = buttonCenterX(settingsColorRedX, 34);
    int yellowCenter = buttonCenterX(settingsColorYellowX, 34);
    int greenCenter = buttonCenterX(settingsColorGreenX, 34);
    int redYellowBoundary = midpointBetween(redCenter, yellowCenter);
    int yellowGreenBoundary = midpointBetween(yellowCenter, greenCenter);
    if (x < settingsColorRedX - CONTROL_TOUCH_PAD || x > settingsColorGreenX + 34 + CONTROL_TOUCH_PAD) {
      return false;
    }

    if (x <= redYellowBoundary) {
      heartbeatLedColor = REAR_LED_HEARTBEAT;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }
    if (x <= yellowGreenBoundary) {
      heartbeatLedColor = REAR_LED_LOCKING;
      appNeedsRedraw = true;
      drawSettingsScreen();
      return true;
    }

    heartbeatLedColor = RearLedColor{0, 255, 0};
    appNeedsRedraw = true;
    drawSettingsScreen();
    return true;
  }

  return false;
}

bool handleSettingsDisplayModeTouch(int16_t x, int16_t y) {
  int displayRowY = settingsRowScreenY(2);
  if (displayRowY < settingsContentTop() || displayRowY + SETTINGS_ROW_H > settingsContentBottom()) return false;
  if (y < displayRowY - CONTROL_TOUCH_PAD || y > displayRowY + SETTINGS_ROW_H + CONTROL_TOUCH_PAD) return false;
  if (x < settingsDisplayModeX - CONTROL_TOUCH_PAD || x > settingsDisplayModeX + 90 + CONTROL_TOUCH_PAD) return false;

  cycleDisplayMode();
  appNeedsRedraw = true;
  drawSettingsScreen();
  return true;
}

bool handlePinScannerTouch(int16_t x, int16_t y) {
  int contentTop = headerHeight + 8;
  int footerY = screenHeight - 14;
  int rowAreaH = max(1, footerY - contentTop - 4);
  int rowH = max(24, rowAreaH / PIN_SCANNER_PIN_COUNT);
  if (y < contentTop || y >= contentTop + rowH * PIN_SCANNER_PIN_COUNT) return false;

  int index = (y - contentTop) / rowH;
  if (index < 0 || index >= PIN_SCANNER_PIN_COUNT) return false;

  scannerActiveIndex = scannerActiveIndex == index ? -1 : index;
  updatePinScannerAdcOwnership();
  if (scannerActiveIndex >= 0) {
    scannerPins[scannerActiveIndex].value = 0;
    scannerPins[scannerActiveIndex].minValue = PIN_SCANNER_ADC_MAX_VALUE;
    scannerPins[scannerActiveIndex].maxValue = 0;
    scannerPins[scannerActiveIndex].movement = 0;
  }
  appNeedsRedraw = true;
  drawApp4PinScanner();
  return true;
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
  bool enteringApp3 = currentApp != APP_PLACEHOLDER_2 && app == APP_PLACEHOLDER_2;
  bool leavingApp3 = currentApp == APP_PLACEHOLDER_2 && app != APP_PLACEHOLDER_2;
  bool enteringPinScanner = currentApp != APP_PIN_SCANNER && app == APP_PIN_SCANNER;
  bool leavingPinScanner = currentApp == APP_PIN_SCANNER && app != APP_PIN_SCANNER;
  currentApp = app;
  stopSignalHarmony();
  if (!enteringApp3) stopApp3CrawlFanfare();
  if (leavingApp3) releaseApp3CrawlSprite();
  if (leavingPinScanner) resumePulseSensorAfterPinScanner();
  if (enteringPinScanner) setupPinScanner();
  dashboardDrawn = false;
  appNeedsRedraw = true;
  if (enteringSettings) settingsScrollY = 0;
  resetPlaceholderState();
  drawActiveApp();
  if (enteringApp3) startApp3CrawlFanfare();
}

void nextApp() {
  uint8_t next = (uint8_t)currentApp + 1;
  if (next > APP_PLACEHOLDER_2) next = APP_PULSE;
  switchApp((AppId)next);
}

void previousApp() {
  uint8_t previous = (uint8_t)currentApp;
  previous = previous == APP_PULSE ? APP_PLACEHOLDER_2 : previous - 1;
  switchApp((AppId)previous);
}

void rotateScreen() {
  screenRotationIndex = (screenRotationIndex + 1) % SCREEN_ROTATION_COUNT;
  screenRotation = SCREEN_ROTATIONS[screenRotationIndex];
  Serial.printf("screenRotation=%u\n", screenRotation);
  releaseApp3CrawlSprite();
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

  if (portraitLayout) {
    headerHeight = 74;
    heartCenterX = screenWidth - 32;
    heartCenterY = 54;

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

    appSettingsButtonX = screenWidth - APP_BUTTON_WIDTH - 4;
    appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appButtonY = 4;
  } else {
    headerHeight = 42;
    int brandEndX = 10 + 15 * 6;
    int navLeftX = screenWidth - (APP_BUTTON_WIDTH * 3) - (APP_BUTTON_GAP * 2) - 4;
    heartCenterX = (brandEndX + navLeftX) / 2;
    heartCenterY = 20;

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

    appSettingsButtonX = screenWidth - APP_BUTTON_WIDTH - 4;
    appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;
    appButtonY = 7;
  }

  settingsVolMinusX = screenWidth - (TOOLBAR_BUTTON_WIDTH * 2) - APP_BUTTON_GAP - 4;
  settingsVolPlusX = settingsVolMinusX + TOOLBAR_BUTTON_WIDTH + APP_BUTTON_GAP;
  settingsRotateX = screenWidth - 90;
  settingsDisplayModeX = settingsRotateX;
  settingsLedX = settingsRotateX;
  settingsColorRedX = screenWidth - 118;
  settingsColorYellowX = settingsColorRedX + 40;
  settingsColorGreenX = settingsColorYellowX + 40;
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
  app3CrawlStartTime = millis();
  lastApp3CrawlFrame = 0;
}

void releaseApp3CrawlSprite() {
  if (!app3CrawlSpriteReady) return;
  app3CrawlSprite.deleteSprite();
  app3CrawlSpriteReady = false;
  app3CrawlSpriteW = 0;
  app3CrawlSpriteH = 0;
}

uint16_t scaledChimeDuty(uint8_t step) {
  step = min<uint8_t>(step, BEAT_CHIME_STEP_COUNT - 1);
  return (BEAT_CHIME_DUTIES[step] * speakerVolume) / VOLUME_MAX;
}

void startBeatChime() {
  stopSignalHarmony();
  stopApp3CrawlFanfare();
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

  uint8_t maxBaseNote = SIGNAL_HARMONY_NOTE_COUNT - SIGNAL_HARMONY_STEP_COUNT;
  signalHarmonyBaseNote = constrain(map(quality, 1, SIGNAL_QUALITY_STEPS, 0, maxBaseNote), 0, maxBaseNote);
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
  if (signalHarmonyStep < SIGNAL_HARMONY_STEP_COUNT) {
    uint8_t note = min<uint8_t>(signalHarmonyBaseNote + signalHarmonyStep, SIGNAL_HARMONY_NOTE_COUNT - 1);
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

uint16_t scaledApp3CrawlFanfareDuty(uint8_t duty) {
  if (speakerVolume == 0 || duty == 0) return 0;
  return max<uint16_t>((duty * speakerVolume) / VOLUME_MAX, 3);
}

void startApp3CrawlFanfare() {
  if (speakerVolume == 0) return;
  if (beatTonePlaying) {
    beatTonePlaying = false;
  }
  stopSignalHarmony();
  app3CrawlFanfarePlaying = true;
  app3CrawlFanfareStep = 0;
  playApp3CrawlFanfareStep();
}

void updateApp3CrawlFanfare() {
  if (!app3CrawlFanfarePlaying || currentApp != APP_PLACEHOLDER_2) return;
  if ((long)(millis() - app3CrawlFanfareNextStepTime) < 0) return;

  app3CrawlFanfareStep++;
  if (app3CrawlFanfareStep >= APP3_CRAWL_FANFARE_STEP_COUNT) {
    app3CrawlFanfareStep = APP3_CRAWL_FANFARE_LOOP_START_STEP;
  }
  playApp3CrawlFanfareStep();
}

void stopApp3CrawlFanfare() {
  if (!app3CrawlFanfarePlaying) return;
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  app3CrawlFanfarePlaying = false;
}

void playApp3CrawlFanfareStep() {
  uint16_t frequency = APP3_CRAWL_FANFARE_FREQUENCIES[app3CrawlFanfareStep];
  uint16_t duty = scaledApp3CrawlFanfareDuty(APP3_CRAWL_FANFARE_DUTIES[app3CrawlFanfareStep]);

  if (frequency == 0 || duty == 0) {
    cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
    cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  } else {
    cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, frequency);
    cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, duty);
  }
  app3CrawlFanfareNextStepTime = millis() + APP3_CRAWL_FANFARE_DURATIONS_MS[app3CrawlFanfareStep];
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
  activePulseThreshold = PULSE_THRESHOLD;
  pulseSensor.setThreshold(activePulseThreshold);
  pulseSensorReady = pulseSensor.begin();

  if (!pulseSensorReady) {
    Serial.println("PulseSensor initialization failed");
  }
}

int detectorThresholdForCurrentRange() {
  int low = constrain(minSignal, 0, 1023);
  int high = constrain(maxSignal, 0, 1023);
  if (high < low) {
    int swapValue = high;
    high = low;
    low = swapValue;
  }

  if (high - low >= SIGNAL_ACQUISITION_MIN_RANGE) {
    return constrain((low + high) / 2, PULSE_DYNAMIC_THRESHOLD_MIN, PULSE_DYNAMIC_THRESHOLD_MAX);
  }

  return PULSE_THRESHOLD;
}

void retunePulseDetectorThreshold() {
  activePulseThreshold = detectorThresholdForCurrentRange();
  pulseSensor.setThreshold(activePulseThreshold);
}

void setupPinScanner() {
  for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
    if (isPinScannerAdcCapable(scannerPins[i].pin)) pinMode(scannerPins[i].pin, INPUT);
    scannerPins[i].value = 0;
    scannerPins[i].minValue = PIN_SCANNER_ADC_MAX_VALUE;
    scannerPins[i].maxValue = 0;
    scannerPins[i].movement = 0;
  }

  scannerActiveIndex = -1;
  lastPinScannerRead = 0;
  lastPinScannerDraw = 0;
  updatePinScannerAdcOwnership();
}

void updateActivePinScannerReading() {
  if (currentApp != APP_PIN_SCANNER) return;
  if (scannerActiveIndex < 0 || scannerActiveIndex >= PIN_SCANNER_PIN_COUNT) return;
  if (!isPinScannerAdcCapable(scannerPins[scannerActiveIndex].pin)) return;
  if (millis() - lastPinScannerRead < PIN_SCANNER_READ_MS) return;
  lastPinScannerRead = millis();

  int value = analogRead(scannerPins[scannerActiveIndex].pin);
  scannerPins[scannerActiveIndex].value = value;
  if (value < scannerPins[scannerActiveIndex].minValue) scannerPins[scannerActiveIndex].minValue = value;
  if (value > scannerPins[scannerActiveIndex].maxValue) scannerPins[scannerActiveIndex].maxValue = value;
  scannerPins[scannerActiveIndex].movement =
      scannerPins[scannerActiveIndex].maxValue - scannerPins[scannerActiveIndex].minValue;

  if (scannerPins[scannerActiveIndex].minValue < value) scannerPins[scannerActiveIndex].minValue++;
  if (scannerPins[scannerActiveIndex].maxValue > value) scannerPins[scannerActiveIndex].maxValue--;
}

void updatePinScannerAdcOwnership() {
  bool scannerNeedsAdc = currentApp == APP_PIN_SCANNER &&
                         scannerActiveIndex >= 0 &&
                         scannerActiveIndex < PIN_SCANNER_PIN_COUNT &&
                         isPinScannerAdcCapable(scannerPins[scannerActiveIndex].pin);
  if (scannerNeedsAdc) {
    pausePulseSensorForPinScanner();
  } else {
    resumePulseSensorAfterPinScanner();
  }
}

void pausePulseSensorForPinScanner() {
  if (pinScannerPulsePaused) return;
  if (pulseSensorReady) {
    pulseSensor.pause();
  }
  pinScannerPulsePaused = true;
  beatTonePlaying = false;
  stopSignalHarmony();
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
}

void resumePulseSensorAfterPinScanner() {
  if (!pinScannerPulsePaused) return;
  if (pulseSensorReady) {
    pulseSensor.resume();
  }
  pinScannerPulsePaused = false;
  scannerActiveIndex = -1;
  displayBPM = 0;
  displayIBI = 0;
  lockedSignal = false;
  qualifiedBeatStreak = 0;
  unqualifiedBeatStreak = 0;
  lastBeatAcceptReason = "none";
  unsigned long now = millis();
  lastBeatTime = now;
  lastQualifiedBeatTime = now;
  resetSignalAcquisitionWindow();
  resetDashboardState();
}

bool isPinScannerRailed(int value) {
  return value < 8 || value > PIN_SCANNER_ADC_MAX_VALUE - 8;
}

bool isPinScannerAdcCapable(uint8_t pin) {
  return pin == 35 || pin == 27;
}

const char* pinScannerStatusText(int index) {
  if (index < 0 || index >= PIN_SCANNER_PIN_COUNT) return "tap";
  if (scannerActiveIndex != index) return "tap";
  if (scannerPins[index].pin == 21) return "backlight";
  if (!isPinScannerAdcCapable(scannerPins[index].pin)) return "not ADC";
  return scannerPins[index].movement > HOT_MOVEMENT_MIN ? "hot" : "active";
}

// ===== SENSOR AND BEAT LOGIC =====

void readPulseSensor() {
#if PERF_DIAGNOSTICS
  uint32_t readStartUs = micros();
#endif
  currentSignal = pulseSensor.getLatestSample();
#if PERF_DIAGNOSTICS
  bool signalChanged = currentSignal != perfPreviousSignal;
  perfPreviousSignal = currentSignal;
#endif
  pulseAmplitude = pulseSensor.getPulseAmplitude();
  insideBeatWindow = pulseSensor.isInsideBeat();
  updateClippingScore();
  updateSignalRange();
  peakToPeakScore = peakToPeakScoreForCurrentSignal();
  maybeRearmDetector();
  updateSignalAcquisitionScore();

  if (pulseSensor.sawStartOfBeat()) {
#if PERF_DIAGNOSTICS
    notePerfBeatEvent();
#endif
    unsigned long now = millis();
    bool wasLocked = lockedSignal;
    int bpm = pulseSensor.getBeatsPerMinute();
    int ibi = pulseSensor.getInterBeatIntervalMs();
    BeatDecision decision = decideBeat(bpm, ibi, pulseAmplitude, wasLocked);

    lastBeatTime = now;

    if (decision.accepted) {
      displayBPM = bpm;
      displayIBI = ibi;
      lastQualifiedBeatTime = now;
      unqualifiedBeatStreak = 0;
      lastLockDropReason = "none";
      lastBeatAcceptReason = decision.acceptReason;
      qualifiedBeatStreak++;
      if (qualifiedBeatStreak > LOCK_QUALIFIED_BEATS) qualifiedBeatStreak = LOCK_QUALIFIED_BEATS;
    } else if (wasLocked) {
      lastBeatAcceptReason = "reject";
      unqualifiedBeatStreak++;
      if (wasLocked && unqualifiedBeatStreak <= LOCK_GRACE_BAD_BEATS &&
          now - lastQualifiedBeatTime <= LOCK_HOLD_GRACE_MS) {
        qualifiedBeatStreak = LOCK_QUALIFIED_BEATS;
      } else {
        dropSignalLock("grace expired");
      }
    } else {
      lastBeatAcceptReason = "reject";
      qualifiedBeatStreak = 0;
      unqualifiedBeatStreak = 0;
    }

    lockedSignal = qualifiedBeatStreak >= LOCK_QUALIFIED_BEATS;
    updateSignalAcquisitionScore();
    rawDiagnosticsBeatPending = true;
    rawDiagnosticsBeatAcceptReason = lastBeatAcceptReason;
    waveformBeatMarkerPending = true;
    waveformBeatMarkerPendingAccepted = decision.accepted;

    if (decision.accepted) {
      if (lockedSignal) {
        triggerBeatEffects();
      } else {
        triggerRearLedPulse(REAR_LED_LOCKING);
      }
    }
  }

  unsigned long now = millis();
  if (lockedSignal && now - lastQualifiedBeatTime > LOCK_HOLD_GRACE_MS) {
    dropSignalLock("grace expired");
  }

  if (now - lastQualifiedBeatTime > NO_BEAT_TIMEOUT) {
    dropSignalLock("no beat timeout");
  }

#if PERF_DIAGNOSTICS
  notePerfReadEnd(readStartUs, signalChanged);
#endif
}

void printRawSignalDiagnostics() {
  if (!RAW_SIGNAL_DIAGNOSTICS) return;

  unsigned long now = millis();
  if (!rawDiagnosticsHeaderPrinted) {
    Serial.println("rawDiag,ms,signal,amp,bpm,ibi,locked,quality,p2p,range,clip,inside,beat,accept,drop,qStreak,badStreak");
    rawDiagnosticsHeaderPrinted = true;
  }

  if (now - lastRawDiagnosticPrint < RAW_SIGNAL_DIAGNOSTICS_MS && !rawDiagnosticsBeatPending) return;
  lastRawDiagnosticPrint = now;

  int beat = rawDiagnosticsBeatPending ? 1 : 0;
  const char* acceptReason = rawDiagnosticsBeatPending ? rawDiagnosticsBeatAcceptReason : "none";
  Serial.printf("rawDiag,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%d,%d\n",
                now,
                currentSignal,
                pulseAmplitude,
                displayBPM,
                displayIBI,
                lockedSignal ? 1 : 0,
                signalQuality,
                peakToPeakScore,
                maxSignal - minSignal,
                clippedSampleScore,
                insideBeatWindow ? 1 : 0,
                beat,
                acceptReason,
                lastLockDropReason,
                qualifiedBeatStreak,
                unqualifiedBeatStreak);

  rawDiagnosticsBeatPending = false;
  rawDiagnosticsBeatAcceptReason = "none";
}

#if PERF_DIAGNOSTICS
void notePerfReadStart(uint32_t readStartUs) {
  if (perfLastReadStartUs > 0) {
    uint32_t gapUs = readStartUs - perfLastReadStartUs;
    if (gapUs > perfMaxReadGapUs) perfMaxReadGapUs = gapUs;
  }
  perfLastReadStartUs = readStartUs;
}

void notePerfReadEnd(uint32_t readStartUs, bool signalChanged) {
  notePerfReadStart(readStartUs);
  uint32_t readUs = micros() - readStartUs;
  if (readUs > perfMaxReadUs) perfMaxReadUs = readUs;
  perfReadCount++;
  perfLoopCount++;
  if (signalChanged) perfSignalChangeCount++;
}

void notePerfBeatEvent() {
  perfBeatEventCount++;
}

void notePerfDraw(const char* label, uint32_t drawStartUs) {
  uint32_t drawUs = micros() - drawStartUs;
  if (drawUs > perfMaxDrawUs) {
    perfMaxDrawUs = drawUs;
    perfMaxDrawLabel = label;
  }
}

void maybePrintPerfDiagnostics() {
  unsigned long now = millis();
  if (perfWindowStartMs == 0) {
    perfWindowStartMs = now;
    return;
  }
  if (now - perfWindowStartMs < PERF_DIAGNOSTICS_MS) return;

  unsigned long elapsedMs = now - perfWindowStartMs;
  uint32_t loopsPerSecond = (perfLoopCount * 1000UL) / elapsedMs;
  uint32_t readsPerSecond = (perfReadCount * 1000UL) / elapsedMs;
  uint32_t changesPerSecond = (perfSignalChangeCount * 1000UL) / elapsedMs;

  Serial.printf("perf app=%s loops/s=%lu reads/s=%lu changed/s=%lu maxReadGapUs=%lu maxReadUs=%lu maxAfterReadUs=%lu maxDraw=%s:%lu beats=%lu\n",
                currentAppName(),
                (unsigned long)loopsPerSecond,
                (unsigned long)readsPerSecond,
                (unsigned long)changesPerSecond,
                (unsigned long)perfMaxReadGapUs,
                (unsigned long)perfMaxReadUs,
                (unsigned long)perfMaxTotalAfterReadUs,
                perfMaxDrawLabel,
                (unsigned long)perfMaxDrawUs,
                (unsigned long)perfBeatEventCount);

  perfWindowStartMs = now;
  perfLoopCount = 0;
  perfReadCount = 0;
  perfSignalChangeCount = 0;
  perfBeatEventCount = 0;
  perfMaxReadGapUs = 0;
  perfMaxReadUs = 0;
  perfMaxDrawUs = 0;
  perfMaxTotalAfterReadUs = 0;
  perfMaxDrawLabel = "none";
}

const char* currentAppName() {
  switch (currentApp) {
    case APP_PULSE:
      return "Pulse";
    case APP_SETTINGS:
      return "Settings";
    case APP_PIN_SCANNER:
      return "PinScanner";
    case APP_PLACEHOLDER_1:
      return "YourApp";
    case APP_PLACEHOLDER_2:
      return "Origin";
    default:
      return "Unknown";
  }
}
#endif

bool isQualifiedBeat(int bpm, int ibi, int amplitude, bool wasLocked) {
  if (!isPlausibleBeatTiming(bpm, ibi)) return false;
  if (amplitude < MIN_QUALIFIED_AMPLITUDE) return false;
  if (maxSignal - minSignal < SIGNAL_COACH_FLAT_RANGE) return false;
  if (!signalLooksCleanForBeat(wasLocked)) return false;
  return true;
}

bool isPlausibleBeatTiming(int bpm, int ibi) {
  if (bpm < MIN_QUALIFIED_BPM || bpm > MAX_QUALIFIED_BPM) return false;
  if (ibi < MIN_QUALIFIED_IBI || ibi > MAX_QUALIFIED_IBI) return false;
  return true;
}

bool isAcquisitionCadenceMatch(int ibi) {
  if (qualifiedBeatStreak <= 0 || displayIBI <= 0) return true;
  if (ibi < (displayIBI * ACQUISITION_CADENCE_MIN_IBI_PERCENT) / 100) return false;
  int ibiTolerance = max(PEAK_RECOVERY_MIN_IBI_TOLERANCE_MS,
                         (displayIBI * ACQUISITION_CADENCE_TOLERANCE_PERCENT) / 100);
  return abs(ibi - displayIBI) <= ibiTolerance;
}

bool isLockedCadenceMatch(int ibi) {
  if (displayIBI <= 0) return false;
  int ibiTolerance = max(PEAK_RECOVERY_MIN_IBI_TOLERANCE_MS,
                         (displayIBI * PEAK_RECOVERY_IBI_TOLERANCE_PERCENT) / 100);
  return abs(ibi - displayIBI) <= ibiTolerance;
}

bool isPeakToPeakCadenceMatch(int ibi) {
  if (displayIBI <= 0) return false;
  if (ibi < (displayIBI * PEAK_TO_PEAK_LOCKED_MIN_IBI_PERCENT) / 100) return false;
  int ibiTolerance = max(PEAK_TO_PEAK_LOCKED_MIN_IBI_TOLERANCE_MS,
                         (displayIBI * PEAK_TO_PEAK_LOCKED_IBI_TOLERANCE_PERCENT) / 100);
  return abs(ibi - displayIBI) <= ibiTolerance;
}

bool isPeakCadenceRecoveryBeat(int bpm, int ibi, int amplitude) {
  if (!lockedSignal) return false;
  if (!isPlausibleBeatTiming(bpm, ibi)) return false;
  if (!isLockedCadenceMatch(ibi)) return false;

  int liveRange = maxSignal - minSignal;
  // sawStartOfBeat() is already the peak-side evidence; do not require the
  // fixed startup threshold here because post-lock movement can shift baseline.
  bool signalStillMoving = liveRange >= PEAK_RECOVERY_MIN_RANGE ||
                           amplitude >= PEAK_RECOVERY_MIN_AMPLITUDE;
  if (!signalStillMoving) return false;

  if (!signalLooksCleanForBeat(true)) return false;
  return true;
}

bool signalIsRecentlyClipped() {
  return clippedSampleScore > 18;
}

bool signalRangeIsMotionArtifact() {
  return maxSignal - minSignal > SIGNAL_MOTION_ARTIFACT_RANGE;
}

bool signalLooksCleanForAcquisition() {
  return !signalIsRecentlyClipped() && !signalRangeIsMotionArtifact();
}

bool signalLooksCleanForBeat(bool wasLocked) {
  if (signalRangeIsMotionArtifact()) return false;
  if (!wasLocked) return !signalIsRecentlyClipped();
  return clippedSampleScore <= LOCKED_CLIPPING_SCORE_LIMIT;
}

int peakToPeakScoreForCurrentSignal() {
  if (!signalLooksCleanForAcquisition()) return 0;

  int liveRange = maxSignal - minSignal;
  int rangeScore = map(constrain(liveRange,
                                 PEAK_TO_PEAK_MIN_RANGE,
                                 PEAK_TO_PEAK_STRONG_RANGE),
                       PEAK_TO_PEAK_MIN_RANGE,
                       PEAK_TO_PEAK_STRONG_RANGE,
                       0,
                       4);
  int amplitudeScore = map(constrain(pulseAmplitude,
                                     PEAK_TO_PEAK_MIN_AMPLITUDE,
                                     AMPLITUDE_METER_MAX),
                           PEAK_TO_PEAK_MIN_AMPLITUDE,
                           AMPLITUDE_METER_MAX,
                           0,
                           3);
  int cleanScore = clippedSampleScore <= 4 ? 2 : (clippedSampleScore <= 18 ? 1 : 0);
  int beatWindowScore = insideBeatWindow ? 1 : 0;
  int score = rangeScore + amplitudeScore + cleanScore + beatWindowScore;

  if (liveRange < PEAK_TO_PEAK_MIN_RANGE && pulseAmplitude < PEAK_TO_PEAK_MIN_AMPLITUDE) {
    score = 0;
  }

  return constrain(score, 0, 10);
}

bool isPeakToPeakCandidateBeat(int bpm, int ibi, int amplitude, bool wasLocked) {
  if (!isPlausibleBeatTiming(bpm, ibi)) return false;
  if (!signalLooksCleanForBeat(wasLocked)) return false;

  int requiredScore = wasLocked ? PEAK_TO_PEAK_LOCKED_MIN_SCORE : PEAK_TO_PEAK_ACQUIRE_MIN_SCORE;
  if (peakToPeakScore < requiredScore) return false;

  if (wasLocked) return isPeakToPeakCadenceMatch(ibi);
  if (qualifiedBeatStreak < PEAK_TO_PEAK_PRELOCK_CADENCE_MIN_STREAK &&
      peakToPeakScore < PEAK_TO_PEAK_FIRST_BEAT_SCORE) {
    return false;
  }
  if (!isAcquisitionCadenceMatch(ibi)) return false;
  return amplitude >= PEAK_TO_PEAK_MIN_AMPLITUDE || (maxSignal - minSignal) >= PEAK_TO_PEAK_MIN_RANGE;
}

BeatDecision decideBeat(int bpm, int ibi, int amplitude, bool wasLocked) {
  BeatDecision decision;
  decision.qualified = isQualifiedBeat(bpm, ibi, amplitude, wasLocked);
  decision.strictAccepted = decision.qualified;
  decision.peakToPeakAccepted = PEAK_TO_PEAK_EXPERIMENT &&
                                !decision.strictAccepted &&
                                isPeakToPeakCandidateBeat(bpm, ibi, amplitude, wasLocked);
  decision.recovered = !decision.strictAccepted &&
                       !decision.peakToPeakAccepted &&
                       wasLocked &&
                       isPeakCadenceRecoveryBeat(bpm, ibi, amplitude);
  decision.accepted = decision.strictAccepted || decision.peakToPeakAccepted || decision.recovered;
  decision.acceptReason = decision.accepted ?
                          (decision.strictAccepted ? "strict" :
                           (decision.peakToPeakAccepted ? "peak2peak" : "peak-cadence")) :
                          "reject";
  return decision;
}

void updateClippingScore() {
  static unsigned long lastClipDecayMs = 0;
  unsigned long now = millis();
  bool clipped = currentSignal <= 8 || currentSignal >= 1015;

  if (clipped) {
    clippingSinceRangeReset = true;
    clippedSampleScore += 8;
    if (clippedSampleScore > 100) clippedSampleScore = 100;
    lastClipDecayMs = now;
    return;
  }

  if (clippedSampleScore > 0 && now - lastClipDecayMs >= CLIPPING_SCORE_DECAY_MS) {
    clippedSampleScore--;
    lastClipDecayMs = now;
    if (clippedSampleScore == 0 && clippingSinceRangeReset) {
      resetSignalRangeWindow();
    }
  }
}

int acquisitionScoreForCurrentSignal() {
  if (lockedSignal) return SIGNAL_QUALITY_STEPS;
  if (!signalLooksCleanForAcquisition()) return 0;

  int liveRange = maxSignal - minSignal;
  int rangeScore = map(constrain(liveRange,
                                 SIGNAL_ACQUISITION_MIN_RANGE,
                                 SIGNAL_ACQUISITION_FULL_RANGE),
                       SIGNAL_ACQUISITION_MIN_RANGE,
                       SIGNAL_ACQUISITION_FULL_RANGE,
                       0,
                       5);
  int amplitudeScore = map(constrain(pulseAmplitude,
                                     SIGNAL_COACH_FLAT_AMPLITUDE,
                                     AMPLITUDE_METER_MAX),
                           SIGNAL_COACH_FLAT_AMPLITUDE,
                           AMPLITUDE_METER_MAX,
                           0,
                           4);
  int cleanScore = clippedSampleScore <= 4 ? 2 : (clippedSampleScore <= 18 ? 1 : 0);
  int beatWindowScore = insideBeatWindow ? 1 : 0;
  int streakScore = qualifiedBeatStreak * 2;
  int peakScore = PEAK_TO_PEAK_EXPERIMENT ? min(2, peakToPeakScore / 3) : 0;
  int score = rangeScore + amplitudeScore + cleanScore + beatWindowScore + streakScore + peakScore;

  if (liveRange < SIGNAL_ACQUISITION_MIN_RANGE && pulseAmplitude < SIGNAL_COACH_FLAT_AMPLITUDE) {
    score = min(score, 2);
  }

  return constrain(score, 0, SIGNAL_ACQUISITION_MAX_SCORE_BEFORE_LOCK);
}

void updateSignalAcquisitionScore() {
  int previousQuality = signalQuality;
  signalQuality = acquisitionScoreForCurrentSignal();

  if (signalQuality <= 1) {
    lastSignalHarmonyQuality = 0;
  }

  if (!lockedSignal &&
      signalQuality > previousQuality &&
      signalQuality > lastSignalHarmonyQuality) {
    startSignalHarmony(signalQuality);
    lastSignalHarmonyQuality = signalQuality;
  }
}

void dropSignalLock(const char* reason) {
  bool hadSignalState = lockedSignal ||
                        displayBPM > 0 ||
                        displayIBI > 0 ||
                        qualifiedBeatStreak > 0 ||
                        unqualifiedBeatStreak > 0;
  if (hadSignalState) {
    lastLockDropReason = reason;
  }

  lockedSignal = false;
  qualifiedBeatStreak = 0;
  unqualifiedBeatStreak = 0;
  lastBeatAcceptReason = "none";
  displayBPM = 0;
  displayIBI = 0;
  updateSignalAcquisitionScore();
}

int signalCoachState() {
  int liveRange = maxSignal - minSignal;

  if (lockedSignal) return COACH_QUALIFIED;
  if (!signalLooksCleanForAcquisition()) return COACH_CLIPPED;
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
    case COACH_CLIPPED:
      return "ADJUST SENSOR";
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
  bool signalLooksAlive = liveRange >= REARM_SIGNAL_RANGE && !signalIsRecentlyClipped();
  bool detectorIsQuiet = (now - lastBeatTime) >= REARM_NO_BEAT_MS;
  bool rearmCooledDown = (now - lastDetectorRearmTime) >= REARM_COOLDOWN_MS;

  if (!lockedSignal && signalLooksAlive && detectorIsQuiet && rearmCooledDown) {
    rearmPulseDetector("alive signal without beat event");
    resetSignalAcquisitionWindow();
  }
}

void rearmPulseDetector(const char* reason) {
  Serial.print("Re-arming PulseSensor detector: ");
  Serial.println(reason);

  retunePulseDetectorThreshold();
  pulseSensor.pause();
  delay(8);
  pulseSensor.resume();

  lastDetectorRearmTime = millis();
  lastBeatTime = millis();
  lastQualifiedBeatTime = millis();
  signalQuality = 0;
  lastSignalHarmonyQuality = 0;
  qualifiedBeatStreak = 0;
  unqualifiedBeatStreak = 0;
  peakToPeakScore = 0;
  displayBPM = 0;
  displayIBI = 0;
  lockedSignal = false;
  lastLockDropReason = reason;
  lastBeatAcceptReason = "none";
  rearmCount++;
}

void resetSignalAcquisitionWindow() {
  beatTonePlaying = false;
  stopSignalHarmony();
  cydLedcWrite(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  cydLedcWriteTone(SPEAKER_PIN, SPEAKER_PWM_CH, 0);
  resetSignalRangeWindow();
  clippedSampleScore = 0;
  peakToPeakScore = 0;
  insideBeatWindow = false;
  rearLedPulseActive = false;
  ledPulseActive = false;
  rearLedBrightness = 0;
  ledBrightness = 0;
  setRearLedColor(REAR_LED_OFF);
}

void resetSignalRangeWindow() {
  minSignal = currentSignal - 40;
  maxSignal = currentSignal + 40;
  clippingSinceRangeReset = false;
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
  } else if (currentApp == APP_PIN_SCANNER) {
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawApp4PinScanner();
  } else if (currentApp == APP_PLACEHOLDER_1) {
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawPlaceholderApp("Your App Here", "your app here");
  } else if (currentApp == APP_PLACEHOLDER_2) {
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawApp3OriginCrawl();
  }
}

void drawStaticScreen() {
  tft.fillScreen(screenBgColor());
  drawHeader();
  graphX = 0;
  lastGraphY = signalToGraphY(currentSignal);
  for (int i = 0; i < WAVEFORM_BEAT_MARKER_COUNT; i++) {
    waveformBeatMarkerActive[i] = false;
  }
  waveformBeatMarkerPending = false;
  drawGraphFrame();
  drawDashboardIfChanged();
}

void drawDashboardIfChanged() {
  int coach = signalCoachState();
  bool statusChanged = !dashboardDrawn ||
                       lockedSignal != previousDashboardLockedSignal ||
                       coach != previousSignalCoachState;
  bool needsFullPanelRedraw = !dashboardDrawn ||
                              lockedSignal != previousDashboardLockedSignal;
  bool bpmChanged = displayBPM != previousDisplayBPM;
  bool ibiChanged = displayIBI != previousDisplayIBI;
  bool signalPanelChanged = signalQuality != previousSignalQuality;

  if (statusChanged) {
    drawHeader();
    drawSignalCoachStatus();
  }

  if (needsFullPanelRedraw && dashboardDrawn) {
    refreshWaveformFrameForLockTransition();
    lastGraphY = signalToGraphY(currentSignal);
    graphX = 0;
    for (int i = 0; i < WAVEFORM_BEAT_MARKER_COUNT; i++) {
      waveformBeatMarkerActive[i] = false;
    }
  }

  if (needsFullPanelRedraw) {
    drawPanels();
  } else {
    if (bpmChanged) {
      drawMetricPanel(bpmPanelX, bpmPanelY, bpmPanelW, bpmPanelH, "BPM", displayBPM, "", lockedSignal);
    }
    if (ibiChanged) {
      drawMetricPanel(ibiPanelX, ibiPanelY, ibiPanelW, ibiPanelH, "IBI", displayIBI, "ms", lockedSignal);
    }
    if (signalPanelChanged) {
      drawSignalPanel();
    }
  }

  if (statusChanged || needsFullPanelRedraw || bpmChanged || ibiChanged || signalPanelChanged) {
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
  uint16_t bg = screenBgColor();
  tft.fillRect(0, 0, screenWidth, headerHeight, bg);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
  beatHeartNeedsRedraw = true;

  drawHeaderVersionIdentity();
  drawAppNavControls();
}

void drawHeaderVersionIdentity() {
  uint16_t bg = screenBgColor();
  int brandX = 10;
  int brandY = portraitLayout ? 38 : 4;

  tft.setTextSize(1);
  tft.setTextColor(chromeTextColor(), bg);
  tft.setCursor(brandX, brandY);
  tft.print("PulseSensor.com");
  tft.setTextColor(chromeTextColor(), bg);
  tft.setCursor(brandX, brandY + 11);
  tft.print(APP_VERSION);
  tft.setCursor(brandX, brandY + 22);
  tft.print(APP_FIRMWARE_DATE);
}

void drawAppFrameHeader(const char* title, const char* subtitle, uint16_t bg, uint16_t subtitleColor) {
  tft.fillRect(0, 0, screenWidth, headerHeight, bg);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
  tft.setTextSize(1);
  tft.setTextColor(chromeTextColor(), bg);
  tft.setCursor(10, portraitLayout ? 38 : 8);
  tft.print(title);
  if (subtitle != nullptr && subtitle[0] != '\0') {
    tft.setTextColor(chromeTextColor(), bg);
    tft.setCursor(10, portraitLayout ? 54 : 24);
    tft.print(subtitle);
  }
  drawAppNavControls();
}

void drawAppNavControls() {
  drawAppButton(appPrevButtonX, appButtonY, "<", false);
  drawAppButton(appNextButtonX, appButtonY, ">", false);
  drawAppButton(appSettingsButtonX, appButtonY, "*", currentApp == APP_SETTINGS);
}

void drawAppButton(int x, int y, const char* label, bool active) {
  uint16_t fill = buttonFillColor(active);
  uint16_t outline = COLOR_TEXT;
  tft.fillRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 10, APP_BUTTON_WIDTH, 1, chromeTextColor(), fill);
}

void drawSettingsScreen() {
  appNeedsRedraw = false;
  clampSettingsScroll();
  uint16_t bg = screenBgColor();
  tft.fillScreen(bg);
  drawAppFrameHeader("Settings", APP_FIRMWARE_DATE, bg, signalSearchColor());
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, settingsValueTextColor());
  tft.setTextSize(1);
  tft.setTextColor(settingsValueTextColor(), bg);
  tft.setCursor(10, portraitLayout ? 54 : 24);
  tft.print(APP_FIRMWARE_DATE);

  int rowY;
  char volumeText[12];
  snprintf(volumeText, sizeof(volumeText), "%u/10", speakerVolume);
  if (settingsRowVisible(0, &rowY)) {
    drawSettingsControlRow(0, rowY, "Volume", volumeText);
    int buttonY = rowY + 6;
    drawSettingsButton(settingsVolMinusX, buttonY, TOOLBAR_BUTTON_WIDTH, "-", false);
    drawSettingsButton(settingsVolPlusX, buttonY, TOOLBAR_BUTTON_WIDTH, "+", false);
  }

  char rotationText[14];
  snprintf(rotationText, sizeof(rotationText), "screen %u", screenRotation);
  if (settingsRowVisible(1, &rowY)) {
    drawSettingsControlRow(1, rowY, "Rotation", rotationText);
    drawSettingsButton(settingsRotateX, rowY + 6, 86, "", false);
    drawRotateIcon(settingsRotateX, rowY + 6, 86, TOOLBAR_BUTTON_HEIGHT, buttonTextColor(false), buttonFillColor(false));
  }

  if (settingsRowVisible(2, &rowY)) {
    drawSettingsControlRow(2, rowY, "Display", displayModeName());
    drawSettingsDisplayModeControl(settingsDisplayModeX, rowY + 6, 90);
  }

  if (settingsRowVisible(3, &rowY)) {
    drawSettingsRow(3, rowY, "WiFi", "setup later");
  }

  if (settingsRowVisible(4, &rowY)) {
    drawSettingsRow(4, rowY, "Bluetooth", "setup later");
  }

  if (settingsRowVisible(5, &rowY)) {
    drawSettingsControlRow(5, rowY, "LED Control", beatLedEnabled ? "beat pulse" : "off");
    drawSettingsButton(settingsLedX, rowY + 6, 86, beatLedEnabled ? "BEAT" : "OFF", beatLedEnabled);
  }

  if (settingsRowVisible(6, &rowY)) {
    drawSettingsControlRow(6, rowY, "Color", "tap");
    int swatchY = rowY + 6;
    drawSettingsSwatch(settingsColorRedX, swatchY, COLOR_RED, heartbeatLedColor.red > 0 && heartbeatLedColor.green == 0);
    drawSettingsSwatch(settingsColorYellowX, swatchY, COLOR_SIGNAL_YELLOW, heartbeatLedColor.red > 0 && heartbeatLedColor.green > 0);
    drawSettingsSwatch(settingsColorGreenX, swatchY, COLOR_LOCK_GREEN, heartbeatLedColor.green > 0 && heartbeatLedColor.red == 0);
  }

  if (settingsRowVisible(7, &rowY)) {
    drawSettingsRow(7, rowY, "About", "PulseSensor CYD");
  }

  if (settingsRowVisible(8, &rowY)) {
    drawSettingsRow(8, rowY, "Version", APP_VERSION);
  }

  if (settingsRowVisible(9, &rowY)) {
    drawSettingsRow(9, rowY, "Firmware", APP_FIRMWARE_DATE);
  }

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  uint32_t usedHeap = heapSize > freeHeap ? heapSize - freeHeap : 0;
  uint8_t freePercent = heapSize > 0 ? (uint8_t)((freeHeap * 100UL) / heapSize) : 0;
  char memoryText[32];
  snprintf(memoryText, sizeof(memoryText), "used %luK free %luK %u%%",
           (unsigned long)(usedHeap / 1024UL),
           (unsigned long)(freeHeap / 1024UL),
           freePercent);
  if (settingsRowVisible(10, &rowY)) {
    drawSettingsRow(10, rowY, "Memory", memoryText);
  }

  char buildText[28];
  snprintf(buildText, sizeof(buildText), "%s %s", APP_BUILD_RAM_USAGE, APP_BUILD_FLASH_USAGE);
  if (settingsRowVisible(11, &rowY)) {
    drawSettingsRow(11, rowY, "Build", buildText);
  }

  drawSettingsScrollControls();
}

bool settingsRowVisible(int rowIndex, int* rowY) {
  int y = settingsRowScreenY(rowIndex);
  if (rowY != nullptr) *rowY = y;
  return y >= settingsContentTop() && y + SETTINGS_ROW_H <= settingsContentBottom();
}

void drawSettingsRow(int rowIndex, int y, const char* label, const char* value) {
  uint16_t bg = settingsRowBackground(rowIndex);
  tft.fillRect(0, y, screenWidth, SETTINGS_ROW_H, bg);
  drawDottedHLine(0, y + SETTINGS_ROW_H - 2, screenWidth, settingsValueTextColor(), 5, 2);

  if (!portraitLayout && settingsValueNeedsCompactText(label, value)) {
    tft.setTextSize(SETTINGS_TEXT_SIZE);
    tft.setTextColor(settingsTextColor(), bg);
    tft.setCursor(10, y + 3);
    tft.print(label);
    tft.setTextColor(settingsValueTextColor(), bg);
    tft.setCursor(10, y + 21);
    tft.print(value);
    return;
  }

  tft.setTextSize(SETTINGS_TEXT_SIZE);
  tft.setTextColor(settingsTextColor(), bg);
  tft.setCursor(10, y + 12);
  tft.print(label);
  int valueTextSize = settingsValueTextSize(label, value);
  int valueY = y + max(2, (SETTINGS_ROW_H - (8 * valueTextSize)) / 2);
  tft.setTextColor(settingsValueTextColor(), bg);
  tft.setTextSize(valueTextSize);
  tft.setCursor(settingsRightAlignedValueX(value, valueTextSize), valueY);
  tft.print(value);
}

void drawSettingsControlRow(int rowIndex, int y, const char* label, const char* value) {
  uint16_t bg = settingsRowBackground(rowIndex);
  tft.fillRect(0, y, screenWidth, SETTINGS_ROW_H, bg);
  drawDottedHLine(0, y + SETTINGS_ROW_H - 2, screenWidth, settingsValueTextColor(), 5, 2);

  tft.setTextSize(SETTINGS_TEXT_SIZE);
  tft.setTextColor(settingsTextColor(), bg);
  tft.setCursor(10, y + 3);
  tft.print(label);
  tft.setTextColor(settingsValueTextColor(), bg);
  tft.setCursor(10, y + 21);
  tft.print(value);
}

int settingsTextWidth(const char* text, int textSize) {
  return strlen(text) * 6 * textSize;
}

int settingsValueTextSize(const char* label, const char* value) {
  if (!settingsValueNeedsCompactText(label, value)) return SETTINGS_TEXT_SIZE;
  return portraitLayout ? 1 : SETTINGS_TEXT_SIZE;
}

bool settingsValueNeedsCompactText(const char* label, const char* value) {
  int labelW = settingsTextWidth(label, SETTINGS_TEXT_SIZE);
  int valueW = settingsTextWidth(value, SETTINGS_TEXT_SIZE);
  int availableValueW = screenWidth - 20 - labelW - 8;
  return valueW > availableValueW;
}

int settingsRightAlignedValueX(const char* value, int textSize) {
  int valueW = settingsTextWidth(value, textSize);
  return max(10, screenWidth - 10 - valueW);
}

uint16_t settingsRowBackground(int rowIndex) {
  return screenBgColor();
}

void drawSettingsButton(int x, int y, int w, const char* label, bool active) {
  uint16_t fill = buttonFillColor(active);
  uint16_t outline = settingsTextColor();
  tft.fillRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 6, w, 2, buttonTextColor(active), fill);
}

void drawSettingsDisplayModeControl(int x, int y, int w) {
  drawSettingsButton(x, y, w, displayModeName(), true);
}

void drawSettingsSwatch(int x, int y, uint16_t color, bool active) {
  uint16_t outline = active ? settingsTextColor() : settingsValueTextColor();
  tft.fillRoundRect(x, y, 34, TOOLBAR_BUTTON_HEIGHT, 4, color);
  tft.drawRoundRect(x, y, 34, TOOLBAR_BUTTON_HEIGHT, 4, outline);
  if (active) {
    tft.drawRoundRect(x + 2, y + 2, 30, TOOLBAR_BUTTON_HEIGHT - 4, 3, screenBgColor());
  }
}

void drawSettingsScrollControls() {
  bool canScrollUp = settingsScrollY > 0;
  bool canScrollDown = settingsScrollY < settingsMaxScroll();
  tft.fillRect(0, settingsScrollButtonY - 3, screenWidth, SETTINGS_SCROLL_BUTTON_H + 7, screenBgColor());
  tft.drawFastHLine(0, settingsScrollButtonY - 4, screenWidth, settingsValueTextColor());
  drawSettingsButton(settingsScrollUpX, settingsScrollButtonY, settingsScrollButtonW, "^", canScrollUp);
  drawSettingsButton(settingsScrollDownX, settingsScrollButtonY, settingsScrollButtonW, "v", canScrollDown);
}

void drawPlaceholderApp(const char* title, const char* message) {
  if (appNeedsRedraw) {
    appNeedsRedraw = false;
    uint16_t bg = screenBgColor();
    tft.fillScreen(bg);
    drawAppFrameHeader(title, nullptr, bg, textColor());
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
    tft.fillRect(placeholderLastX - 2, placeholderLastY - 2, textW + 4, textH + 4, screenBgColor());
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

  drawPlaceholderText(message, currentApp == APP_PLACEHOLDER_1 ? signalLockColor() : signalSearchColor());
}

void drawPlaceholderText(const char* message, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT, screenBgColor());
  tft.setCursor(placeholderX, placeholderY);
  tft.print(message);
  placeholderLastX = placeholderX;
  placeholderLastY = placeholderY;
}

void drawApp4PinScanner() {
  if (!appNeedsRedraw && scannerActiveIndex < 0) return;
  if (!appNeedsRedraw && millis() - lastPinScannerDraw < PIN_SCANNER_DRAW_MS) return;
  lastPinScannerDraw = millis();

  uint16_t bg = screenBgColor();
  int contentTop = headerHeight + 8;
  int footerY = screenHeight - 14;
  int rowAreaH = max(1, footerY - contentTop - 4);
  int rowH = max(24, rowAreaH / PIN_SCANNER_PIN_COUNT);
  bool fullRedraw = appNeedsRedraw;

  if (fullRedraw) {
    appNeedsRedraw = false;
    tft.fillScreen(bg);
    char subtitle[24];
    snprintf(subtitle, sizeof(subtitle), "%s raw ADC 0..1023", displayModeName());
    drawAppFrameHeader("Pin Scanner", subtitle, bg, textColor());
  }

  if (fullRedraw) {
    for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
      bool hot = i == scannerActiveIndex && scannerPins[i].movement > HOT_MOVEMENT_MIN;
      drawPinScannerRow(i, contentTop + i * rowH, rowH, hot, true);
    }
  } else if (scannerActiveIndex >= 0 && scannerActiveIndex < PIN_SCANNER_PIN_COUNT) {
    bool hot = scannerPins[scannerActiveIndex].movement > HOT_MOVEMENT_MIN;
    drawPinScannerRow(scannerActiveIndex, contentTop + scannerActiveIndex * rowH, rowH, hot, false);
  }

  if (fullRedraw) {
    tft.fillRect(0, footerY - 1, screenWidth, 15, bg);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, bg);
    tft.setCursor(8, footerY);
    tft.print("Tap one pin. IO21/IO22 guarded.");
  }
}

void drawPinScannerRow(int index, int y, int rowH, bool hot, bool fullRedraw) {
  uint16_t bg = screenBgColor();
  bool active = index == scannerActiveIndex;
  bool adcCapable = isPinScannerAdcCapable(scannerPins[index].pin);
  uint16_t rowText = COLOR_TEXT;
  uint16_t barColor = pinScannerBarColor(hot);
  int labelX = 8;
  int barX = portraitLayout ? 72 : 82;
  int valueX = portraitLayout ? screenWidth - 74 : screenWidth - 76;
  int railX = screenWidth - 30;
  int barW = max(38, valueX - barX - 8);
  int barH = 12;
  int barY = y + max(6, (rowH - barH) / 2);
  int value = active ? constrain(scannerPins[index].value, 0, PIN_SCANNER_ADC_MAX_VALUE) : 0;
  int fillW = active && adcCapable ? map(value, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, barW) : 0;

  if (fullRedraw) {
    tft.fillRect(0, y, screenWidth, rowH, bg);
    drawDottedHLine(0, y + rowH - 1, screenWidth, gridSoftColor(), 5, 1);
  }

  if (active) {
    tft.fillRect(0, y + 2, 4, rowH - 4, bg);
    tft.fillRect(0, y + 2, 4, rowH - 4, hot ? pinScannerHotColor() : displayValueTextColor());
  } else if (fullRedraw) {
    tft.fillRect(0, y + 2, 4, rowH - 4, bg);
  }

  if (fullRedraw) {
    tft.setTextSize(1);
    tft.setTextColor(rowText, bg);
    tft.setCursor(labelX, y + 5);
    tft.print(scannerPins[index].label);
  }

  if (fullRedraw) {
    tft.drawRect(barX, barY, barW, barH, gridColor());
  }
  tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, bg);
  if (fillW > 2) {
    tft.fillRect(barX + 1, barY + 1, fillW - 2, barH - 2, barColor);
  }

  tft.setTextColor(displayValueTextColor(), bg);
  tft.fillRect(valueX, y + 2, screenWidth - valueX, 9, bg);
  tft.setCursor(valueX, y + 2);
  if (active && adcCapable) {
    tft.printf("%4d", value);
  } else {
    tft.print(pinScannerStatusText(index));
  }

  tft.setTextColor(COLOR_TEXT, bg);
  tft.fillRect(valueX, y + 13, screenWidth - valueX, 9, bg);
  tft.setCursor(valueX, y + 13);
  if (active && adcCapable) {
    tft.printf("d%4d", scannerPins[index].movement);
  } else if (active) {
    tft.print(pinScannerStatusText(index));
  }

  tft.fillRect(railX, y + 2, screenWidth - railX, 22, bg);
  if (hot) {
    tft.setTextColor(COLOR_TEXT, bg);
    tft.setCursor(railX, y + 2);
    tft.print("hot");
  }

  if (active && adcCapable && isPinScannerRailed(value)) {
    tft.setTextColor(COLOR_TEXT, bg);
    tft.setCursor(railX, y + 13);
    tft.print("rail");
  }
}

uint16_t app3Blend565(uint16_t from, uint16_t to, int amount) {
  amount = constrain(amount, 0, 255);
  int fromR = (from >> 11) & 0x1F;
  int fromG = (from >> 5) & 0x3F;
  int fromB = from & 0x1F;
  int toR = (to >> 11) & 0x1F;
  int toG = (to >> 5) & 0x3F;
  int toB = to & 0x1F;
  int outR = fromR + ((toR - fromR) * amount) / 255;
  int outG = fromG + ((toG - fromG) * amount) / 255;
  int outB = fromB + ((toB - fromB) * amount) / 255;
  return (outR << 11) | (outG << 5) | outB;
}

void drawApp3CrawlLinePerspective(const char* line, int localY, uint16_t baseColor,
                                  uint16_t bg, int crawlTop, int crawlBottom) {
  if (line[0] == '\0') return;
  if (localY < crawlTop || localY > crawlBottom - 8) return;

  int depth = map(constrain(localY, APP3_CRAWL_HORIZON_Y, crawlBottom),
                  APP3_CRAWL_HORIZON_Y, crawlBottom, 0, 255);
  if (depth < 20) return;

  int textSize = depth < 116 ? APP3_CRAWL_MIN_TEXT_SIZE : APP3_CRAWL_TEXT_SIZE;
  int textW = strlen(line) * 6 * textSize;
  int vanishingW = map(depth, 0, 255, screenWidth / 3, screenWidth);
  int x = (screenWidth - min(textW, vanishingW)) / 2;
  if (textW < vanishingW) x = (screenWidth - textW) / 2;

  app3CrawlSprite.setTextSize(textSize);
    app3CrawlSprite.setTextColor(COLOR_TEXT, bg);
  app3CrawlSprite.setCursor(max(0, x), localY);
  app3CrawlSprite.print(line);
}

void drawApp3CrawlLinePerspectiveDirect(const char* line, int localY, uint16_t baseColor,
                                        uint16_t bg, int crawlTop, int crawlBottom) {
  if (line[0] == '\0') return;
  if (localY < crawlTop || localY > crawlBottom - 8) return;

  int depth = map(constrain(localY, APP3_CRAWL_HORIZON_Y, crawlBottom),
                  APP3_CRAWL_HORIZON_Y, crawlBottom, 0, 255);
  if (depth < 20) return;

  int textSize = depth < 116 ? APP3_CRAWL_MIN_TEXT_SIZE : APP3_CRAWL_TEXT_SIZE;
  int textW = strlen(line) * 6 * textSize;
  int vanishingW = map(depth, 0, 255, screenWidth / 3, screenWidth);
  int x = (screenWidth - min(textW, vanishingW)) / 2;
  if (textW < vanishingW) x = (screenWidth - textW) / 2;

  tft.setTextSize(textSize);
  tft.setTextColor(COLOR_TEXT, bg);
  tft.setCursor(max(0, x), headerHeight + localY);
  tft.print(line);
}

void drawApp3OriginCrawl() {
  unsigned long now = millis();
  if (!appNeedsRedraw && now - lastApp3CrawlFrame < APP3_CRAWL_FRAME_MS) return;
  lastApp3CrawlFrame = now;

  uint16_t bg = COLOR_BG;
  uint16_t gold = displayMode == DISPLAY_COLOR_LIGHT ? COLOR_LIGHT_AMBER : COLOR_SIGNAL_YELLOW;
  uint16_t dimGold = displayMode == DISPLAY_MONO_DARK || displayMode == DISPLAY_MONO_LIGHT ? textColor() : COLOR_AMBER;
  int contentH = screenHeight - headerHeight;

  if (appNeedsRedraw) {
    appNeedsRedraw = false;
    tft.fillScreen(bg);
    app3CrawlStartTime = now;
    drawAppFrameHeader("PulseSensor.com", "Origin Story", bg, gold);
  }

  int crawlTop = 6;
  int crawlBottom = contentH - 4;
  int lineHeight = 24;
  unsigned long elapsed = now - app3CrawlStartTime;
  int travel = APP3_ORIGIN_CRAWL_LINE_COUNT * lineHeight + (crawlBottom - crawlTop) + 40;
  int offset = (elapsed / APP3_CRAWL_SPEED_MS) % travel;
  int baseY = crawlBottom - lineHeight - offset;

  if (!ensureApp3CrawlSprite(screenWidth, contentH)) {
    drawApp3OriginCrawlDirectFallback(bg, gold, dimGold, crawlTop, crawlBottom, lineHeight, baseY);
    return;
  }

  app3CrawlSprite.fillSprite(bg);
  drawApp3Starfield();
  app3CrawlSprite.drawFastHLine(46, APP3_CRAWL_HORIZON_Y, screenWidth - 92, app3Blend565(bg, dimGold, 72));

  for (int i = 0; i < APP3_ORIGIN_CRAWL_LINE_COUNT; i++) {
    const char* line = APP3_ORIGIN_CRAWL_LINES[i];
    int y = baseY + i * lineHeight;
    uint16_t color = i < 3 ? gold : dimGold;
    drawApp3CrawlLinePerspective(line, y, color, bg, crawlTop, crawlBottom);
  }

  app3CrawlSprite.pushSprite(0, headerHeight);
}

bool ensureApp3CrawlSprite(int w, int h) {
  if (w <= 0 || h <= 0) return false;
  if (app3CrawlSpriteReady && app3CrawlSpriteW == w && app3CrawlSpriteH == h) return true;

  releaseApp3CrawlSprite();

  app3CrawlSprite.setColorDepth(8);
  app3CrawlSpriteReady = app3CrawlSprite.createSprite(w, h) != nullptr;
  app3CrawlSpriteW = app3CrawlSpriteReady ? w : 0;
  app3CrawlSpriteH = app3CrawlSpriteReady ? h : 0;
  return app3CrawlSpriteReady;
}

void drawApp3OriginCrawlDirectFallback(uint16_t bg, uint16_t gold, uint16_t dimGold,
                                       int crawlTop, int crawlBottom, int lineHeight,
                                       int baseY) {
  tft.fillRect(0, headerHeight, screenWidth, screenHeight - headerHeight, bg);
  tft.drawFastHLine(46, headerHeight + APP3_CRAWL_HORIZON_Y, screenWidth - 92, app3Blend565(bg, dimGold, 72));
  for (int i = 0; i < APP3_ORIGIN_CRAWL_LINE_COUNT; i++) {
    const char* line = APP3_ORIGIN_CRAWL_LINES[i];
    int localY = baseY + i * lineHeight;
    uint16_t color = i < 3 ? gold : dimGold;
    drawApp3CrawlLinePerspectiveDirect(line, localY, color, bg, crawlTop, crawlBottom);
  }
}

void drawApp3Starfield() {
  const uint16_t stars[][2] = {
    {18, 55}, {42, 94}, {64, 148}, {86, 72}, {109, 210}, {130, 118},
    {151, 60}, {173, 169}, {195, 96}, {217, 222}, {240, 142},
    {263, 75}, {286, 190}, {306, 114}, {28, 222}, {300, 55}
  };
  uint16_t starColor = displayMode == DISPLAY_MONO_DARK || displayMode == DISPLAY_MONO_LIGHT ? textColor() : COLOR_TEXT;
  for (int i = 0; i < 16; i++) {
    int x = stars[i][0];
    int y = stars[i][1] - headerHeight;
    if (x >= app3CrawlSpriteW || y < 0 || y >= app3CrawlSpriteH) continue;
    app3CrawlSprite.drawPixel(x, y, starColor);
  }
}

void drawGraphFrame() {
  tft.fillRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, panelDarkColor());
  tft.drawRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, graphGridColor());
  tft.fillRect(graphLeft, graphTop, graphWidth, graphHeight, screenBgColor());

  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;

  for (int x = 0; x <= graphWidth; x += verticalGridStep) {
    tft.drawFastVLine(graphLeft + x, graphTop, graphHeight, graphGridColor());
  }
  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawFastHLine(graphLeft, graphTop + y, graphWidth, graphGridColor());
  }
  for (int x = 0; x < graphWidth; x += GRAPH_THRESHOLD_DOT_STEP) {
    drawThresholdMarker(x);
  }

  drawGraphLabels();
  drawSignalCoachStatus();
}

void refreshWaveformFrameForLockTransition() {
  tft.fillRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, panelDarkColor());
  tft.drawRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, graphGridColor());
  tft.fillRect(graphLeft, graphTop, graphWidth, graphHeight, screenBgColor());

  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;

  for (int x = 0; x <= graphWidth; x += verticalGridStep) {
    tft.drawFastVLine(graphLeft + x, graphTop, graphHeight, graphGridColor());
  }
  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawFastHLine(graphLeft, graphTop + y, graphWidth, graphGridColor());
  }
  for (int x = 0; x < graphWidth; x += GRAPH_THRESHOLD_DOT_STEP) {
    drawThresholdMarker(x);
  }

  drawGraphLabels();
  drawSignalCoachStatus();
}

void drawGraphLabels() {
  tft.setTextSize(1);
  tft.setTextColor(textColor(), screenBgColor());
  tft.setCursor(graphLeft + 6, graphTop + 5);
  tft.print("LIVE LINE");

  tft.setCursor(graphLeft + graphWidth - 48, graphTop + 5);
  tft.print("THR ");
  tft.print(activePulseThreshold);
}

void drawSignalCoachStatus() {
  const char* status = signalCoachText();
  int statusW = strlen(status) * 6;
  int statusAreaW = min(graphWidth - 12, 112);
  int statusX = graphLeft + graphWidth - statusAreaW - 3;
  int statusY = graphTop + graphHeight - 16;
  int textX = statusX + max(0, statusAreaW - statusW - 3);

  tft.fillRect(statusX, statusY, statusAreaW, 14, screenBgColor());
  tft.setTextSize(1);
  tft.setTextColor(textColor(), screenBgColor());
  tft.setCursor(textX, statusY + 2);
  tft.print(status);
}

void drawGraphColumnBackground(int localX) {
  int screenX = graphLeft + localX;
  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;
  tft.drawFastVLine(screenX, graphTop, graphHeight, screenBgColor());

  if (localX % verticalGridStep == 0) {
    tft.drawFastVLine(screenX, graphTop, graphHeight, graphGridColor());
  }

  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawPixel(screenX, graphTop + y, graphGridColor());
  }

  drawThresholdMarker(localX);
}

void drawThresholdMarker(int localX) {
  int y = signalToGraphY(activePulseThreshold);

  if (localX % GRAPH_THRESHOLD_DOT_STEP == 0) {
    tft.drawFastVLine(graphLeft + localX, y - 1, 3, COLOR_TEXT);
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
  int clearX0 = graphX;
  int clearX1 = (graphX + 1) % graphWidth;
  int clearX2 = (graphX + 2) % graphWidth;

  clearWaveformBeatMarkerAt(clearX0);
  clearWaveformBeatMarkerAt(clearX1);
  clearWaveformBeatMarkerAt(clearX2);
  drawGraphColumnBackground(clearX0);
  drawGraphColumnBackground(clearX1);
  drawGraphColumnBackground(clearX2);

  uint16_t waveColor = liveTraceColor();

  if (graphX > 0) {
    int x0 = graphLeft + graphX - 1;
    int x1 = graphLeft + graphX;
    for (int offset = -WAVEFORM_TRACE_HALF_THICKNESS; offset <= WAVEFORM_TRACE_HALF_THICKNESS; offset++) {
      tft.drawLine(x0, lastGraphY + offset, x1, y + offset, waveColor);
    }
  }

  if (waveformBeatMarkerPending) {
    addWaveformBeatMarker(graphX, y, waveformBeatMarkerPendingAccepted);
    waveformBeatMarkerPending = false;
  }
  redrawWaveformBeatMarkers();

  lastGraphY = y;
  graphX++;

  if (graphX >= graphWidth) {
    graphX = 0;
    lastGraphY = y;
    drawGraphLabels();
    drawSignalCoachStatus();
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
  tft.drawRoundRect(x, y, w, h, 6, valid ? signalLockColor() : signalSearchColor());
  if (!valid) {
    tft.drawRoundRect(x + 4, y + 4, w - 8, h - 8, 4, inactiveColor());
  }

  tft.setTextSize(1);
  tft.setTextColor(textColor(), panelBg);
  tft.setCursor(x + 8, y + 8);
  tft.print(label);

  uint8_t valueTextSize = portraitLayout ? 3 : 4;
  if (portraitLayout && strcmp(label, "IBI") == 0 && value >= 1000) {
    valueTextSize = 2;
  }
  tft.setTextSize(valueTextSize);
  tft.setTextColor(COLOR_TEXT, panelBg);
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
    tft.setTextColor(textColor(), panelBg);
    tft.setCursor(x + (portraitLayout ? 43 : 72), y + h - 17);
    tft.print(unit);
  }
}

uint16_t metricPanelBackground(const char* label, bool valid) {
  return panelBgColor();
}

void drawSignalPanel() {
  char pinLabel[8];

  snprintf(pinLabel, sizeof(pinLabel), "GPIO%d", PULSE_PIN);

  uint16_t panelBg = panelBgColor();
  tft.fillRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, panelBg);
  tft.drawRoundRect(signalPanelX, signalPanelY, signalPanelW, signalPanelH, 6, lockedSignal ? signalLockColor() : signalSearchColor());
  if (!lockedSignal) {
    tft.drawRoundRect(signalPanelX + 4, signalPanelY + 4, signalPanelW - 8, signalPanelH - 8, 4, inactiveColor());
  }

  tft.setTextSize(1);
  tft.setTextColor(textColor(), panelBg);
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
  bool drawInactiveSegments = shouldDrawInactiveQualitySegments();
  for (int i = 0; i < SIGNAL_QUALITY_STEPS; i++) {
    if (i < signalQuality) {
      uint16_t color = lockedSignal ? signalLockColor() : signalSearchColor();
      tft.fillRect(x + i * (segmentW + segmentGap), y, segmentW, segmentH, color);
    } else if (drawInactiveSegments) {
      tft.fillRect(x + i * (segmentW + segmentGap), y, segmentW, segmentH, inactiveColor());
    }
  }
}

void drawAmplitudeMeter(int x, int y, int amplitude) {
  int segments = amplitudeMeterSegments(amplitude);
  int displayAmplitude = constrain(amplitude, 0, 999);

  for (int i = 0; i < 10; i++) {
    uint16_t color = inactiveColor();
    if (i < segments) {
      color = lockedSignal ? signalLockColor() : signalSearchColor();
    }
    tft.fillRect(x + i * 4, y, 3, 7, color);
  }

  tft.setTextSize(1);
  tft.setTextColor(textColor(), panelBgColor());
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
  const int clearX = centerX - HEART_SPRITE_WIDTH / 2;
  const int clearY = centerY - HEART_SPRITE_HEIGHT / 2;
  uint16_t heartColor = blendRed(ledBrightness);
  uint16_t outlineColor = COLOR_TEXT;

  if (ensureHeartSprite()) {
    heartSprite.fillSprite(screenBgColor());
    fillHeartShapeSprite(heartSprite, HEART_SPRITE_WIDTH / 2, HEART_SPRITE_HEIGHT / 2, size + 2, outlineColor);
    fillHeartShapeSprite(heartSprite, HEART_SPRITE_WIDTH / 2, HEART_SPRITE_HEIGHT / 2, size, heartColor);
    heartSprite.pushSprite(clearX, clearY);
  } else {
    tft.fillRect(clearX, clearY, HEART_SPRITE_WIDTH, HEART_SPRITE_HEIGHT, screenBgColor());
    fillHeartShape(centerX, centerY, size + 2, outlineColor);
    fillHeartShape(centerX, centerY, size, heartColor);
  }

  lastDraw = millis();
  lastSize = size;
  lastBrightness = ledBrightness;
  beatHeartNeedsRedraw = false;
}

bool ensureHeartSprite() {
  if (heartSpriteReady) return true;
  heartSprite.setColorDepth(8);
  heartSpriteReady = heartSprite.createSprite(HEART_SPRITE_WIDTH, HEART_SPRITE_HEIGHT) != nullptr;
  return heartSpriteReady;
}

void fillHeartShape(int centerX, int centerY, int size, uint16_t color) {
  tft.fillCircle(centerX - size / 2, centerY - size / 3, size / 2, color);
  tft.fillCircle(centerX + size / 2, centerY - size / 3, size / 2, color);
  tft.fillTriangle(centerX - size, centerY - size / 4,
                   centerX + size, centerY - size / 4,
                   centerX, centerY + size, color);
}

void fillHeartShapeSprite(TFT_eSprite& sprite, int centerX, int centerY, int size, uint16_t color) {
  sprite.fillCircle(centerX - size / 2, centerY - size / 3, size / 2, color);
  sprite.fillCircle(centerX + size / 2, centerY - size / 3, size / 2, color);
  sprite.fillTriangle(centerX - size, centerY - size / 4,
                      centerX + size, centerY - size / 4,
                      centerX, centerY + size, color);
}

void drawCenteredText(const char* text, int x, int y, int w, int textSize, uint16_t color, uint16_t bg) {
  int charW = 6 * textSize;
  int textW = strlen(text) * charW;
  int cursorX = x + max(0, (w - textW) / 2);
  tft.setTextSize(textSize);
  tft.setTextColor(COLOR_TEXT, bg);
  tft.setCursor(cursorX, y);
  tft.print(text);
}

uint16_t liveTraceColor() {
  return liveTraceColorForMode();
}

void drawBeatMarker(int x, int y) {
  tft.fillCircle(x, y, WAVEFORM_BEAT_MARKER_RADIUS, COLOR_TEXT);
}

void clearWaveformBeatMarkerAt(int localX) {
  for (int i = 0; i < WAVEFORM_BEAT_MARKER_COUNT; i++) {
    if (!waveformBeatMarkerActive[i]) continue;
    int dx = abs(localX - waveformBeatMarkerX[i]);
    dx = min(dx, graphWidth - dx);
    if (dx <= waveformBeatMarkerRadius[i]) {
      waveformBeatMarkerActive[i] = false;
    }
  }
}

void addWaveformBeatMarker(int localX, int y, bool accepted) {
  waveformBeatMarkerX[waveformBeatMarkerWrite] = localX;
  waveformBeatMarkerY[waveformBeatMarkerWrite] = y;
  waveformBeatMarkerRadius[waveformBeatMarkerWrite] = accepted ? WAVEFORM_BEAT_MARKER_RADIUS : WAVEFORM_CALC_MARKER_RADIUS;
  waveformBeatMarkerFilled[waveformBeatMarkerWrite] = accepted;
  waveformBeatMarkerActive[waveformBeatMarkerWrite] = true;
  waveformBeatMarkerWrite = (waveformBeatMarkerWrite + 1) % WAVEFORM_BEAT_MARKER_COUNT;
}

void redrawWaveformBeatMarkers() {
  for (int i = 0; i < WAVEFORM_BEAT_MARKER_COUNT; i++) {
    if (!waveformBeatMarkerActive[i]) continue;
    int x = graphLeft + waveformBeatMarkerX[i];
    int y = waveformBeatMarkerY[i];
    int radius = waveformBeatMarkerRadius[i];
    if (waveformBeatMarkerFilled[i]) {
      tft.fillCircle(x, y, radius, COLOR_TEXT);
    } else {
      tft.drawCircle(x, y, radius, COLOR_TEXT);
      tft.drawPixel(x, y, COLOR_TEXT);
    }
  }
}

uint16_t blendRed(int brightness) {
  brightness = constrain(brightness, 0, 255);
  if (displayMode == DISPLAY_MONO_DARK || displayMode == DISPLAY_MONO_LIGHT) {
    return brightness < 20 ? screenBgColor() : textColor();
  }
  if (brightness < 20) return displayMode == DISPLAY_COLOR_LIGHT ? COLOR_RED_DARK : COLOR_RED_DARK;
  if (brightness < 120) return displayMode == DISPLAY_COLOR_LIGHT ? COLOR_RED : 0xA800;
  return COLOR_RED;
}

void cycleDisplayMode() {
  displayMode = static_cast<DisplayMode>((displayMode + 1) % DISPLAY_MODE_COUNT);
  resetDashboardState();
  appNeedsRedraw = true;
}

const char* displayModeName() {
  switch (displayMode) {
    case DISPLAY_MONO_DARK:
      return "M DARK";
    case DISPLAY_MONO_LIGHT:
      return "M LIGHT";
    case DISPLAY_COLOR_DARK:
      return "C DARK";
    case DISPLAY_COLOR_LIGHT:
      return "C LIGHT";
    default:
      return "M DARK";
  }
}

uint16_t screenBgColor() {
  return (displayMode == DISPLAY_MONO_LIGHT || displayMode == DISPLAY_COLOR_LIGHT) ? COLOR_TEXT : COLOR_BG;
}

uint16_t panelBgColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_BG;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_TEXT;
  return screenBgColor();
}

uint16_t panelDarkColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_PANEL_DARK;
  return screenBgColor();
}

uint16_t gridColor() {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) {
    return lockedSignal ? signalLockColor() : signalSearchColor();
  }
  return textColor();
}

uint16_t gridSoftColor() {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) {
    return lockedSignal ? signalLockColor() : signalSearchColor();
  }
  return textColor();
}

uint16_t graphGridColor() {
  return COLOR_GRAPH_GRID_GRAY;
}

uint16_t chromeTextColor() {
  return COLOR_TEXT;
}

uint16_t settingsTextColor() {
  return COLOR_TEXT;
}

uint16_t settingsValueTextColor() {
  return COLOR_TEXT;
}

uint16_t textColor() {
  return COLOR_TEXT;
}

uint16_t displayValueTextColor() {
  return COLOR_TEXT;
}

uint16_t buttonFillColor(bool active) {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_BG;
  if (displayMode == DISPLAY_COLOR_LIGHT) return active ? COLOR_LIGHT_BUTTON_FILL : COLOR_TEXT;
  return screenBgColor();
}

uint16_t buttonOutlineColor(bool active) {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) return textColor();
  return textColor();
}

uint16_t buttonTextColor(bool active) {
  return chromeTextColor();
}

uint16_t signalSearchColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_SIGNAL_YELLOW;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_AMBER;
  return textColor();
}

uint16_t signalLockColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_LOCK_GREEN;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_GREEN;
  return textColor();
}

uint16_t inactiveColor() {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) {
    return lockedSignal ? signalLockColor() : signalSearchColor();
  }
  return textColor();
}

bool shouldDrawInactiveQualitySegments() {
  return displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT;
}

uint16_t beatColor() {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) {
    return lockedSignal ? signalLockColor() : signalSearchColor();
  }
  return textColor();
}

uint16_t liveTraceColorForMode() {
  return lockedSignal ? signalLockColor() : signalSearchColor();
}

uint16_t pinScannerHotColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_SIGNAL_YELLOW;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_AMBER;
  return textColor();
}

uint16_t pinScannerBarColor(bool hot) {
  if (hot) return pinScannerHotColor();
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LOCK_GREEN;
  return textColor();
}

uint16_t pinScannerRailColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_RED;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_RED_DARK;
  return textColor();
}

void drawDottedHLine(int x, int y, int w, uint16_t color, int step, int thickness) {
  for (int px = x; px < x + w; px += step) {
    tft.fillRect(px, y, thickness, thickness, color);
  }
}

void drawRotateIcon(int x, int y, int w, int h, uint16_t color, uint16_t bg) {
  int cx = x + w / 2;
  int cy = y + h / 2;
  tft.drawCircle(cx, cy, 8, color);
  tft.fillRect(cx - 10, cy - 10, 9, 8, bg);
  tft.drawFastHLine(cx - 1, cy - 8, 8, color);
  tft.drawFastVLine(cx + 6, cy - 8, 7, color);
  tft.fillTriangle(cx + 3, cy - 10,
                   cx + 10, cy - 8,
                   cx + 6, cy - 2,
                   color);
}
