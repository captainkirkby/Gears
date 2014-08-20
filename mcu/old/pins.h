#ifndef PINS_H
#define PINS_H

// Defines ATMega644 pin assignments for the custom TickTock circuit

// LED driver outputs (low = ON)
// These are Arduino pin numbers whose mappings to the device ports
// are specified in pins_arduino.h
#define LED_GREEN		20
#define LED_YELLOW		21
#define LED_RED			22

// PWM outputs
// These are Arduino pin numbers whose mappings to the device ports
// are specified in pins_arduino.h
#define PWM_IR_OUT		15

// ADC inputs
// These are ADC channel numbers, not Arduino pin numbers.
#define ADC_HUMIDITY	1			// TP2 on schematic
#define ADC_THERMISTOR	3
#define ADC_IR_IN		4

// Test Points
// These are Arduino pin numbers.
#define TRIGGER_TEST_POINT 31		// TP3 on schematic
#define PULSE_TEST_POINT 29			// TP1 on schematic


// ICP inputs (not sure if this is needed yet)
//#define GPS_1PPS		14

// Use ADC2 = PA2 = Arduino#26 as digital output test point
#define TEST_POINT		26


#endif
