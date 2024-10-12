#define PORT_SPEED 115200

#define SENS_0_PIN A0
#define INIT_DELAY 3000
#define POLLING_DELAY 100

#define TICKS_NUM 31250

volatile unsigned int sens_0 = 0;

void setup() {
  pinMode(SENS_0_PIN, INPUT);

  Serial.begin(PORT_SPEED);

  noInterrupts();
  TCCR1A = 0;           // init Timer1 - clear reg A
  TCCR1B = 0;           // init Timer1 - clear reg B
  TCNT1 = 0;            // Reset current timer counter
  OCR1A = TICKS_NUM;        // timer COMPA register
  TCCR1B |= (1 << WGM12);   // Enable CTC (Clear Timer on Compare Match)
  TCCR1B |= (1 << CS12);    // Prescaler 256
  TIMSK1 |= (1 << OCIE1A);  // Enable interrupt on coincidence
  delay(INIT_DELAY);
  interrupts();
}

void loop() {
  sens_0 = analogRead(SENS_0_PIN);
  delay(POLLING_DELAY);
}

ISR(TIMER1_COMPA_vect) {
  Serial.println(sens_0, HEX);
}
