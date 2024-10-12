#include <EEPROM.h>

#define MODE "read"

void setup() {
  Serial.begin(9600);
  delay(5000);

  Serial.println(">>>");
  for (int i = 0; i < EEPROM.length(); i += 1) {
    if (MODE == "update") {
      EEPROM.update(i, 0);
    }
    Serial.print(i);
    Serial.print(":");
    Serial.print(EEPROM.read(i));
    Serial.println();
    delay(10);
  }
  Serial.println("<<<");
}

void loop() {
}