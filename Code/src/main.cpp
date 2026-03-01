#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VEML7700.h>

// Pin definitions
#define THERMISTOR_PIN A0
#define MIC_PIN        A1
#define STATUS_LED     D6

Adafruit_VEML7700 veml;
bool vemlOK = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Sensor Unit Starting ===");

  // Turn on status LED immediately on boot
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  Serial.println("Status LED on");

  // Initialize I2C with explicit SDA/SCL pins
  Wire.begin(D4, D5);
  delay(100);

  // Initialize VEML7700 light sensor
  if (!veml.begin(&Wire)) {
    Serial.println("VEML7700 not found, check wiring!");
  } else {
    Serial.println("VEML7700 initialized successfully");
    vemlOK = true;
  }
}

void loop() {
  // Read light level from VEML7700
  if (vemlOK) {
    float lux = veml.readLux();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lux");
  } else {
    Serial.println("Light: skipped (sensor not found)");
  }

  // Read thermistor (raw ADC value, replace with BME280 later)
  int thermRaw = analogRead(THERMISTOR_PIN);
  Serial.print("Temperature (raw): ");
  Serial.println(thermRaw);

  // Read microphone peak value (replace with SPH0645 later)
  int micMax = 0;
  for (int i = 0; i < 100; i++) {
    int val = analogRead(MIC_PIN);
    if (val > micMax) micMax = val;
  }
  Serial.print("Microphone (peak): ");
  Serial.println(micMax);

  Serial.println("---");
  delay(2000);
}