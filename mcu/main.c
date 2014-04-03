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
#include "FreeRunningADC.h"

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

// Value to store analog result
volatile uint16_t adcValue = 0;

//Create buffer
uint16_t circularbuffer[CIRCULAR_BUFFER_LENGTH];
uint16_t currentElementIndex;

//High when the buffer is ready to be dumped
volatile int done;

//Declare timer
volatile uint16_t timer;

int main(void)
{
    uint8_t bmpError, adcError;

    // Initializes low-level hardware
	initLEDs();
    initIR();
	initUARTs();
    initTWI();

    // Initializes communication with the BMP sensor
    bmpError = initBMP180();
    if(bmpError) {
        // A non-zero status indicates a problem, so flash an bmpError code
        flashNumber(100+bootPacket.bmpSensorStatus);
        bootPacket.bmpSensorStatus = bmpError;
    }

    adcError = (testADC() < THRESHOLD);
    if(adcError){
        flashNumber(100+adcError);
        // Add boot packet adc error
    } else {
        //Setup circular buffer and timer
        currentElementIndex = 0;
        timer = 0;
        startFreeRunningADC();
    }

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
        bmpError = readBMP180Sensors(&dataPacket.temperature,&dataPacket.pressure);
        if(bmpError) flashNumber(200+bmpError);

        // Store ADC run and kick off new conversion
        if(done && !adcError){
            // Circular buffer has 800 2 byte entries
            uint16_t rawFill = 0;
            for(uint16_t i=currentElementIndex+1;i<CIRCULAR_BUFFER_LENGTH;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }
            for(uint16_t i=0;i<currentElementIndex+1;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }
            
            done = 0;
            restartFreeRunningADC();
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
            ADCSRA &= ~0B10000000;

            //Clear ADC start bit
            ADCSRA &= ~0B01000000;

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
