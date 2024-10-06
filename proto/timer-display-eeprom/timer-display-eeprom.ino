#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SENS_0_PIN A0
#define SENS_POLLING_INTERVAL 10

#define TICKS_NUM 31250  // pre-256 -> 31250 = 500 ms

#define OLED_MOSI 11  // MOSI pin 11 (default)
#define OLED_CLK 13   // SCK pin 13 (default)
#define OLED_DC 9     // pin 9 - user defined pin
#define OLED_CS 8     // pin 8 - user defined pin
#define OLED_RST 10   // pin 10 - user defined pin
#define DISPLAY_W 128
#define DISPLAY_H 64

// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(DISPLAY_W, DISPLAY_H, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

volatile unsigned int sens_0 = 0;
unsigned long sens_read_t0 = 0;

volatile unsigned int count = 0;
bool sens_low_lvl_flag = true;

unsigned long cycle_t0 = 0;          // t0 for 5 cycles
volatile unsigned long cycle_T = 0;  // period of 5 cycles

void setup() {
  pinMode(SENS_0_PIN, INPUT);

  noInterrupts();
  TCCR1A = 0;               // init Timer1 - clear reg A
  TCCR1B = 0;               // init Timer1 - clear reg B
  TCNT1 = 0;                // Reset current timer counter
  OCR1A = TICKS_NUM;        // timer COMPA register
  TCCR1B |= (1 << WGM12);   // Enable CTC (Clear Timer on Compare Match)
  TCCR1B |= (1 << CS12);    // Prescaler 256
  TIMSK1 |= (1 << OCIE1A);  // Enable interrupt on coincidence
  interrupts();

  // Start OLED
  display.begin(0, true);  // we dont use the i2c address but we will reset!
  display.clearDisplay();
}

void loop() {
  unsigned long sens_read_t1 = millis();

  if (sens_read_t1 - sens_read_t0 >= SENS_POLLING_INTERVAL) {
    sens_read_t0 = sens_read_t1;


    sens_0 = analogRead(SENS_0_PIN);

    if (sens_low_lvl_flag && sens_0 > 700) {
      sens_low_lvl_flag = false;
    }

    if (!sens_low_lvl_flag && sens_0 < 200) {
      sens_low_lvl_flag = true;
      count += 1;

      if (!(count % 5) && count != 0) {
        cycle_T = sens_read_t1 - cycle_t0;
        cycle_t0 = sens_read_t1;
      }
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  display.clearDisplay();

  // text display tests
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 0);
  display.print("> ");
  display.println(count);
  display.println();
  display.setTextSize(1);
  display.print("sens: ");
  display.println(sens_0);
  display.println();
  display.print("T5: ");
  // 1.22 - time coeff; 5 cycles; millis to s - /1000
  float time_per_cycle = (float)cycle_T * 1.22 / 5000;
  display.println(time_per_cycle);
  display.display();
}
