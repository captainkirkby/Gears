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
#include "Trimble.h"
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
volatile uint16_t adcTestTries;

//Declare timer used for filling up the buffer
/*volatile*/ uint16_t timer;

//Declare counter used to keep track of timing
uint16_t timingCounter = 0;
volatile uint64_t runningCount = 0;

// Oversampling Counter
uint8_t oversampleCount = 0;

uint16_t thermistorReading;
uint16_t humidityReading;

uint8_t currentSensorIndex;

volatile uint8_t currentMuxChannel;
volatile uint8_t ppsFlag = 0;

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
        flashNumber(100+bmpError);
        bootPacket.bmpSensorStatus = bmpError;
    }

    // Test Point Configuration
    // Set pin as output
    DDRA |= 0B00000100;
    // Set pin to low
    PORTA &= ~0B00000100;

    // PPS Interrupt Configuration
    // Set pin as input
    DDRD &= ~0B01000000;
    // Enable internal pull-up resistor
    PORTD |= 0B01000000;
    // Enable Interrupts on selected pin
    PCICR |= 0B00001000;
    // Select Pin to listen to
    PCMSK3 |= 0B01000000;
    // Enable Global Interrupts
    // When interrupt is called, Free Running ADC is started
    sei();

    // Wait for the test to start
    while(!ppsFlag);

    // Wait for test to finish
    // At the end, the ADC will either be running in error or idle mode
    while(adcStatus == ADC_STATUS_TESTING);

    // Non-zero if sensor block is OK
    bootPacket.sensorBlockOK = !(adcStatus == ADC_STATUS_ERROR);

    // Turn off GPS Auto packets and store status in boot packet
    bootPacket.gpsSerialOk = turnOffGPSAutoPackets();

    // Readout GPS health (and position?)
    TsipHealthResponsePacket health = getGPSHealth();
    bootPacket.latitude = health.latitude;
    bootPacket.longitude = health.longitude;
    bootPacket.altitude = health.altitude;

    // Copies our serial number from EEPROM address 0x10 into the boot packet
    bootPacket.serialNumber = eeprom_read_dword((uint32_t*)0x10);

    // Get Time
    TsipCommandResponsePacket time = getTime();
    bootPacket.utcOffset = time.gpsOffset;
    bootPacket.weekNumber = time.weekNumber;
    bootPacket.timeOfWeek = time.timeOfWeek;

    uint8_t zero = 0x00;

    // Write Zeros before we send boot packet
    for(uint8_t zi = 0; zi < 100; ++zi){
        serialWriteUSB((const uint8_t*)&zero,sizeof(zero));    
    }

    // Sends our boot packet
    LED_ON(GREEN);
    // _delay_ms(2000);
    serialWriteUSB((const uint8_t*)&bootPacket,sizeof(bootPacket));
    LED_OFF(GREEN);

    // Initializes the constant header of our data packet
    dataPacket.start[0] = START_BYTE;
    dataPacket.start[1] = START_BYTE;
    dataPacket.start[2] = START_BYTE;
    dataPacket.type = DATA_PACKET;
    dataPacket.sequenceNumber = 0;

    // Enable Trigger
    adcStatus = ADC_STATUS_CONTINUOUS;

    while(1) {
        // Store ADC run and transmit data
        if(adcStatus == ADC_STATUS_DONE){
            // Updates our sequence number for the next packet
            dataPacket.sequenceNumber++;

            dataPacket.timeSinceLastReading = runningCount;

            // Reads the BMP180 sensor values and saves the results in the data packet
            bmpError = readBMP180Sensors(&dataPacket.temperature,&dataPacket.pressure);
            
            if(bmpError) flashNumber(200+bmpError);

            dataPacket.rawPhase = (currentElementIndex + 1) % CIRCULAR_BUFFER_LENGTH;
            dataPacket.thermistor = thermistorReading;
            dataPacket.humidity = humidityReading;

            // Readout GPS health
            health = getGPSHealth();

            dataPacket.recieverMode = health.recieverMode;
            dataPacket.discipliningMode = health.discipliningMode;
            dataPacket.criticalAlarms = health.criticalAlarms;
            dataPacket.minorAlarms = health.minorAlarms;
            dataPacket.gpsDecodingStatus = health.gpsDecodingStatus;
            dataPacket.discipliningActivity = health.discipliningActivity;
            dataPacket.clockOffset = health.clockOffset;
            
            // Sends binary packet data synchronously
            serialWriteUSB((const uint8_t*)&dataPacket,sizeof(dataPacket));

            adcStatus = ADC_STATUS_CONTINUOUS;
        }
    }
    return 0; // never actually reached
}

// Interrupt service routine for the ADC completion
// Note: When this is called, the next ADC conversion is already underway
ISR(ADC_vect){
    // // Set pin to low
    // PORTA &= ~0B00000100;
    // Update counter that keeps track of the timing
    ++timingCounter;

    // Must read low first
    adcValue = ADCL | (ADCH << 8);

    if(adcStatus >= ADC_STATUS_TESTING){
        if(adcValue > THRESHOLD){
            // Not covered, leave testing mode
            adcStatus = ADC_STATUS_IDLE;
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
                // Store time since last threshold (max 5.4s for a 16 bit counter)
                runningCount += timingCounter;
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
        dataPacket.raw[currentElementIndex] = (adcValue & 0xFF);             // Lower byte 
    } else if(adcStatus == ADC_STATUS_UNSTABLE){
        // Current reading is unstable, but next one will be stable
        adcStatus = ADC_STATUS_ONE_SHOT;

    } else if(adcStatus == ADC_STATUS_ONE_SHOT){
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
                currentSensorIndex = 0;

            } else {
                // Next is another one shot
                // set adcStatus to unstable
                adcStatus = ADC_STATUS_UNSTABLE;
            }
            // Set ADC channel
            switchADCMuxChannel(analogSensors[currentSensorIndex]);
        }
    } // Do nothing if adc status is ADC_STATUS_IDLE or ADC_STATUS_ERROR
}

// Interrupt fired when 1PPS changes value
ISR(PCINT3_vect){
    // // Set pin to high
    // PORTA |= 0B00000100;
    if(PIND & 0B01000000)
    {
        /* LOW to HIGH pin change */
        LED_TOGGLE(GREEN);
        // Disable Interrupts on selected pin
        PCICR &= ~0B00001000;
        ppsFlag = 1;

        // ADC Interrupt configuration
        // Setup circular buffer and timer
        currentElementIndex = 0;
        timer = 0;
        // Set ADC state and choose ADC channel
        adcStatus = ADC_STATUS_TESTING;
        currentSensorIndex = 0;
        // Set the number of tries for sensor to be unobstructed
        adcTestTries = 0;
        startFreeRunningADC(analogSensors[currentSensorIndex]);
    }
}
