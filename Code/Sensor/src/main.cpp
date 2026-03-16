#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_BMP280.h>
#include <driver/i2s.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

// ─── Pin definitions ──────────────────────────────────────────────────────────
#define STATUS_LED  D6
#define I2S_BCLK    D0
#define I2S_LRCK    D1
#define I2S_DOUT    D2

// ─── Demo config ──────────────────────────────────────────────────────────────
#define NUM_READINGS   5
#define READING_DELAY  5000   // ms between readings

// ─── BLE config ───────────────────────────────────────────────────────────────
#define SLEEP_SERVICE_UUID  "12345678-1234-1234-1234-123456789012"

// ─── Sensor objects ───────────────────────────────────────────────────────────
Adafruit_VEML7700 veml;
Adafruit_BMP280   bmp;
bool vemlOK = false;
bool bmpOK  = false;

// ─── I2S setup for SPH0645 ────────────────────────────────────────────────────
void setupI2S() {
  i2s_config_t cfg = {
    .mode               = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate        = 16000,
    .bits_per_sample    = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format     = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags   = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count      = 4,
    .dma_buf_len        = 256,
    .use_apll           = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk         = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num    = D0,
    .ws_io_num     = D1,
    .data_out_num  = I2S_PIN_NO_CHANGE,
    .data_in_num   = D2
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

int noiseFloor = 0;   // measured at startup

void calibrateNoiseFloor() {
  Serial.println("[Mic] Calibrating noise floor...");
  // Discard first few frames to let mic stabilize
  int32_t discard[256];
  size_t dummy;
  for (int i = 0; i < 5; i++) {
    i2s_read(I2S_NUM_0, discard, sizeof(discard), &dummy, 100);
  }
  // Measure baseline over 3 readings
  long sum = 0;
  for (int r = 0; r < 3; r++) {
    int32_t samples[256];
    size_t bytes_read;
    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, 100);
    int count = bytes_read / sizeof(int32_t);
    double sumSq = 0;
    for (int i = 0; i < count; i++) {
      double val = (double)(samples[i] >> 14);
      sumSq += val * val;
    }
    sum += (int)sqrt(sumSq / count);
  }
  noiseFloor = sum / 3;
  Serial.printf("[Mic] Noise floor = %d\n", noiseFloor);
}

int readMicLevel() {
  int32_t samples[256];
  size_t  bytes_read;
  i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, 100);

  int count = bytes_read / sizeof(int32_t);
  if (count == 0) return 0;

  double sumSquares = 0;
  for (int i = 0; i < count; i++) {
    double val = (double)(samples[i] >> 14);
    sumSquares += val * val;
  }
  int rms = (int)sqrt(sumSquares / count);

  // Subtract noise floor, clamp to 0
  return max(0, rms - noiseFloor);
}

// ─── Threshold Classifier ────────────────────────────────────────────────────

typedef enum { LVL_POOR = 0, LVL_OK = 1, LVL_GOOD = 2 } SensorLevel;

SensorLevel classifyLight(float lux) {
  if (lux < 5.0f)  return LVL_GOOD;
  if (lux < 20.0f) return LVL_OK;
  return LVL_POOR;
}

SensorLevel classifySound(float peak) {
  if (peak < 500)  return LVL_GOOD;   // quiet after noise floor removed
  if (peak < 2000) return LVL_OK;     // moderate
  return LVL_POOR;                     // loud
}

SensorLevel classifyTemp(float temp) {
  if (temp >= 18.0f && temp <= 22.0f) return LVL_GOOD;
  if (temp >= 15.0f && temp <= 25.0f) return LVL_OK;
  return LVL_POOR;
}

const char* levelName(SensorLevel l) {
  if (l == LVL_GOOD) return "GOOD";
  if (l == LVL_OK)   return "OK";
  return "POOR";
}

// Convert classified levels → final score (0–100)
// Weights: Light 40%, Sound 40%, Temperature 20%
int calcScore(float avgLux, float avgSound, float avgTemp) {
  SensorLevel lightLevel = classifyLight(avgLux);
  SensorLevel soundLevel = classifySound(avgSound);
  SensorLevel tempLevel  = classifyTemp(avgTemp);

  Serial.printf("  Light class:  %s\n", levelName(lightLevel));
  Serial.printf("  Sound class:  %s\n", levelName(soundLevel));
  Serial.printf("  Temp  class:  %s\n", levelName(tempLevel));

  // Each level maps to a score fraction: POOR=0, OK=0.5, GOOD=1.0
  float lightScore = (lightLevel / 2.0f) * 40.0f;
  float soundScore = (soundLevel / 2.0f) * 40.0f;
  float tempScore  = (tempLevel  / 2.0f) * 20.0f;

  int total = (int)(lightScore + soundScore + tempScore);
  return constrain(total, 0, 100);
}

// ─── BLE broadcast ────────────────────────────────────────────────────────────

void broadcastScore(uint8_t score, float lux, float snd, float temp) {
  BLEDevice::init("SleepSensor");

  // Keep payload short to fit BLE 31-byte advertisement limit.
  // Format: "score,lux,snd,temp" with reduced precision
  // e.g. "46,17,139,20" (sound divided by 100, reconstructed on display)
  char payload[24];
  snprintf(payload, sizeof(payload), "%d,%d,%d,%d",
           (int)score,
           (int)lux,
           (int)(snd / 100),   // divide by 100 to keep short; display multiplies back
           (int)temp);

  std::string mfr;
  mfr += (char)0xFF;
  mfr += (char)0xFF;
  mfr += std::string(payload);

  BLEAdvertisementData advData;
  advData.setName("SlpSns");   // shorter name saves bytes
  advData.setManufacturerData(mfr);

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->start();

  Serial.printf("\n>>> BLE broadcasting payload: [%s]\n", payload);
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Sensor Unit (Demo Mode) ===");

  // Status LED on during init
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  delay(2000);
  digitalWrite(STATUS_LED, LOW);

  Wire.begin(D4, D5);
  delay(100);

  if (!veml.begin(&Wire)) {
    Serial.println("[WARN] VEML7700 not found");
  } else {
    Serial.println("[OK]  VEML7700 ready");
    vemlOK = true;
  }

  if (!bmp.begin(0x76)) {
    Serial.println("[WARN] BMP280 not found");
  } else {
    Serial.println("[OK]  BMP280 ready");
    bmpOK = true;
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  }

  setupI2S();
  calibrateNoiseFloor();
  Serial.println("[OK]  SPH0645 ready");
  Serial.printf("\nStarting %d readings, %d s apart...\n\n", NUM_READINGS, READING_DELAY / 1000);
}

bool done = false;

void loop() {
  if (done) {
    delay(1000);
    return;
  }

  float sumLux   = 0;
  float sumSound = 0;
  float sumTemp  = 0;
  int   validLux = 0, validTemp = 0, validSound = 0;

  for (int r = 0; r < NUM_READINGS; r++) {
    Serial.printf("--- Reading %d/%d ---\n", r + 1, NUM_READINGS);

    // Light
    float lux = 0;
    if (vemlOK) {
      lux = veml.readLux();
      sumLux += lux;
      validLux++;
    }
    Serial.printf("  Light:  %.2f lux\n", lux);

    // Temperature
    float temp = 20.0f;
    if (bmpOK) {
      temp = bmp.readTemperature();
      sumTemp += temp;
      validTemp++;
    }
    Serial.printf("  Temp:   %.2f C\n", temp);

    // Sound — skip first reading (mic not yet stable)
    int snd = readMicLevel();
    Serial.printf("  Sound:  %d\n", snd);
    if (r > 0) {
      sumSound += snd;
      validSound++;
    } else {
      Serial.println("  (first sound reading discarded — mic warmup)");
    }

    // Blink LED each reading
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);

    if (r < NUM_READINGS - 1) delay(READING_DELAY - 200);
  }

  // Averages
  float avgLux   = validLux   ? sumLux   / validLux   : 0;
  float avgTemp  = validTemp  ? sumTemp  / validTemp  : 20.0f;
  float avgSound = validSound ? sumSound / validSound : 0;

  uint8_t score = (uint8_t)calcScore(avgLux, avgSound, avgTemp);

  Serial.println("\n=== Results ===");
  Serial.printf("  Avg lux:   %.2f\n", avgLux);
  Serial.printf("  Avg sound: %.1f\n", avgSound);
  Serial.printf("  Avg temp:  %.2f C\n", avgTemp);
  Serial.printf("  SCORE:     %d / 100\n", score);

  // Broadcast via BLE
  broadcastScore(score, avgLux, avgSound, avgTemp);

  // Keep LED solid to indicate done
  digitalWrite(STATUS_LED, HIGH);
  done = true;
  Serial.println("\nBLE broadcasting. Done.");
}