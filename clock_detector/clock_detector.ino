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

void setup() {
  Serial.begin(115200);
  Serial.println("Detecting I2C clock signal...");
}

void wait_clock_until_up_or_down(bool up) {
    bool exit_condition = up;
    while (SCL != exit_condition) {
    }
}

void loop() {
    unsigned long ts = millis();
    while (true) {
        wait_clock_until_up_or_down(true);
        Serial.print("Time for clk to go up (ms): ");
        Serial.println(millis() - ts);
        ts = millis();
        wait_clock_until_up_or_down(false);
        Serial.print("Time for clk to go down (ms): ");
        Serial.println(millis() - ts);
        ts = millis();
    }
}