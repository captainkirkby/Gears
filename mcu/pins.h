#ifndef PINS_H
#define PINS_H

// Defines ATMega644 pin assignments for the custom TickTock circuit
// These are Arduino pin numbers whose mappings to the device ports
// are specified in pins_arduino.h

// LED driver outputs (low = ON)
#define LED_GREEN		20
#define LED_YELLOW		21
#define LED_RED			22

// ADC inputs
#define ADC_HUMIDITY	30
#define ADC_THERMISTOR	28
#define ADC_IR_IN		27

// PWM outputs
#define PWM_IR_OUT		15

// ICP inputs
#define GPS_1PPS		14

#endif
