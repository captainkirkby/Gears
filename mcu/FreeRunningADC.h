#ifndef ADC_H
#define ADC_H

#include "packet.h"

// ADC status constants
#define ADC_STATUS_CONTINUOUS 1
#define ADC_STATUS_UNSTABLE 2
#define ADC_STATUS_ONE_SHOT 3
#define ADC_STATUS_DONE 4
#define ADC_STATUS_TESTING 5
#define ADC_STATUS_IDLE 6
#define ADC_STATUS_ERROR 255

// ADC test constants
#define ADC_MAX_TEST_TRIES 10000    // This number determines how many tries the ADC gets to not be covered

// ADC inputs
// These are ADC channel numbers, not Arduino pin numbers.
#define ADC_HUMIDITY    1           // TP2 on schematic
#define ADC_THERMISTOR  3
#define ADC_IR_IN       4

#define NUM_SENSORS 3
uint8_t analogSensors[NUM_SENSORS] = {ADC_IR_IN, ADC_THERMISTOR, ADC_HUMIDITY};

// ADC oversampling
#define ADC_ONE_SHOT_OVERSAMPLING 64

// Buffer Length
//#define CIRCULAR_BUFFER_LENGTH 3687       // imported by packet.h

// Set trigger (percent of a 10 bit sample)
#define THRESHOLD 500

// Set time before trigger
#define SAMPLES_BEFORE_TRIGGER 50

// How much further after the trigger we go
// CIRCULAR_BUFFER_LENGTH - END_TIMER = SAMPLES_BEFORE_TRIGGER
// Portion of a CIRCULAR_BUFFER_LENGTH sample that comes after the trigger
const uint16_t END_TIMER = (CIRCULAR_BUFFER_LENGTH - ceil(CIRCULAR_BUFFER_LENGTH / 5.0)- SAMPLES_BEFORE_TRIGGER);

void switchADCMuxChannel(uint8_t channel)
{
    // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
    // input
    ADMUX &= 0B11110000;
    
    // Set MUX3..0 in ADMUX (0x7C) to read from a given channel
    // Do not set above 15! You will overrun other parts of ADMUX. A full
    // list of possible inputs is available in the datasheet
    ADMUX |= channel;
}

void startFreeRunningADC(uint8_t channel)
{    
    // clear ADLAR in ADMUX (0x7C) to right-adjust the result
    // ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits)
    ADMUX &= 0B11011111;
    
    // Set REFS1..0 in ADMUX (0x7C) to change reference voltage to the
    // proper source (01)
    ADMUX |= 0B01000000;
    
    // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
    // input
    ADMUX &= 0B11110000;
    
    // Set MUX3..0 in ADMUX (0x7C) to read from the IR Photodiode
    // Do not set above 15! You will overrun other parts of ADMUX. A full
    // list of possible inputs is available in the datasheet
    ADMUX |= channel;
    
    // Set ADEN in ADCSRA (0x7A) to enable the ADC.
    // Note, this instruction takes 12 ADC clocks to execute
    ADCSRA |= 0B10000000;
    
    // Set ADATE in ADCSRA (0x7A) to enable auto-triggering.
    ADCSRA |= 0B00100000;
    
    // Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
    // This means that as soon as an ADC has finished, the next will be
    // immediately started.
    ADCSRB &= 0B11111000;
    
    // Set the Prescaler to 64 (10000KHz/64 = 156.25kHz)
    // Above 200KHz 10-bit results are not reliable.
    ADCSRA |= 0B00000110;
    
    // Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
    // Without this, the internal interrupt will not trigger.
    ADCSRA |= 0B00001000;
    
    // Enable global interrupts
    // AVR macro included in <avr/interrupts.h>, which the Arduino IDE
    // supplies by default.
    sei();
    
    // Set ADSC in ADCSRA (0x7A) to start the ADC conversion
    ADCSRA |= 0B01000000;
}

void restartFreeRunningADC()
{
    // Set ADEN in ADCSRA (0x7A) to enable the ADC.
    // Note, this instruction takes 12 ADC clocks to execute
    ADCSRA |= 0B10000000;
    
    // Enable global interrupts
    // AVR macro included in <avr/interrupts.h>, which the Arduino IDE
    // supplies by default.
    sei();
        
    // Set ADSC in ADCSRA (0x7A) to start the ADC conversion
    ADCSRA |= 0B01000000;
}

#endif
