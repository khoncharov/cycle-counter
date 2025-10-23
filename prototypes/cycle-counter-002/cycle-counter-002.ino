#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <EEPROM.h>
#include <Adafruit_ADS1X15.h>

#define TICKS_NUM 25000  // 400 ms

#define SENS_POLLING_INTERVAL 100

#define MODULE_1_ADDRESS 0x48
#define CHANNEL_1 0

#define I2C_ADDRESS 0x3c
#define OLED_RESET -1
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define TIME_FACTOR 0.0013f  // time coeff = 1.3 for 1 ms

#define UPPER_THRESHOLD 12800
#define LOWER_THRESHOLD 11900

// Regression coeffs - y(x) = k * x + b
#define K_COEFF 0.075f
#define B_COEFF -199.0f  // default -198.907

#define MEMO_DATA_SIZE 3  // bytes
#define MEMO_BLOCKS 341
#define MEMO_CELLS_NUM (MEMO_DATA_SIZE * MEMO_BLOCKS)  // 341 * 3 = 1023 | EEPROM size 1024 bytes for Atmega328p
#define CYCLES_PER_SAVE 10

unsigned int memo_pos = 0;  // first byte in EEPROM which stores Count (2bytes) & CRC (1byte)

unsigned int sens_0_value = 0;
unsigned int sens_smoothed = 0;
float smoothing_factor = 0.3f;


unsigned long time_t0;
unsigned long time_t1;

unsigned long count = 0;
bool sens_flag = true;

unsigned long cycle_t0 = 0;  // initial time for CYCLES_PER_SAVE
unsigned long cycle_T = 0;   // time period of CYCLES_PER_SAVE

volatile bool need_display_update = false;

// ADS1115 module
Adafruit_ADS1115 converterModule;

// OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

void setup() {
  get_count();

  // Start converter
  converterModule.setDataRate(RATE_ADS1115_64SPS);  // 128SPS - default
  converterModule.begin();

  setupTimer1();

  // Start OLED
  delay(250);
  display.begin(I2C_ADDRESS, true);
  display.display();
  delay(500);

  time_t0 = millis();
  cycle_t0 = time_t0;
}

void loop() {
  time_t1 = millis();

  if (time_t1 - time_t0 >= SENS_POLLING_INTERVAL) {
    time_t0 = time_t1;

    sens_0_value = converterModule.readADC_SingleEnded(CHANNEL_1);
    sens_smoothed = (unsigned int)(smoothing_factor * (float)sens_0_value + (1.0f - smoothing_factor) * (float)sens_smoothed);

    if (sens_flag && sens_smoothed > UPPER_THRESHOLD) {
      sens_flag = false;
    }

    if (!sens_flag && sens_smoothed < LOWER_THRESHOLD) {
      sens_flag = true;
      count += 1;

      // Update cycle period
      cycle_T = time_t1 - cycle_t0;
      cycle_t0 = time_t1;

      // if count reach number of cycle to save & excluding the case of count == 0
      if (!(count % CYCLES_PER_SAVE) && count != 0) {
        save_count();
      }
    }
  }

  if (need_display_update) {
    need_display_update = false;
    update_display();
  }
}

ISR(TIMER1_COMPA_vect) {
  need_display_update = true;
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
  unsigned long saved_count = 0;
  unsigned long save_pos = 0;

  for (unsigned int pos = 0; pos < MEMO_CELLS_NUM; pos += MEMO_DATA_SIZE) {
    unsigned int read_value;
    EEPROM.get(pos, read_value);

    if (saved_count < (unsigned long)read_value) {
      saved_count = (unsigned long)read_value;
      save_pos = pos;
    }
  }

  count = saved_count * CYCLES_PER_SAVE;
  memo_pos = save_pos;
}

void save_count(void) {
  memo_pos += MEMO_DATA_SIZE;
  if (memo_pos >= MEMO_CELLS_NUM) {
    memo_pos = 0;
  }
  unsigned int write_value = count / CYCLES_PER_SAVE;
  EEPROM.put(memo_pos, write_value);
}

void update_display(void) {
  unsigned int sens_display_value = sens_smoothed;
  // float sens_display_value = (float)sens_smoothed * K_COEFF + B_COEFF;
  float cycle_T_display_value = (float)cycle_T * TIME_FACTOR;

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
  // display.println(sens_display_value, 1);
  display.println(sens_display_value);

  display.setTextSize(1);
  display.println();
  display.setTextSize(2);

  display.print("T");
  display.setTextSize(1);
  display.print(" ");
  display.print(sens_flag);
  display.setTextSize(2);
  display.print(" = ");
  if (cycle_T < 1) {
    display.print("n-a");
  } else {
    display.print(cycle_T_display_value, 1);
  }

  display.display();
}