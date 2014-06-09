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
    0,0,0,0,
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
// uint16_t circularbuffer[CIRCULAR_BUFFER_LENGTH];
uint16_t currentElementIndex;

// Stores state of the ADC
volatile uint8_t adcStatus;

// Stores number of tries we've had in the ADC to get an unobstructed reading
uint8_t adcTestTries;

//Declare timer used for filling up the buffer
/*volatile*/ uint16_t timer;

//Declare counter used to keep track of timing
uint16_t timingCounter = 0;
volatile uint16_t lastCount;

// Oversampling Counter
uint8_t oversampleCount = 0;

uint16_t thermistorReading;
uint16_t humidityReading;

uint8_t currentSensorIndex;

volatile uint8_t currentMuxChannel;

int main(void)
{
    uint8_t bmpError;

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
    // Set the number of tries for sensor to be unobstructed
    adcTestTries = 0;
    // currentMuxChannel = analogSensors[currentSensorIndex];
    startFreeRunningADC(analogSensors[currentSensorIndex]);

    // Wait for test to finish
    // At the end, the ADC will either be running in Free Running Mode or
    // be disabled.
    while(adcStatus == ADC_STATUS_TESTING);

    // Non-zero if sensor block is OK
    bootPacket.sensorBlockOK = !(adcStatus == ADC_STATUS_ERROR);

    // Copies our serial number from EEPROM address 0x10 into the boot packet
    bootPacket.serialNumber = eeprom_read_dword((uint32_t*)0x10);

    // Sends our boot packet
    LED_ON(GREEN);
    serialWriteUSB((const uint8_t*)&bootPacket,sizeof(bootPacket));
    _delay_ms(500);
    LED_OFF(GREEN);

    // Initializes the constant header of our data packet
    dataPacket.start[0] = START_BYTE;
    dataPacket.start[1] = START_BYTE;
    dataPacket.start[2] = START_BYTE;
    dataPacket.type = DATA_PACKET;
    dataPacket.sequenceNumber = 0;


    while(1) {
        // Store ADC run and transmit data
        if(adcStatus == ADC_STATUS_DONE){
            // Updates our sequence number for the next packet
            dataPacket.sequenceNumber++;

            dataPacket.timeSinceLastReading = lastCount;

            // Reads the BMP180 sensor values and saves the results in the data packet
            bmpError = readBMP180Sensors(&dataPacket.temperature,&dataPacket.pressure);
            
            if(bmpError) flashNumber(200+bmpError);

            dataPacket.rawPhase = currentElementIndex+1;
            dataPacket.thermistor = thermistorReading;
            dataPacket.humidity = humidityReading;
            
            // Sends binary packet data synchronously
            serialWriteUSB((const uint8_t*)&dataPacket,sizeof(dataPacket));

            currentSensorIndex = 0;
            adcStatus = ADC_STATUS_CONTINUOUS;
        }
    }
    return 0; // never actually reached
}

// Interrupt service routine for the ADC completion
// Note: When this is called, the next ADC conversion is already underway
ISR(ADC_vect){
    // Update counter that keeps track of the timing
    ++timingCounter;

    // Must read low first
    adcValue = ADCL | (ADCH << 8);

    if(adcStatus >= ADC_STATUS_TESTING){
        if(adcValue > THRESHOLD){
            // Not covered, leave testing mode
            adcStatus = ADC_STATUS_CONTINUOUS;
        } else {
            if(adcTestTries < ADC_MAX_TEST_TRIES){
                // Keep incrementing the ADC Status until we hit ADC_STATUS_ERROR
                ++adcTestTries;
            } else {
                // Stop ADC (indicate this in the boot packet?)
                // Clear ADEN in ADCSRA (0x7A) to disable the ADC.
                // Note, this instruction takes 12 ADC clocks to execute
                ADCSRA &= ~0B10000000;
                adcStatus = ADC_STATUS_ERROR;
                LED_ON(RED);
            }
        }
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

                // Change ADC channel
                // Next reading is unstable, one after that is good data
                // currentMuxChannel = analogSensors[0];
                currentSensorIndex = 1;
                switchADCMuxChannel(analogSensors[currentSensorIndex]);

                // Reset fields for analog readings
                thermistorReading = 0;
                humidityReading = 0;

                // set adcStatus to unstable
                adcStatus = ADC_STATUS_UNSTABLE;
            }
        } else {
            // if above THRESHOLD, allow buffer to fill n number of times, dump on the serial port, then continue
            // in either case, fill buffer once and increment pointer
        
            if(adcValue <= THRESHOLD){
                // Store time since last threshold (max 10.9s for a 16 bit counter)
                lastCount = timingCounter;
                timingCounter = 0;
                // start timer
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
        dataPacket.raw[currentElementIndex] = adcValue;             // Fill actual data field instead ?
    } else { 
        // Reading out "one shot" analog sensors
        if(adcStatus == ADC_STATUS_UNSTABLE){
            // Current reading is unstable, but next one will be stable
            adcStatus = ADC_STATUS_ONE_SHOT;
        } else {

            // One shot mode with stable readings.
            // Store ACD conversion
            if(analogSensors[currentSensorIndex] == ADC_THERMISTOR){
                thermistorReading += adcValue;
            } else if(analogSensors[currentSensorIndex] == ADC_HUMIDITY){
                humidityReading += adcValue;
            }

            ++oversampleCount;

            // Only change the channel if we are done oversampling
            if(oversampleCount >= ADC_ONE_SHOT_OVERSAMPLING){
                oversampleCount = 0;
                // Change ADC channel
                // Next reading is unstable, one after that is good data
                currentSensorIndex++;

                if(currentSensorIndex == NUM_SENSORS){
                    // Next is done (reading is still unstable though)
                    adcStatus = ADC_STATUS_DONE;
                } else {
                    // Next is another one shot, change channel
                    switchADCMuxChannel(analogSensors[currentSensorIndex]);
    
                    // set adcStatus to unstable
                    adcStatus = ADC_STATUS_UNSTABLE;
                }
            }
        }
    }
}
