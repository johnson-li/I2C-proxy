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
unsigned long last_print_ts = 0;
unsigned int print_interval = 2000; // Print every 2 seconds
unsigned int print_index = 1;
unsigned int buffer_size = 0;
unsigned int buffer_limit = 0;

uint16_t availableMemory() {
  int size = 2048; // start with the most
  char *buf;
  while ((buf = (char *) malloc(--size)) == NULL) ;
  free(buf);
  return size;
}

void setup() {
  start_time = millis();
  Serial.begin(115200);
  Serial.println("Monitoring I2C communication...");
  int memory_size = availableMemory();
  buffer_limit = memory_size - 128;
  buffer = (char *) malloc(buffer_limit);
  Serial.print("Allocated ");
  Serial.print(buffer_limit);
  Serial.println(" bytes for the buffer.");
}

void print() {
  for (int i = 0; i < buffer_size; i++) {
    unsigned char b = buffer[i];
    unsigned char condition = b >> 4;
    unsigned char data = b & 0x01;
    if (condition == 0) {
      Serial.print("BIT: ");
      Serial.println(data);
    } else if (condition == 1) {
      Serial.println("START");
    } else if (condition == 2){
      Serial.println("STOP");
    } else {
      Serial.println("ERROR: Unknown condition");
    }
  }
  buffer_size = 0;
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
      // SCL is low, optionally record SDA/condition and reset the condition 
      if (condition >= 0) {
        if (buffer_size >= buffer_limit) {
          Serial.println("ERROR: Buffer overflow");
        } else {
          buffer[buffer_size++] = (condition << 4) | data;
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

    // Print the data if the interval is reached
    if (millis() - start_time > print_interval * print_index) {
      print();
      print_index++;
    }
  }
}