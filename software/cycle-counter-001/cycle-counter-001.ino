#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <EEPROM.h>
#include <Adafruit_ADS1X15.h>

#define TICKS_NUM 31250  // pre-256 -> 31250 = 500 ms

#define SENS_POLLING_INTERVAL 100

#define MODULE_1_ADDRESS 0x48
#define CHANNEL_1 0

#define OLED_MOSI 11  // SDA pin D11 (default)
#define OLED_CLK 13   // SCK pin D13 (default)
#define OLED_DC 9     // pin D9 - user defined pin
#define OLED_CS 8     // pin D8 - user defined pin (chip select)
#define OLED_RST 10   // RES pin D10 - user defined pin
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define TIME_FACTOR 1.3 / (CYCLES_PER_SAVE * 1000)  // 1.25 - time coeff; 1000 ms to s

#define UPPER_THRESHOLD 12790  // pressure 760
#define LOWER_THRESHOLD 12000  // pressure 720

// Regression params - y(x) = k * x + b
#define K_PARAM 0.075
#define B_PARAM -200  // default -198.907

#define MEMO_DATA_SIZE 3  // bytes
#define MEMO_BLOCKS 341
#define MEMO_CELLS_NUM (MEMO_DATA_SIZE * MEMO_BLOCKS)  // 341 * 3 = 1023 | EEPROM size 1024 bytes for Atmega328p
#define CYCLES_PER_SAVE 10

unsigned int memo_pos = 0;  // first byte in EEPROM which stores Count (2bytes) & CRC (1byte)

volatile unsigned int sens_0_value = 0;

unsigned long sens_read_t0;
unsigned long sens_read_t1;

volatile unsigned int count = 0;
bool sens_low_lvl_flag = true;

unsigned long cycle_t0 = 0;          // initial time for CYCLES_PER_SAVE
volatile unsigned long cycle_T = 0;  // time period of CYCLES_PER_SAVE

// ADS1115 module
Adafruit_ADS1115 converterModule;

// OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(DISPLAY_WIDTH, DISPLAY_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

void setup() {
  get_count();

  // Start converter
  converterModule.setDataRate(RATE_ADS1115_64SPS);  // 128SPS - default
  converterModule.begin();

  setupTimer1();

  // Start OLED
  display.begin(0, true);  // we dont use the i2c address but we will reset!
  display.clearDisplay();

  sens_read_t0 = millis();
}

void loop() {
  sens_read_t1 = millis();

  if (sens_read_t1 - sens_read_t0 >= SENS_POLLING_INTERVAL) {
    sens_read_t0 = sens_read_t1;

    sens_0_value = converterModule.readADC_SingleEnded(CHANNEL_1);

    if (sens_low_lvl_flag && sens_0_value > UPPER_THRESHOLD) {
      sens_low_lvl_flag = false;
    }

    if (!sens_low_lvl_flag && sens_0_value < LOWER_THRESHOLD) {
      sens_low_lvl_flag = true;
      count += 1;

      // if count reach number of cycle to save & excluding the case of count == 0
      if (!(count % CYCLES_PER_SAVE) && count != 0) {
        save_count();

        // Update cycle time
        cycle_T = sens_read_t1 - cycle_t0;
        cycle_t0 = sens_read_t1;
      }
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 0);

  display.print("> ");
  display.println(count);

  display.setTextSize(1);
  display.println();
  display.setTextSize(2);

  display.print("S: ");
  display.println((float)sens_0_value * K_PARAM + B_PARAM);
  // display.println(sens_0_value);

  display.setTextSize(1);
  display.println();
  display.setTextSize(2);

  display.print("T = ");
  if (cycle_T == 0) {
    display.println("n-a");
  } else {
    display.println((float)cycle_T * TIME_FACTOR);
  }

  display.display();
}

void setupTimer1(void) {
  noInterrupts();
  TCCR1A = 0;               // init Timer1 - clear reg A
  TCCR1B = 0;               // init Timer1 - clear reg B
  TCNT1 = 0;                // Reset current timer counter
  OCR1A = TICKS_NUM;        // timer COMPA register
  TCCR1B |= (1 << WGM12);   // Enable CTC (Clear Timer on Compare Match)
  TCCR1B |= (1 << CS12);    // Prescaler 256
  TIMSK1 |= (1 << OCIE1A);  // Enable interrupt on coincidence
  interrupts();
}

void get_count(void) {
  for (unsigned int pos = 0; pos < MEMO_CELLS_NUM; pos += MEMO_DATA_SIZE) {
    unsigned int read_value;
    EEPROM.get(pos, read_value);

    if (read_value * CYCLES_PER_SAVE > count) {
      count = read_value * CYCLES_PER_SAVE;
      memo_pos = pos;
    }
  }
}

void save_count(void) {
  memo_pos += MEMO_DATA_SIZE;
  if (memo_pos >= MEMO_CELLS_NUM) {
    memo_pos = 0;
  }
  unsigned int write_value = count / CYCLES_PER_SAVE;
  EEPROM.put(memo_pos, write_value);
}