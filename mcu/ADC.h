#ifndef ADC_H
#define ADC_H

void initFreeRunningADC()
{
    // Value to store analog result
    volatile uint16_t adcValue = 0;
    
    //Create buffer hardcoded with 800 elements
    const uint16_t CIRCULAR_BUFFER_LENGTH = 800;
    uint16_t circularbuffer[CIRCULAR_BUFFER_LENGTH];
    uint16_t currentElementIndex;
    
    //High when the buffer is ready to be dumped
    volatile int done;
    
    //Set trigger (percent of a 10 bit sample)
    const uint16_t THRESHOLD = 500;
    
    //Declare timer
    volatile uint16_t timer;
    const uint16_t END_TIMER = 500;
}

void startFreeRunningADC()
{
    //Setup circular buffer
    currentElementIndex = 0;

    //Setup timer
    timer = 0;
    
    // clear ADLAR in ADMUX (0x7C) to right-adjust the result
    // ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits)
    ADMUX &= B11011111;
    
    // Set REFS1..0 in ADMUX (0x7C) to change reference voltage to the
    // proper source (01)
    ADMUX |= B01000000;
    
    // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
    // input
    ADMUX &= B11110000;
    
    // Set MUX3..0 in ADMUX (0x7C) to read from the IR Photodiode
    // Do not set above 15! You will overrun other parts of ADMUX. A full
    // list of possible inputs is available in the datasheet
    ADMUX |= ADC_IR_IN;
    
    // Set ADEN in ADCSRA (0x7A) to enable the ADC.
    // Note, this instruction takes 12 ADC clocks to execute
    ADCSRA |= B10000000;
    
    // Set ADATE in ADCSRA (0x7A) to enable auto-triggering.
    ADCSRA |= B00100000;
    
    // Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
    // This means that as soon as an ADC has finished, the next will be
    // immediately started.
    ADCSRB &= B11111000;
    
    // Set the Prescaler to 128 (10000KHz/128 = 78.125KHz)
    // Above 200KHz 10-bit results are not reliable.
    ADCSRA |= B00000111;
    
    // Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
    // Without this, the internal interrupt will not trigger.
    ADCSRA |= B00001000;
    
    // Enable global interrupts
    // AVR macro included in <avr/interrupts.h>, which the Arduino IDE
    // supplies by default.
    sei();
    
    // Set ADSC in ADCSRA (0x7A) to start the ADC conversion
    ADCSRA |=B01000000;
}

void restartFreeRunningADC()
{
    done = 0;
    // Set ADEN in ADCSRA (0x7A) to enable the ADC.
    // Note, this instruction takes 12 ADC clocks to execute
    ADCSRA |= B10000000;
    
    // Enable global interrupts
    // AVR macro included in <avr/interrupts.h>, which the Arduino IDE
    // supplies by default.
    sei();
        
    // Set ADSC in ADCSRA (0x7A) to start the ADC conversion
    ADCSRA |=B01000000;
}

#endif