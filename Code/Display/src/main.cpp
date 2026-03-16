#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ─── NeoPixel ─────────────────────────────────────────────────────────────────
#define NEOPIXEL_PIN  D7   // change to whichever pin your NeoPixel data line is on
#define NEOPIXEL_NUM  1
Adafruit_NeoPixel pixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ─── OLED ─────────────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── Stepper motor ────────────────────────────────────────────────────────────
#define MOTOR_PIN1  10
#define MOTOR_PIN2   4
#define MOTOR_PIN3   3
#define MOTOR_PIN4   2
#define STEP_DELAY_MS 8

const int motorSeq[4][4] = {
  {1, 0, 1, 0},
  {0, 1, 1, 0},
  {0, 1, 0, 1},
  {1, 0, 0, 1}
};

int currentStep     = 0;
int currentPosition = 0;   // absolute step count from home

void stepMotor(int s) {
  digitalWrite(MOTOR_PIN1, motorSeq[s][0]);
  digitalWrite(MOTOR_PIN2, motorSeq[s][1]);
  digitalWrite(MOTOR_PIN3, motorSeq[s][2]);
  digitalWrite(MOTOR_PIN4, motorSeq[s][3]);
}

void motorOff() {
  // De-energise all coils to save power / heat
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  digitalWrite(MOTOR_PIN3, LOW);
  digitalWrite(MOTOR_PIN4, LOW);
}

// ─── NeoPixel color ───────────────────────────────────────────────────────────
void setLedForScore(uint8_t score) {
  if (score >= 75) {
    pixel.setPixelColor(0, pixel.Color(0, 180, 0));   // 好 → green
  } else if (score >= 50) {
    pixel.setPixelColor(0, pixel.Color(180, 100, 0)); // 中 → yellow
  } else {
    pixel.setPixelColor(0, pixel.Color(180, 0, 0));   // 差 → red
  }
  pixel.show();
}

void moveToPosition(int target) {
  int steps = target - currentPosition;
  int dir   = (steps > 0) ? 1 : -1;
  steps     = abs(steps);
  for (int i = 0; i < steps; i++) {
    currentStep     = (currentStep + dir + 4) % 4;
    currentPosition += dir;
    stepMotor(currentStep);
    delay(STEP_DELAY_MS);
  }
  motorOff();
}

void homeMotor() {
  currentPosition = 0;
  Serial.println("[Motor] Position reset to 0 (manual home assumed).");
}

#define MAX_STEPS 430

int scoreToPosition(uint8_t score) {
  return -(int)((score / 100.0f) * MAX_STEPS);
}

const char* scoreLabel(uint8_t score) {
  if (score >= 75) return "GOOD";
  if (score >= 50) return "OK";
  return "POOR";
}

// ─── Button ───────────────────────────────────────────────────────────────────
#define BUTTON_PIN 21

bool buttonPressed() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      while (digitalRead(BUTTON_PIN) == LOW);  // wait for release
      return true;
    }
  }
  return false;
}

// ─── OLED centered text helpers ───────────────────────────────────────────────
void printCentered(const char* text, int y, int textSize = 1) {
  display.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

// ─── Screen definitions ───────────────────────────────────────────────────────

void oledStatus(const char* line1, const char* line2 = nullptr) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  printCentered(line1, line2 ? 20 : 28, 1);
  if (line2) printCentered(line2, 38, 1);
  display.display();
}

// Screen 0: main score
void showScreenScore(uint8_t score) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Score label e.g. "Score"
  printCentered("Score", 6, 1);

  // Large number
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", score);
  printCentered(buf, 18, 3);

  // Rating label
  printCentered(scoreLabel(score), 52, 1);

  display.display();
}

// Screen 1: light
void showScreenLight(float lux) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  printCentered("Light", 12, 2);

  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f lux", lux);
  printCentered(buf, 38, 1);

  display.display();
}

// Screen 2: temperature
void showScreenTemp(float temp) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  printCentered("Temperature", 12, 1);

  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f C", temp);
  printCentered(buf, 30, 2);

  display.display();
}

// Screen 3: sound
void showScreenSound(float snd) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  printCentered("Sound", 12, 2);

  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f peak", snd);
  printCentered(buf, 38, 1);

  display.display();
}

// Screen 4: goodnight
void showScreenGoodnight() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  printCentered("Have A", 10, 2);
  printCentered("Good Night", 30, 2);

  display.display();
}

// ─── BLE scan ─────────────────────────────────────────────────────────────────
struct SleepData {
  bool    received = false;
  uint8_t score    = 0;
  float   lux      = 0;
  float   sound    = 0;
  float   temp     = 0;
};

SleepData gData;

class SleepCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    if (!dev.haveName() || dev.getName() != "SlpSns") return;
    if (!dev.haveManufacturerData()) return;

    std::string mfr = dev.getManufacturerData();
    // Strip 2-byte company ID prefix, rest is CSV string
    if (mfr.size() < 3) return;
    std::string csv = mfr.substr(2);

    Serial.printf("[BLE] Raw CSV: [%s]\n", csv.c_str());

    // Parse "score,lux,snd,temp"
    float parsedScore, parsedLux, parsedSnd, parsedTemp;
    int matched = sscanf(csv.c_str(), "%f,%f,%f,%f",
                         &parsedScore, &parsedLux, &parsedSnd, &parsedTemp);
    if (matched != 4) {
      Serial.printf("[BLE] Parse failed, matched=%d\n", matched);
      return;
    }

    gData.score    = (uint8_t)constrain((int)parsedScore, 0, 100);
    gData.lux      = parsedLux;
    gData.sound    = parsedSnd * 100.0f;   // was divided by 100 on sensor side
    gData.temp     = parsedTemp;
    gData.received = true;

    Serial.printf("[BLE] Parsed: score=%d lux=%.1f snd=%.0f temp=%.1f\n",
                  gData.score, gData.lux, gData.sound, gData.temp);

    BLEDevice::getScan()->stop();
  }
};

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Display Unit (Demo Mode) ===");

  // NeoPixel init
  pixel.begin();
  pixel.setBrightness(80);  // 0–255, keep moderate for classroom
  pixel.clear();
  pixel.show();

  // Button init
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Motor init
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_PIN3, OUTPUT);
  pinMode(MOTOR_PIN4, OUTPUT);

  // OLED init
  Wire.begin(D4, D5);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[ERROR] SSD1306 not found!");
    while (true);
  }
  oledStatus("Set pointer to", "POOR position!");
  delay(3000);   // 给你3秒时间手动拨指针
  homeMotor();
  oledStatus("Scanning BLE...", "Waiting for sensor");

  // BLE init
  BLEDevice::init("SleepDisplay");
  Serial.println("[OK] BLE ready — scanning for SleepSensor");
}

// ─── State ────────────────────────────────────────────────────────────────────
bool motorMoved  = false;
int  screenIndex = 0;   // 0=Score 1=Light 2=Temp 3=Sound 4=Goodnight

void showCurrentScreen() {
  switch (screenIndex) {
    case 0: showScreenScore(gData.score);   break;
    case 1: showScreenLight(gData.lux);     break;
    case 2: showScreenTemp(gData.temp);     break;
    case 3: showScreenSound(gData.sound);   break;
    case 4: showScreenGoodnight();          break;
    default: showScreenGoodnight();         break;
  }
}

void resetAll() {
  gData.received  = false;
  gData.score     = 0;
  gData.lux       = 0;
  gData.sound     = 0;
  gData.temp      = 0;
  motorMoved      = false;
  screenIndex     = 0;

  // Return motor to home position
  moveToPosition(0);

  // Turn off LED
  pixel.clear();
  pixel.show();

  Serial.println("[Reset] Ready for next scan.");
}

void loop() {
  // ── Phase 1: waiting for BLE data ─────────────────────────────────────────
  if (!gData.received) {
    oledStatus("Scanning BLE...", "Waiting for sensor");
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new SleepCallback(), true);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    scan->start(5, false);
    scan->clearResults();
    return;
  }

  // ── Phase 2: first time data arrives — move motor + show score ────────────
  if (!motorMoved) {
    setLedForScore(gData.score);

    int target = scoreToPosition(gData.score);
    Serial.printf("[Motor] Moving to position %d (score=%d)\n", target, gData.score);
    moveToPosition(target);

    screenIndex = 0;
    showCurrentScreen();
    motorMoved = true;
    Serial.println("[Done] Score displayed. Press button to cycle metrics.");
    return;
  }

  // ── Phase 3: button cycles through screens ────────────────────────────────
  if (buttonPressed()) {
    if (screenIndex < 4) {
      screenIndex++;
      showCurrentScreen();
      Serial.printf("[Button] Screen %d\n", screenIndex);
    } else {
      // On goodnight screen — reset and start over
      resetAll();
    }
  }

  delay(20);
}