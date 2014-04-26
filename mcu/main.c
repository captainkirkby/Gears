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

// Create buffer
uint16_t circularbuffer[CIRCULAR_BUFFER_LENGTH];
uint16_t currentElementIndex;

// Stores state of the ADC
volatile uint8_t adcStatus;

//Declare timer
volatile uint16_t timer;

uint16_t thermistorReading;
uint16_t humidityReading;
// uint16_t irReading;

uint8_t currentSensorIndex;

volatile uint8_t currentMuxChannel;

int main(void)
{
    uint8_t bmpError, adcError;
    uint16_t adcTestReading;

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

    // Setup circular buffer and timer
    currentElementIndex = 0;
    timer = 0;
    // Set ADC state and choose ADC channel
    adcStatus = ADC_STATUS_TESTING;
    currentSensorIndex = 0;
    // currentMuxChannel = analogSensors[currentSensorIndex];
    startFreeRunningADC(analogSensors[currentSensorIndex]);

    // Wait for test to finish
    // At the end, the ADC will either be running in Free Running Mode or
    // be disabled.
    while(adcStatus == ADC_STATUS_TESTING);

    // adcTestReading = testADC(currentMuxChannel);
    // adcError = (adcTestReading < THRESHOLD);
    // //adcError = 0;
    // if(adcError){
    //     flashNumber(adcTestReading);
    //     // Add boot packet adc error
    // } else {
    //     // Setup circular buffer and timer
    //     currentElementIndex = 0;
    //     timer = 0;
    //     // Set ADC state and choose ADC channel
    //     adcStatus = ADC_STATUS_CONTINUOUS;
    //     currentSensorIndex = 0;
    //     // currentMuxChannel = analogSensors[currentSensorIndex];
    //     startFreeRunningADC(analogSensors[currentSensorIndex]);
    // }

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
        // rippleUp();
        // rippleDown();

        // Store ADC run and transmit data
        if(adcStatus == ADC_STATUS_DONE && !adcError){
            // Updates our sequence number for the next packet
            dataPacket.sequenceNumber++;

            // Reads the BMP180 sensor values and saves the results in the data packet
            bmpError = readBMP180Sensors(&dataPacket.temperature,&dataPacket.pressure);
            
            if(bmpError) flashNumber(200+bmpError);
            // Circular buffer has 800 2 byte entries
            uint16_t rawFill = 0;
            for(uint16_t i=currentElementIndex+1;i<CIRCULAR_BUFFER_LENGTH;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }
            for(uint16_t i=0;i<currentElementIndex+1;++i){
                dataPacket.raw[rawFill++] = circularbuffer[i];
            }

            dataPacket.thermistor = thermistorReading;
            dataPacket.humidity = humidityReading;
            
            currentSensorIndex = 0;
            adcStatus = ADC_STATUS_CONTINUOUS;
            
            // Sends binary packet data synchronously
            serialWriteUSB((const uint8_t*)&dataPacket,sizeof(dataPacket));
        }
    }
    return 0; // never actually reached
}

// Interrupt service routine for the ADC completion
// Note: When this is called, the next ADC conversion is already underway
ISR(ADC_vect){
    // Update counter
    // ++counter;

    // Must read low first
    adcValue = ADCL | (ADCH << 8);

    // if(digitalRead(PULSE_TEST_POINT)){
    //     digitalWrite(PULSE_TEST_POINT, LOW);
    // } else {
    //     digitalWrite(PULSE_TEST_POINT, HIGH);
    // }

    if(adcStatus >= ADC_STATUS_TESTING && adcStatus < ADC_STATUS_ERROR){
        if(adcValue > THRESHOLD){
            // Not covered, leave testing mode
            adcStatus = ADC_STATUS_CONTINUOUS;
        } else {
            // Keep incrementing the ADC Status until we hit ADC_STATUS_ERROR
            ++adcStatus;
        }
    } else if(adcStatus == ADC_STATUS_ERROR){
        // Stop ADC
        // Clear ADEN in ADCSRA (0x7A) to disable the ADC.
        // Note, this instruction takes 12 ADC clocks to execute
        ADCSRA &= ~0B10000000;
    } else if(adcStatus == ADC_STATUS_CONTINUOUS){
        // Take IR ADC data

        if(timer > 0){
            // if we are still filling buffer before dump
            ++timer;
    
            // check if we are finished filling the buffer
            if(timer == END_TIMER){
                LED_TOGGLE(YELLOW);
                //uncomment this for mutiple buffers.
                timer = 0;
    
                // //Stop the ADC by clearing ADEN
                // ADCSRA &= ~0B10000000;
    
                // //Clear ADC start bit
                // ADCSRA &= ~0B01000000;
    
                // Change ADC channel
                // Next reading is unstable, one after that is good data
                // currentMuxChannel = analogSensors[0];
                switchADCMuxChannel(analogSensors[currentSensorIndex++]);

                // set adcStatus to unstable
                adcStatus = ADC_STATUS_UNSTABLE;
            }
        } else {
            // if above THRESHOLD, allow buffer to fill n number of times, dump on the serial port, then continue
            // in either case, fill buffer once and increment pointer
        
            if(adcValue <= THRESHOLD){
                // start timer
                timer = 1;
                LED_ON(RED);
                // digitalWrite(TRIGGER_TEST_POINT, HIGH);
            } else {
                //make sure timer is stopped
                timer = 0;
                // digitalWrite(TRIGGER_TEST_POINT, LOW);
            }
        }
    
        //Add value to buffer
        currentElementIndex = (currentElementIndex + 1) % CIRCULAR_BUFFER_LENGTH;
        circularbuffer[currentElementIndex] = adcValue;             // Fill actual data field instead ?
    } else { 
        // Reading out "one shot" analog sensors
        if(adcStatus == ADC_STATUS_UNSTABLE){
            // Current reading is unstable, but next one will be stable
            adcStatus = ADC_STATUS_ONE_SHOT;
        } else {

            // One shot mode with stable readings.
            // Store ACD conversion
            if(analogSensors[currentSensorIndex] == ADC_THERMISTOR){
                thermistorReading = adcValue;
            } else if(analogSensors[currentSensorIndex] == ADC_HUMIDITY){
                humidityReading = adcValue;
            }

            // Change ADC channel
            // Next reading is unstable, one after that is good data
            // currentMuxChannel = analogSensors[0];
            if(currentSensorIndex + 1 < NUM_SENSORS){
                // Next is done (reading is still unstable though)
                adcStatus = ADC_STATUS_DONE;
            } else {
                // Next is another one shot, change channel
                switchADCMuxChannel(analogSensors[currentSensorIndex++]);

                // set adcStatus to unstable
                adcStatus = ADC_STATUS_UNSTABLE;
            }
            LED_ON(GREEN);
        }
    }
}
