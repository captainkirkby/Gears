#ifndef ADC_H
#define ADC_H

// ADC input pin number
#define ADC_IR_IN      4

// Buffer Length
#define CIRCULAR_BUFFER_LENGTH 800

// Set trigger (percent of a 10 bit sample)
#define THRESHOLD 500

// How much further after the trigger we go
// Percent of a CIRCULAR_BUFFER_LENGTH sample that comes after the trigger
#define END_TIMER 500



#endif