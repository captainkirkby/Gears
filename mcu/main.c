/***************************************************************************
 * Top-level microcontroller firmware program.
 * 
 * Authors: David & Dylan Kirkby / Jan 2014.
 ***************************************************************************/

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "LED.h"
#include "IR.h"
#include "UART.h"
#include "TWI.h"
#include "BMP180.h"

#include "packet.h"

// Declares and initializes our boot info packet
BootPacket bootPacket = {
    { START_BYTE, START_BYTE, START_BYTE },
    BOOT_PACKET,
    0,0,0,
#ifdef COMMIT_INFO
    COMMIT_INFO
#else
    0, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 0
#endif
};

// Declares the data packet we will transmit periodically
DataPacket dataPacket;

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

int main(void)
{
    uint8_t error;

    // Initializes low-level hardware
	initLEDs();
    initIR();
	initUARTs();
    initTWI();
    initFreeRunningADC();

    // Initializes communication with the BMP sensor
    error = initBMP180();
    if(error) {
        // A non-zero status indicates a problem, so flash an error code
        flashNumber(100+bootPacket.bmpSensorStatus);
        bootPacket.bmpSensorStatus = error;
    }

    startFreeRunningADC();

    // Copies our serial number from EEPROM address 0x10 into the boot packet
    bootPacket.serialNumber = eeprom_read_dword((uint32_t*)0x10);

    // Sends our boot packet
    LED_ON(RED);
    serialWriteUSB((const uint8_t*)&bootPacket,sizeof(bootPacket));
    _delay_ms(500);
    LED_OFF(RED);

    // Initializes the constant header of our data packet
    dataPacket.start[0] = START_BYTE;
    dataPacket.start[1] = START_BYTE;
    dataPacket.start[2] = START_BYTE;
    dataPacket.type = DATA_PACKET;
    dataPacket.sequenceNumber = 0;


    while(1) {
        rippleUp();
        rippleDown();
        // Updates our sequence number for the next packet
        dataPacket.sequenceNumber++;
        // Reads the BMP180 sensor values and saves the results in the data packet
        error = readBMP180Sensors(&dataPacket.temperature,&dataPacket.pressure);
        if(error) flashNumber(200+error);

        // Store ADC run and kick off new conversion
        if(done){
            // Circular buffer has 800 2 byte entries
            uint16_t rawFill = 0;
            for(uint16_t i=currentElementIndex+1;i<CIRCULAR_BUFFER_LENGTH;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }
            for(uint16_t i=0;i<currentElementIndex+1;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }
            
            restartFreeRunningADC();
            // startFreeRunningADC();
        }

        // Sends binary packet data
        serialWriteUSB((const uint8_t*)&dataPacket,sizeof(dataPacket));
    }
    return 0; // never actually reached
}


// Interrupt service routine for the ADC completion
ISR(ADC_vect){
    // Must read low first
    adcValue = ADCL | (ADCH << 8);

    // if(digitalRead(PULSE_TEST_POINT)){
    //     digitalWrite(PULSE_TEST_POINT, LOW);
    // } else {
    //     digitalWrite(PULSE_TEST_POINT, HIGH);
    // }

    if(timer > 0){
        //if we are still filling buffer before dump
        ++timer;

        //Time padding
        // digitalWrite(TRIGGER_TEST_POINT, LOW);

        //check if we are done filling the buffer
        if(timer == END_TIMER){
            LED_TOGGLE(YELLOW);
            //uncomment this for mutiple buffers.
            timer = 0;

            //Stop the ADC by clearing ADEN
            ADCSRA &= ~B10000000;

            //Clear ADC start bit
            ADCSRA &= ~B01000000;

            //set done
            done = 1;
        }
    } else {
        // if above THRESHOLD, allow buffer to fill n number of times, dump on the serial port, then continue
        // in either case, fill buffer once and increment pointer
    
        if(adcValue <= THRESHOLD){
            //start timer
            timer = 1;
            // digitalWrite(TRIGGER_TEST_POINT, HIGH);
        } else {
            //make sure timer is stopped
            timer = 0;
            // digitalWrite(TRIGGER_TEST_POINT, LOW);
        }
    }

    //Add value to buffer
    currentElementIndex = (currentElementIndex + 1) % CIRCULAR_BUFFER_LENGTH;
    circularbuffer[currentElementIndex] = adcValue;
}
