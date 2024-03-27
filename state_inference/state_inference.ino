#include <Arduino.h>

// Descrition of the I2C protocol: 
// https://www.circuitbasics.com/basics-of-the-i2c-communication-protocol/

// SCL  D2  00000100  0x04
// SDA  D3  00001000  0x08
#define SCL_PIN 2
#define SDA_PIN 3

// the sample buffer
char *buffer;
unsigned long start_time;

void setup() {
  start_time = millis();
  Serial.begin(115200);
  Serial.println("Monitoring I2C communication...");
  // buffer = (char *) malloc(2048);
  // if (buffer == NULL) {
  //   Serial.println("[ERR] Failed to allocate memory for buffer");
  //   return;
  // }
}

void loop() {
  // Background information: Data Line (SDA) Changes
  // Data Stability: 
  //   The data on the SDA line must be stable during the HIGH period of the clock (SCL). 
  //   This means that the value of the data line (SDA) cannot change while the clock line (SCL) is HIGH.
  // Data Change: 
  //   The SDA line can only change when the SCL line is LOW. 
  //   This ensures that data transitions do not occur during the clock's HIGH period, 
  //   preventing misinterpretation of the data.

  // Insight of inferencing the I2C protocol:
  // All data reading happens when SCL is high
  // 1. If the data keeps the same value, it is normal data reading
  // 2. If the data changes, it is a START or STOP condition
  //   2.1 If data changes from 1 to 0, it is a START condition
  //   2.2 If data changes from 0 to 1, it is a STOP condition

  unsigned char data = 0;
  char condition = -1; // -1, uninitialized, 0: normal data, 1: START, 2: STOP

  // Read the signals one by onerdar. 
  // Each signal is either a bit or a start/stop condition
  while (true) {
    // Read SCL and SDA
    unsigned char signal = PIND;
    unsigned char scl = (signal >> SCL_PIN) & 0x01;
    unsigned char sda = (signal >> SDA_PIN) & 0x01;

    // Do not process SDA if the SCL is low
    if (!scl) {
      // SCL is low, optionally print SDA/condition and reset the condition 
      if (condition >= 0) {
        unsigned long elapsed = millis() - start_time;
        Serial.print(elapsed);
        Serial.print(" - ");
        if (condition == 0) {
          Serial.print("BIT: ");
          Serial.println(data);
        } else if (condition == 1) {
          Serial.println("START");
        } else {
          Serial.println("STOP");
        }
      }
      condition = -1;
    } else {
      // SCL is high, read SDA
      if (condition >= 0) {
        if (data != sda) {
          if (condition > 0) {
            // SDA should not change twice during a high SCL
            Serial.println("ERROR: SDA changes twice during a high SCL");
          } else {
            if (data) {
              condition = 1;
            } else {
              condition = 2;
            }
          }
        }
      } else {
        condition = 0;
      }
      data = sda;
    }
  }
}