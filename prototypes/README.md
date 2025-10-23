# List of prototypes

## timer-adc

Checking the timer configuration.

## timer-adc-display

Checking the configuration of the timer, sensor and display. Showing information on the display

## timer-display-eeprom

Checking data writing to EEPROM. Main function rewritten to non-blocking style.

## cycle-counter-002

Prototype version.
Hardware config:
- arduino nano
- ads1115
- oled display 128x64 i2c

Main objectives:
- test light ISR function
- exponential smoothing
- min / max value
- uint_xt
