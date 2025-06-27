/*
MODE description:
"read" - read eeprom and print to IDE
"clear" - clear eeprom to all zeroes
"set_count" - clear and set value
*/

#include <EEPROM.h>

#define MODE "read"

const unsigned int COUNT = 0  // MAX VALUE 65535

void setup() {
  Serial.begin(9600);
  delay(5000);

  Serial.println(">>>");
  if (MODE == "set_count") {
    EEPROM.put(0, COUNT);
    Serial.println("<<<");
  } else {
    for (int i = 0; i < EEPROM.length(); i += 1) {
      if (MODE == "read") {
        Serial.print(i);
        Serial.print(":");
        Serial.print(EEPROM.read(i), HEX);
        Serial.println();
        delay(10);
      }

      if (MODE == "clear") {
        EEPROM.update(i, 0);
      }
    }
    Serial.println("<<<");
  }
}

void loop() {
}