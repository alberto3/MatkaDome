#include <Arduino.h>
#include <EEPROM.h>

static const uint8_t Index = 0;
static const uint8_t K = 3;
static const uint8_t N = 4;
static const uint8_t Value = (N << 4) | K;

void setup() {
    Serial.begin(115200);
    while (!Serial);
    EEPROM.update(Index, Value);
    Serial.print(Value);
    Serial.println(F(" is written to EEPROM"));
}

void loop() {
}
