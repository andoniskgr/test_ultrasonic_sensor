#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>

static constexpr uint8_t I2C_SDA = 2;
static constexpr uint8_t I2C_SCL = 0;

static constexpr uint8_t PCF_ADDR = 0x20;   // A0/A1/A2 = GND
static constexpr uint8_t BUTTON_PIN = P4;   // P4: pulled up, pressed -> GND

PCF8574 pcf(PCF_ADDR, I2C_SDA, I2C_SCL);
static bool pcfReady = false;
static bool buttonStateKnown = false;
static uint8_t lastButton = HIGH; // 1=released, 0=pressed

static uint8_t readPcfRawByte() {
  uint8_t b = 0xFF;
  Wire.requestFrom((int)PCF_ADDR, 1);
  if (Wire.available()) {
    b = (uint8_t)Wire.read();
  }
  return b;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  // Check device ACK (ground truth).
  Wire.beginTransmission(PCF_ADDR);
  const uint8_t ackErr = Wire.endTransmission();
  const bool ackOk = (ackErr == 0);

  const bool libOk = pcf.begin(0xFF);
  pcfReady = (ackOk || libOk);

  if (pcfReady) {
    Serial.println("PCF connected!");
  } else {
    Serial.println("PCF not connected.");
  }

  if (pcfReady) {
    pcf.setLatency(0);
    pcf.pinMode(BUTTON_PIN, INPUT_PULLUP);
    pcf.digitalWrite(BUTTON_PIN, HIGH);

    // Ensure all port latches are HIGH (safe default for inputs/idle).
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
  }
}

void loop() {
  if (!pcfReady) {
    delay(1000);
    return;
  }

  const uint8_t raw = readPcfRawByte();
  const uint8_t cur = (raw >> 4) & 0x01; // P4 bit: 1=released, 0=pressed

  if (!buttonStateKnown) {
    lastButton = cur;
    buttonStateKnown = true;
    delay(10);
    return;
  }

  // Print once per press (released->pressed).
  if (lastButton == HIGH && cur == LOW) {
    Serial.println("button pressed");
  }
  lastButton = cur;
  delay(10);
}