#include <Arduino.h>

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

void setup() {
  Serial.begin(115200);
  Serial.println("Detecting I2C clock signal...");
  buffer = (char *) malloc(2048);
  if (buffer == NULL) {
    Serial.println("[ERR] Failed to allocate memory for buffer");
    return;
  }
}

void loop() {

}