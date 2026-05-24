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
#define APP3_CRAWL_FANFARE_STEP_COUNT 42
#define APP3_CRAWL_FANFARE_LOOP_START_STEP 0
#define APP3_CRAWL_FANFARE_LOOP_MS 15000
#define APP3_ORIGIN_CRAWL_LINE_COUNT 68
#define APP3_CRAWL_FRAME_MS 72
#define APP3_CRAWL_SPEED_MS 52
#define APP3_CRAWL_TEXT_SIZE 2
#define APP3_CRAWL_MIN_TEXT_SIZE 1
#define APP3_CRAWL_HORIZON_Y 8
#define PIN_SCANNER_PIN_COUNT 6
#define PIN_SCANNER_ADC_MAX_VALUE 4095
#define HOT_MOVEMENT_MIN 20
#define SORT_INTERVAL_MS 3000
#define SORT_HYSTERESIS 8
#define PIN_SCANNER_DRAW_MS 100
#define HEART_MIN_SIZE 8
#define HEART_MAX_SIZE 15
#define VOLUME_MIN 0
#define VOLUME_MAX 10
#define VOLUME_START 1
#define SCREEN_ROTATION_DEFAULT 1
#define SCREEN_ROTATION_COUNT 4

// ===== APP SHELL =====

#define APP_VERSION "0.4.8-pin-scanner"
#define APP_FIRMWARE_DATE "2026-05-24"
#define TOOLBAR_BUTTON_WIDTH 44
#define TOOLBAR_BUTTON_HEIGHT 28
#define APP_BUTTON_WIDTH TOOLBAR_BUTTON_WIDTH
#define APP_BUTTON_HEIGHT TOOLBAR_BUTTON_HEIGHT
#define APP_BUTTON_GAP 2
#define SETTINGS_TEXT_SIZE 2
#define SETTINGS_ROW_H 40
#define SETTINGS_ROW_COUNT 10
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
#define COLOR_LIGHT_BUTTON_FILL 0xE7DF
#define COLOR_LIGHT_BLUE 0x02F6
#define COLOR_LIGHT_TRACE_BLUE 0x039F
#define COLOR_LIGHT_CYAN 0x047F
#define COLOR_LIGHT_TEAL 0x04B0
#define COLOR_LIGHT_GREEN 0x04A0
#define COLOR_LIGHT_AMBER 0xBC20
#define COLOR_LIGHT_INACTIVE 0x94B2
#define COLOR_LIGHT_NAV_FILL 0x001F

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
  APP_PIN_SCANNER,
  APP_SETTINGS,
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
  "WorldFamousElectronics/",
  "PulseSensor_Amped_Arduino",
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

ScannerPin scannerPins[] = {
  {"P3  IO35", 35, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"IO22", 22, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"CN1 IO27", 27, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"LDR IO34", 34, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"IO32", 32, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
  {"IO33", 33, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, 0},
};

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

bool app3CrawlFanfarePlaying = false;
uint8_t app3CrawlFanfareStep = 0;
unsigned long app3CrawlFanfareNextStepTime = 0;

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
unsigned long app3CrawlStartTime = 0;
unsigned long lastApp3CrawlFrame = 0;
bool app3CrawlSpriteReady = false;
int app3CrawlSpriteW = 0;
int app3CrawlSpriteH = 0;
unsigned long lastPinScannerDraw = 0;
unsigned long lastPinScannerSort = 0;

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
bool handleSettingsDisplayModeTouch(int16_t x, int16_t y);
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
uint16_t scaledApp3CrawlFanfareDuty(uint8_t duty);
void startApp3CrawlFanfare();
void updateApp3CrawlFanfare();
void stopApp3CrawlFanfare();
void playApp3CrawlFanfareStep();
void triggerRearLedPulse(RearLedColor color);
void triggerBeatEffects();
void setupPulseSensor();
void setupPinScanner();
void updatePinScannerReadings();
void maybeSortScannerPins();
int hottestScannerPinIndex();
bool isPinScannerRailed(int value);
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
void drawSettingsDisplayModeControl(int x, int y, int w);
void drawSettingsSwatch(int x, int y, uint16_t color, bool active);
void drawSettingsScrollControls();
void drawPlaceholderApp(const char* title, const char* message);
void drawPlaceholderText(const char* message, uint16_t color);
void drawApp3OriginCrawl();
void drawApp4PinScanner();
void drawPinScannerRow(int index, int y, int rowH, bool hot);
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
void drawRotateControl();
void drawRotateIcon(int x, int y, int w, int h, uint16_t color, uint16_t bg);
void drawVolumeControl();
void drawDottedHLine(int x, int y, int w, uint16_t color, int step, int thickness);
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
void cycleDisplayMode();
const char* displayModeName();
uint16_t screenBgColor();
uint16_t panelBgColor();
uint16_t panelDarkColor();
uint16_t gridColor();
uint16_t gridSoftColor();
uint16_t textColor();
uint16_t displayValueTextColor();
uint16_t buttonFillColor(bool active);
uint16_t buttonOutlineColor(bool active);
uint16_t buttonTextColor(bool active);
uint16_t signalSearchColor();
uint16_t signalLockColor();
uint16_t inactiveColor();
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
  readTouchControls();
  updateLED();
  updateBeatChime();
  updateSignalHarmony();
  updateApp3CrawlFanfare();
  if (currentApp != APP_PIN_SCANNER) readPulseSensor();

  if (currentApp == APP_PULSE) {
    drawBeatHeart();
    drawWaveform();
    drawDashboardIfChanged();
  } else if (currentApp == APP_PLACEHOLDER_1) {
    drawPlaceholderApp("App 2", "your app here");
  } else if (currentApp == APP_PLACEHOLDER_2) {
    drawApp3OriginCrawl();
  } else if (currentApp == APP_PIN_SCANNER) {
    updatePinScannerReadings();
    maybeSortScannerPins();
    drawApp4PinScanner();
  } else if (appNeedsRedraw) {
    drawSettingsScreen();
  }

  if (millis() - lastSerialPrint >= 500) {
    lastSerialPrint = millis();
    if (currentApp == APP_PIN_SCANNER) {
      for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
        Serial.printf("%s=%4d d%4d%s",
                      scannerPins[i].label,
                      scannerPins[i].value,
                      scannerPins[i].movement,
                      i == PIN_SCANNER_PIN_COUNT - 1 ? "\n" : "  ");
      }
    } else {
      Serial.printf("signal=%d amp=%d bpm=%d ibi=%d locked=%d quality=%d\n",
                    currentSignal, pulseAmplitude, displayBPM, displayIBI,
                    lockedSignal ? 1 : 0, signalQuality);
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
  bool enteringPinScanner = currentApp != APP_PIN_SCANNER && app == APP_PIN_SCANNER;
  bool leavingPinScanner = currentApp == APP_PIN_SCANNER && app != APP_PIN_SCANNER;
  currentApp = app;
  stopSignalHarmony();
  if (!enteringApp3) stopApp3CrawlFanfare();
  if (enteringPinScanner) setupPinScanner();
  if (leavingPinScanner) setupPulseSensor();
  dashboardDrawn = false;
  appNeedsRedraw = true;
  if (enteringSettings) settingsScrollY = 0;
  resetPlaceholderState();
  drawActiveApp();
  if (enteringApp3) startApp3CrawlFanfare();
}

void nextApp() {
  uint8_t next = currentApp == APP_SETTINGS ? APP_PULSE : (uint8_t)currentApp + 1;
  if (next > APP_PIN_SCANNER) next = APP_PULSE;
  switchApp((AppId)next);
}

void previousApp() {
  uint8_t previous = currentApp == APP_SETTINGS ? APP_PIN_SCANNER : (uint8_t)currentApp;
  previous = previous == APP_PULSE ? APP_PIN_SCANNER : previous - 1;
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
    heartCenterX = screenWidth / 2;
    heartCenterY = 48;

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

    rotateButtonX = screenWidth - rotateButtonWidth - 4;
    rotateButtonY = 7;
    volumePlusX = rotateButtonX - volumeButtonSize - 6;
    volumeValueX = volumePlusX - 24;
    volumeMinusX = volumeValueX - volumeButtonSize - 2;
    volumeLabelX = volumeMinusX - 28;
    volumeY = 9;

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
  app3CrawlStartTime = millis();
  lastApp3CrawlFrame = 0;
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
  pulseSensor.setThreshold(PULSE_THRESHOLD);
  pulseSensorReady = pulseSensor.begin();

  if (!pulseSensorReady) {
    Serial.println("PulseSensor initialization failed");
  }
}

void setupPinScanner() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
    pinMode(scannerPins[i].pin, INPUT);
    scannerPins[i].value = 0;
    scannerPins[i].minValue = PIN_SCANNER_ADC_MAX_VALUE;
    scannerPins[i].maxValue = 0;
    scannerPins[i].movement = 0;
  }

  lastPinScannerDraw = 0;
  lastPinScannerSort = millis();
}

void updatePinScannerReadings() {
  for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
    long total = 0;
    for (int sample = 0; sample < 8; sample++) {
      total += analogRead(scannerPins[i].pin);
      delayMicroseconds(150);
    }

    scannerPins[i].value = total / 8;
    if (scannerPins[i].value < scannerPins[i].minValue) scannerPins[i].minValue = scannerPins[i].value;
    if (scannerPins[i].value > scannerPins[i].maxValue) scannerPins[i].maxValue = scannerPins[i].value;
    scannerPins[i].movement = scannerPins[i].maxValue - scannerPins[i].minValue;

    if (scannerPins[i].minValue < scannerPins[i].value) scannerPins[i].minValue++;
    if (scannerPins[i].maxValue > scannerPins[i].value) scannerPins[i].maxValue--;
  }
}

void maybeSortScannerPins() {
  if (millis() - lastPinScannerSort < SORT_INTERVAL_MS) return;
  lastPinScannerSort = millis();

  for (int pass = 0; pass < PIN_SCANNER_PIN_COUNT - 1; pass++) {
    for (int i = 0; i < PIN_SCANNER_PIN_COUNT - 1 - pass; i++) {
      if (scannerPins[i + 1].movement > scannerPins[i].movement + SORT_HYSTERESIS) {
        ScannerPin temp = scannerPins[i];
        scannerPins[i] = scannerPins[i + 1];
        scannerPins[i + 1] = temp;
      }
    }
  }
}

int hottestScannerPinIndex() {
  int hotIndex = 0;
  for (int i = 1; i < PIN_SCANNER_PIN_COUNT; i++) {
    if (scannerPins[i].movement > scannerPins[hotIndex].movement) hotIndex = i;
  }
  return hotIndex;
}

bool isPinScannerRailed(int value) {
  return value < 20 || value > PIN_SCANNER_ADC_MAX_VALUE - 20;
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
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawPlaceholderApp("App 2", "your app here");
  } else if (currentApp == APP_PLACEHOLDER_2) {
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawApp3OriginCrawl();
  } else if (currentApp == APP_PIN_SCANNER) {
    tft.fillScreen(screenBgColor());
    appNeedsRedraw = true;
    drawApp4PinScanner();
  }
}

void drawStaticScreen() {
  tft.fillScreen(screenBgColor());
  drawHeader();
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
    drawGraphFrame();
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
  uint16_t bg = screenBgColor();
  tft.fillRect(0, 0, screenWidth, headerHeight, bg);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
  beatHeartNeedsRedraw = true;

  tft.setTextSize(1);
  tft.setTextColor(textColor(), bg);
  tft.setCursor(portraitLayout ? 52 : 10, portraitLayout ? 38 : 8);
  tft.print("PulseSensor.com");
  drawAppNavControls();
}

void drawAppNavControls() {
  drawAppButton(appPrevButtonX, appButtonY, "<", false);
  drawAppButton(appNextButtonX, appButtonY, ">", false);
  drawAppButton(appSettingsButtonX, appButtonY, "*", currentApp == APP_SETTINGS);
}

void drawAppButton(int x, int y, const char* label, bool active) {
  uint16_t fill = buttonFillColor(active);
  uint16_t outline = buttonOutlineColor(active);
  tft.fillRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, APP_BUTTON_WIDTH, APP_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 10, APP_BUTTON_WIDTH, 1, buttonTextColor(active), fill);
}

void drawSettingsScreen() {
  appNeedsRedraw = false;
  clampSettingsScroll();
  uint16_t bg = screenBgColor();
  tft.fillScreen(bg);
  tft.fillRect(0, 0, screenWidth, headerHeight, bg);
  tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
  tft.setTextSize(1);
  tft.setTextColor(textColor(), bg);
  tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 38 : 8);
  tft.print("Settings ");
  tft.print(APP_FIRMWARE_DATE);
  drawAppNavControls();

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
    drawSettingsButton(settingsRotateX, rowY + 8, 86, "", false);
    drawRotateIcon(settingsRotateX, rowY + 8, 86, TOOLBAR_BUTTON_HEIGHT, buttonTextColor(false), buttonFillColor(false));
  }

  rowY = settingsRowScreenY(2);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(2, rowY, "Display", displayModeName());
    drawSettingsDisplayModeControl(settingsDisplayModeX, rowY + 8, 90);
  }

  rowY = settingsRowScreenY(3);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(3, rowY, "WiFi", "setup later");
  }

  rowY = settingsRowScreenY(4);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(4, rowY, "Bluetooth", "setup later");
  }

  rowY = settingsRowScreenY(5);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(5, rowY, "LED Control", beatLedEnabled ? "beat pulse" : "off");
    drawSettingsButton(settingsLedX, rowY + 8, 86, beatLedEnabled ? "BEAT" : "OFF", beatLedEnabled);
  }

  rowY = settingsRowScreenY(6);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(6, rowY, "Color", "tap");
    int swatchY = rowY + 8;
    drawSettingsSwatch(settingsColorRedX, swatchY, COLOR_RED, heartbeatLedColor.red > 0 && heartbeatLedColor.green == 0);
    drawSettingsSwatch(settingsColorYellowX, swatchY, COLOR_SIGNAL_YELLOW, heartbeatLedColor.red > 0 && heartbeatLedColor.green > 0);
    drawSettingsSwatch(settingsColorCyanX, swatchY, COLOR_CYAN, heartbeatLedColor.blue > 0);
  }

  rowY = settingsRowScreenY(7);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(7, rowY, "About", "PulseSensor CYD");
  }

  rowY = settingsRowScreenY(8);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(8, rowY, "Version", APP_VERSION);
  }

  rowY = settingsRowScreenY(9);
  if (rowY >= settingsContentTop() && rowY + SETTINGS_ROW_H <= settingsContentBottom()) {
    drawSettingsRow(9, rowY, "Firmware", APP_FIRMWARE_DATE);
  }

  drawSettingsScrollControls();
}

void drawSettingsRow(int rowIndex, int y, const char* label, const char* value) {
  uint16_t bg = settingsRowBackground(rowIndex);
  tft.fillRect(0, y, screenWidth, SETTINGS_ROW_H, bg);
  drawDottedHLine(0, y + SETTINGS_ROW_H - 2, screenWidth, gridColor(), 5, 2);

  tft.setTextSize(SETTINGS_TEXT_SIZE);
  tft.setTextColor(textColor(), bg);
  tft.setCursor(10, y + 2);
  tft.print(label);
  tft.setTextColor(displayValueTextColor(), bg);
  tft.setCursor(10, y + 21);
  tft.print(value);
}

uint16_t settingsRowBackground(int rowIndex) {
  return screenBgColor();
}

void drawSettingsButton(int x, int y, int w, const char* label, bool active) {
  uint16_t fill = buttonFillColor(active);
  uint16_t outline = buttonOutlineColor(active);
  tft.fillRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, fill);
  tft.drawRoundRect(x, y, w, TOOLBAR_BUTTON_HEIGHT, 4, outline);
  drawCenteredText(label, x, y + 6, w, 2, buttonTextColor(active), fill);
}

void drawSettingsDisplayModeControl(int x, int y, int w) {
  drawSettingsButton(x, y, w, displayModeName(), true);
}

void drawSettingsSwatch(int x, int y, uint16_t color, bool active) {
  uint16_t outline = active ? textColor() : gridColor();
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
  tft.drawFastHLine(0, settingsScrollButtonY - 4, screenWidth, gridColor());
  drawSettingsButton(settingsScrollUpX, settingsScrollButtonY, settingsScrollButtonW, "^", canScrollUp);
  drawSettingsButton(settingsScrollDownX, settingsScrollButtonY, settingsScrollButtonW, "v", canScrollDown);
}

void drawPlaceholderApp(const char* title, const char* message) {
  if (appNeedsRedraw) {
    appNeedsRedraw = false;
    uint16_t bg = screenBgColor();
    tft.fillScreen(bg);
    tft.fillRect(0, 0, screenWidth, headerHeight, bg);
    tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
    tft.setTextSize(1);
    tft.setTextColor(textColor(), bg);
    tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 38 : 8);
    tft.print(title);
    drawAppNavControls();
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
  tft.setTextColor(color, screenBgColor());
  tft.setCursor(placeholderX, placeholderY);
  tft.print(message);
  placeholderLastX = placeholderX;
  placeholderLastY = placeholderY;
}

void drawApp4PinScanner() {
  if (!appNeedsRedraw && millis() - lastPinScannerDraw < PIN_SCANNER_DRAW_MS) return;
  lastPinScannerDraw = millis();

  uint16_t bg = screenBgColor();
  int contentTop = headerHeight + 8;
  int footerY = screenHeight - 14;
  int rowAreaH = max(1, footerY - contentTop - 4);
  int rowH = max(24, rowAreaH / PIN_SCANNER_PIN_COUNT);

  if (appNeedsRedraw) {
    appNeedsRedraw = false;
    tft.fillScreen(bg);
    tft.fillRect(0, 0, screenWidth, headerHeight, bg);
    tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
    tft.setTextSize(1);
    tft.setTextColor(textColor(), bg);
    tft.setCursor(10, portraitLayout ? 38 : 8);
    tft.print("Pin Scanner");
    tft.setCursor(10, portraitLayout ? 54 : 24);
    tft.print(displayModeName());
    tft.print(" raw ADC 0..4095");
    drawAppNavControls();
  }

  int hotIndex = hottestScannerPinIndex();
  for (int i = 0; i < PIN_SCANNER_PIN_COUNT; i++) {
    bool hot = i == hotIndex && scannerPins[i].movement > HOT_MOVEMENT_MIN;
    drawPinScannerRow(i, contentTop + i * rowH, rowH, hot);
  }

  tft.fillRect(0, footerY - 1, screenWidth, 15, bg);
  tft.setTextSize(1);
  tft.setTextColor(inactiveColor(), bg);
  tft.setCursor(8, footerY);
  tft.print("Known: IO35 IO22 IO27  HOT=movement");
}

void drawPinScannerRow(int index, int y, int rowH, bool hot) {
  uint16_t bg = screenBgColor();
  uint16_t rowText = hot ? pinScannerHotColor() : textColor();
  uint16_t barColor = pinScannerBarColor(hot);
  int labelX = 8;
  int barX = portraitLayout ? 72 : 82;
  int valueX = portraitLayout ? screenWidth - 74 : screenWidth - 76;
  int railX = screenWidth - 30;
  int barW = max(38, valueX - barX - 8);
  int barH = 12;
  int barY = y + max(6, (rowH - barH) / 2);
  int value = constrain(scannerPins[index].value, 0, PIN_SCANNER_ADC_MAX_VALUE);
  int fillW = map(value, 0, PIN_SCANNER_ADC_MAX_VALUE, 0, barW);

  tft.fillRect(0, y, screenWidth, rowH, bg);
  drawDottedHLine(0, y + rowH - 1, screenWidth, gridSoftColor(), 5, 1);

  if (hot) {
    tft.fillRect(0, y + 2, 4, rowH - 4, pinScannerHotColor());
  }

  tft.setTextSize(1);
  tft.setTextColor(rowText, bg);
  tft.setCursor(labelX, y + 5);
  tft.print(scannerPins[index].label);

  tft.drawRect(barX, barY, barW, barH, gridColor());
  if (fillW > 2) {
    tft.fillRect(barX + 1, barY + 1, fillW - 2, barH - 2, barColor);
  }

  tft.setTextColor(displayValueTextColor(), bg);
  tft.setCursor(valueX, y + 2);
  tft.printf("%4d", value);

  tft.setTextColor(hot ? pinScannerHotColor() : inactiveColor(), bg);
  tft.setCursor(valueX, y + 13);
  tft.printf("d%4d", scannerPins[index].movement);

  if (hot) {
    tft.setTextColor(pinScannerHotColor(), bg);
    tft.setCursor(railX, y + 2);
    tft.print("hot");
  }

  if (isPinScannerRailed(value)) {
    tft.setTextColor(pinScannerRailColor(), bg);
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
  app3CrawlSprite.setTextColor(app3Blend565(bg, baseColor, depth), bg);
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
  tft.setTextColor(app3Blend565(bg, baseColor, depth), bg);
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
    tft.fillRect(0, 0, screenWidth, headerHeight, bg);
    tft.drawFastHLine(0, headerHeight - 1, screenWidth, gridColor());
    tft.setTextSize(1);
    tft.setTextColor(textColor(), bg);
    tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 38 : 8);
    tft.print("PulseSensor.com");
    tft.setTextColor(gold, bg);
    tft.setCursor(portraitLayout ? 10 : 10, portraitLayout ? 58 : 25);
    tft.print("Origin Story");
    drawAppNavControls();
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

  if (app3CrawlSpriteReady) {
    app3CrawlSprite.deleteSprite();
    app3CrawlSpriteReady = false;
  }

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
  tft.fillRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, panelDarkColor());
  tft.drawRoundRect(graphLeft - 2, graphTop - 2, graphWidth + 4, graphHeight + 4, 6, gridColor());
  tft.fillRect(graphLeft, graphTop, graphWidth, graphHeight, screenBgColor());

  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;

  for (int x = 0; x <= graphWidth; x += verticalGridStep) {
    drawDottedHLine(graphLeft + x, graphTop, 1, gridSoftColor(), 7, 1);
    for (int y = graphTop; y <= graphTop + graphHeight; y += 7) {
      tft.drawPixel(graphLeft + x, y, gridSoftColor());
    }
  }
  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    drawDottedHLine(graphLeft, graphTop + y, graphWidth, gridSoftColor(), 7, 1);
  }
  for (int x = 0; x < graphWidth; x += 6) {
    drawThresholdMarker(x);
  }

  tft.setTextSize(1);
  tft.setTextColor(textColor(), screenBgColor());
  tft.setCursor(graphLeft + 6, graphTop + 5);
  tft.print("LIVE LINE");

  tft.setCursor(graphLeft + graphWidth - 48, graphTop + 5);
  tft.print("THR ");
  tft.print(PULSE_THRESHOLD);

  const char* status = signalCoachText();
  int statusW = strlen(status) * 6;
  int statusX = graphLeft + graphWidth - statusW - 6;
  int statusY = graphTop + graphHeight - 14;
  tft.fillRect(statusX - 3, statusY - 2, statusW + 6, 12, screenBgColor());
  tft.setCursor(statusX, statusY);
  tft.print(status);
}

void drawGraphColumnBackground(int localX) {
  int screenX = graphLeft + localX;
  int verticalGridStep = portraitLayout ? 28 : 38;
  int horizontalGridStep = portraitLayout ? 33 : 28;
  tft.drawFastVLine(screenX, graphTop, graphHeight, screenBgColor());

  if (localX % verticalGridStep == 0) {
    for (int y = graphTop; y <= graphTop + graphHeight; y += 7) {
      tft.drawPixel(screenX, y, gridSoftColor());
    }
  }

  for (int y = 0; y <= graphHeight; y += horizontalGridStep) {
    tft.drawPixel(screenX, graphTop + y, gridSoftColor());
  }

  drawThresholdMarker(localX);
}

void drawThresholdMarker(int localX) {
  int y = signalToGraphY(PULSE_THRESHOLD);

  if (localX % 6 == 0) {
    tft.drawPixel(graphLeft + localX, y, beatColor());
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
    tft.fillCircle(graphLeft + graphX, y, 3, beatColor());
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
  tft.setTextColor(textColor(), panelBg);
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
  for (int i = 0; i < SIGNAL_QUALITY_STEPS; i++) {
    uint16_t color = inactiveColor();
    if (i < signalQuality) {
      color = lockedSignal ? signalLockColor() : signalSearchColor();
    }
    tft.fillRect(x + i * (segmentW + segmentGap), y, segmentW, segmentH, color);
  }
}

void drawAmplitudeMeter(int x, int y, int amplitude) {
  int segments = amplitudeMeterSegments(amplitude);
  int displayAmplitude = constrain(amplitude, 0, 999);

  for (int i = 0; i < 10; i++) {
    uint16_t color = inactiveColor();
    if (i < segments) {
      color = segments >= 7 ? signalLockColor() : signalSearchColor();
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
  const int clearX = centerX - HEART_MAX_SIZE - 4;
  const int clearY = centerY - HEART_MAX_SIZE - 4;
  const int clearW = (HEART_MAX_SIZE + 4) * 2 + 1;
  const int clearH = HEART_MAX_SIZE * 2 + 7;

  uint16_t heartColor = blendRed(ledBrightness);
  uint16_t outlineColor = beatColor();
  tft.fillRect(clearX, clearY, clearW, clearH, screenBgColor());
  fillHeartShape(centerX, centerY, size + 2, outlineColor);
  fillHeartShape(centerX, centerY, size, heartColor);

  lastDraw = millis();
  lastSize = size;
  lastBrightness = ledBrightness;
  beatHeartNeedsRedraw = false;
}

void fillHeartShape(int centerX, int centerY, int size, uint16_t color) {
  int lobeRadius = max(3, size * 3 / 5);
  int lobeOffset = size * 3 / 5;
  tft.fillCircle(centerX - lobeOffset, centerY - size / 3, lobeRadius, color);
  tft.fillCircle(centerX + lobeOffset, centerY - size / 3, lobeRadius, color);
  tft.fillTriangle(centerX - size * 7 / 5, centerY - size / 5,
                   centerX + size * 7 / 5, centerY - size / 5,
                   centerX, centerY + size * 4 / 5, color);
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
  return liveTraceColorForMode();
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
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_GRID;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_BG;
  return textColor();
}

uint16_t gridSoftColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_GRID_SOFT;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_BG;
  return textColor();
}

uint16_t textColor() {
  return (displayMode == DISPLAY_MONO_LIGHT || displayMode == DISPLAY_COLOR_LIGHT) ? COLOR_BG : COLOR_TEXT;
}

uint16_t displayValueTextColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_SIGNAL_YELLOW;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_BLUE;
  return textColor();
}

uint16_t buttonFillColor(bool active) {
  if (displayMode == DISPLAY_COLOR_DARK) return active ? COLOR_CYAN_DARK : COLOR_BG;
  if (displayMode == DISPLAY_COLOR_LIGHT) return active ? COLOR_LIGHT_BUTTON_FILL : COLOR_LIGHT_NAV_FILL;
  return screenBgColor();
}

uint16_t buttonOutlineColor(bool active) {
  if (displayMode == DISPLAY_COLOR_DARK) return active ? COLOR_CYAN : COLOR_GRID;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_BLUE;
  return textColor();
}

uint16_t buttonTextColor(bool active) {
  if (displayMode == DISPLAY_COLOR_LIGHT && !active) return COLOR_TEXT;
  return textColor();
}

uint16_t signalSearchColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_SIGNAL_YELLOW;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_AMBER;
  return textColor();
}

uint16_t signalLockColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_CYAN;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_TEAL;
  return textColor();
}

uint16_t inactiveColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_GRID;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_INACTIVE;
  return textColor();
}

uint16_t beatColor() {
  if (displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT) return COLOR_RED;
  return textColor();
}

uint16_t liveTraceColorForMode() {
  if (lockedSignal) return textColor();
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_ACQUIRE_BLUE;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_TRACE_BLUE;
  return textColor();
}

uint16_t pinScannerHotColor() {
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_SIGNAL_YELLOW;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_AMBER;
  return textColor();
}

uint16_t pinScannerBarColor(bool hot) {
  if (hot) return pinScannerHotColor();
  if (displayMode == DISPLAY_COLOR_DARK) return COLOR_CYAN;
  if (displayMode == DISPLAY_COLOR_LIGHT) return COLOR_LIGHT_BLUE;
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
