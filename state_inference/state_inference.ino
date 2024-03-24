#include <Arduino.h>

// Descrition of the I2C protocol: 
// https://www.circuitbasics.com/basics-of-the-i2c-communication-protocol/

// SCL  D2  00000100  0x04
// SDA  D3  00001000  0x08
// SCL+SDA  00001100  0x0c

#define SCL_MASK 0x04
#define SDA_MASK 0x08
#define I2C_MASK 0x0c

#define SCL (byte) (PIND & SCL_MASK)
#define SDA (byte) ((PIND >> 3) & 0x01)
#define I2C (byte) (PIND & I2C_MASK)

// the sample buffer
char *buffer;
unsigned long start_time;

void setup() {
  start_time = millis();
  Serial.begin(115200);
  Serial.println("Detecting I2C clock signal...");
  buffer = (char *) malloc(2048);
  if (buffer == NULL) {
    Serial.println("[ERR] Failed to allocate memory for buffer");
    return;
  }
}

void log(String &msg) {
  unsigned long elapsed = millis() - start_time;
  Serial.print(elapsed);
  Serial.print(" - ");
  Serial.println(msg);
}

// void loop0() {
//   // The log [H, L] means that the SCL is high and the SDA is low

//   // Step 0: both SCL and SDA are high
//   log("Waiting for the start condition [H, H]");
//   while (I2C != I2C_MASK) {
//     // Wait for the initial condition
//   }

//   // Step 1: SDA becomes low
//   log("Waiting for [H, L]");
//   while (I2C != SCL_MASK) {
//     // Wait for SDA to become low
//   }

//   // Step 2: SCL becomes low
//   log("Waiting for [L, L]");
//   while (I2C != 0) {
//     // Wait for SCL to become low
//     if (I2C == SDA_MASK) {
//       log("SDA is high, aborting");
//       return;
//     }
//   }

//   // Step 3: SCL pulse
//   while (true) {
//     while (!SCL) {
//       // Wait for [H, X]
//     }
//     // [H, X], it is time to read data
//     char data = 0;
//     bool first = true;
//     while (SCL) {
//       if (!first) {
//         if (!data && SDA) {
//           // [H, H], STOP condition
//           log("Termination condition reached");
//           return;
//         }
//       }
//       data = SDA;
//       first = false;
//     }
//     // [L, X], data is ready
//   }
// }

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

  // Read the signals one by one. 
  // Each signal is either a bit or a start/stop condition
  while (true) {
    // Wait for the SCL to be high
    while (!SCL) { }

    // SCL is high, read the data
    unsigned char data = 0;
    bool first = true;
    char condition = 0; // 0: normal data, 1: START, 2: STOP
    while (SCL) {
      if (!first) {
        if (data != SDA) {
          if (condition > 0) {
            // SDA should not change twice during a high SCL
            Serial.println("ERROR: SDA changes twice during a high SCL");
          }
          if (data) {
            condition = 1;
          } else {
            condition = 2;
          }
        }
      }
      data = SDA;
      first = false;
    }
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
}