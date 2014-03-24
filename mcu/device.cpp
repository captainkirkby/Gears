#include <Arduino.h>
#include <Adafruit_BMP085_U.h>
#include <avr/eeprom.h>

#include "packet.h"
#include "pins.h"
#include "leds.h"

// Uses the fastest baud rate that can be synthesized from a 10 MHz clock with <2% error
// See http://www.wormfood.net/avrbaudcalc.php for details on how this is calculated.
#define BAUD_RATE 57600

// Creates our BMP interface object
Adafruit_BMP085_Unified bmpSensor = Adafruit_BMP085_Unified();

// Declares and initializes our boot info packet
BootPacket bootPacket = {
	START_BYTE, START_BYTE, START_BYTE, BOOT_PACKET,
	0,0,0,
#ifdef COMMIT_INFO
    COMMIT_INFO
#elif
    0, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 0
#endif
};

// Declares the data packet we will transmit periodically
DataPacket dataPacket;

/////////////// Free Running ADC ////////////////////

// Value to store analog result
volatile uint16_t adcValue = 0;

//Create buffer hardcoded with 800 elements
const uint16_t CIRCULAR_BUFFER_LENGTH = 800;
uint16_t circularbuffer[CIRCULAR_BUFFER_LENGTH];
uint16_t currentElementIndex;

//High when the buffer is ready to be dumped
volatile int done;

//Set trigger
const uint16_t THRESHOLD = 500;

//Declare timer
volatile uint16_t timer;
const uint16_t END_TIMER = 500;

//Set pins
// const uint8_t DIGITAL_TRIGGER_PIN = 4;
// const uint8_t DIGITAL_PULSE_PIN = 3;
// const uint8_t ANALOG_READ_PIN = 2;

// //Dumps the circular buffer from current element, wrapping around, to itself.
// void dumpToSerialPort(){
// 	Serial.println();
// 	for(uint16_t i=currentElementIndex;i<CIRCULAR_BUFFER_LENGTH;++i){
// 		Serial.println(circularbuffer[i]);
// 	}
// 	for(uint16_t i=0;i<currentElementIndex;++i){
// 		Serial.println(circularbuffer[i]);
// 	}
// 	Serial.println();
// }

///////////////////////////////////////////////////

void setup() {

	// Initializes our I/O pins
	pinMode(LED_GREEN,OUTPUT);
	pinMode(LED_YELLOW,OUTPUT);
	pinMode(LED_RED,OUTPUT);
	analogWrite(PWM_IR_OUT,0); // pinMode init not necessary for PWM function

	//Setup Digital pulse pins
	pinMode(PULSE_TEST_POINT, OUTPUT);
	digitalWrite(PULSE_TEST_POINT, LOW);

	//Setup Digital trigger pins
	pinMode(TRIGGER_TEST_POINT, OUTPUT);
	digitalWrite(TRIGGER_TEST_POINT, LOW);

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
	
	// Set MUX3..0 in ADMUX (0x7C) to read from AD8 (Internal temp)
	// Do not set above 15! You will overrun other parts of ADMUX. A full
	// list of possible inputs is available in Table 24-4 of the ATMega328
	// datasheet
	ADMUX |= ADC_IR_IN;
	// ADMUX |= B00000010; // Binary equivalent
	
	// Set ADEN in ADCSRA (0x7A) to enable the ADC.
	// Note, this instruction takes 12 ADC clocks to execute
	ADCSRA |= B10000000;
	
	// Set ADATE in ADCSRA (0x7A) to enable auto-triggering.
	ADCSRA |= B00100000;
	
	// Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
	// This means that as soon as an ADC has finished, the next will be
	// immediately started.
	ADCSRB &= B11111000;
	
	// Set the Prescaler to 128 (16000KHz/128 = 125KHz)
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



	// Use ADC2 = PA2 = Arduino#26 as digital diagnostic output
	pinMode(TEST_POINT,OUTPUT);
	digitalWrite(TEST_POINT,LOW);

	// Turns all LEDs on then off (0.5s + 0.5s)
	LED_ON(GREEN); LED_ON(YELLOW); LED_ON(RED);
	delay(500);
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	delay(500);

	// Initializes serial communications
	Serial.begin(BAUD_RATE);
	Serial1.begin(9600);

	// Copies our serial number from EEPROM address 0x10 into the boot packet
	bootPacket.serialNumber = eeprom_read_dword((uint32_t*)0x10);
	// Initializes the BMP air pressure & temperature sensor communication via I2C
	if(bmpSensor.begin()) {
		bootPacket.bmpSensorOk = 1;
		LED_ON(GREEN);
	}
	// Initializes the GPS serial communication
	// ...
	// bootPacket.gpsSerialOk = 1;
	// ...
	// Sends our boot packet.
	LED_ON(RED);
	Serial.write((const uint8_t*)&bootPacket,sizeof(bootPacket));
	delay(500);
	LED_OFF(RED);

	// Initializes the constant header of our data packet
	dataPacket.start[0] = START_BYTE;
	dataPacket.start[1] = START_BYTE;
	dataPacket.start[2] = START_BYTE;
	dataPacket.type = DATA_PACKET;
	dataPacket.sequenceNumber = 0;
}

void loop() {
	// Updates our sequence number for the next packet
	dataPacket.sequenceNumber++;
	// Reads out the BMP sensor if we have one available
	if(bootPacket.bmpSensorOk) {
		// Reads BMP sensors
		bmpSensor.getTemperature(&dataPacket.temperature);
		bmpSensor.getPressure(&dataPacket.pressure);
	}

	// Store ADC run and kick off new conversion
	if(done){
		//memcpy(&dataPacket.raw, &circularbuffer,NUM_RAW_BYTES);
		// Data packet has 1600 1 byte entries
		// Circular buffer has 800 2 byte entries
		uint16_t rawFill = 0;
		for(uint16_t i=currentElementIndex+1;i<CIRCULAR_BUFFER_LENGTH;++i){
			dataPacket.raw[rawFill++] = circularbuffer[i];
		}
		for(uint16_t i=0;i<currentElementIndex+1;++i){
			dataPacket.raw[rawFill++] = circularbuffer[i];
		}

		// dumpToSerialPort();

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
	// Reads out the ADC channels with 64x oversampling.
	// We add the samples since the sum of 64 10-bit samples fully uses the available
	// 16 bits without any overflow.
	dataPacket.thermistor = dataPacket.humidity = dataPacket.irLevel = 0;
	for(uint8_t count = 0; count < 64; ++count) {
		dataPacket.thermistor += (uint16_t)analogRead(ADC_THERMISTOR);
		dataPacket.humidity += (uint16_t)analogRead(ADC_HUMIDITY);
		dataPacket.irLevel += (uint16_t)analogRead(ADC_IR_IN);
	}
	// Sends binary packet data
	LED_ON(RED);
	Serial.write((const uint8_t*)&dataPacket,sizeof(dataPacket));
	delay(500);
	LED_OFF(RED);

	// // Looks for any incoming data on the 2nd UART
	// char buffer[64];
	// for(uint8_t i = 0; i < 10; i++) {
	// 	while(Serial1.available() > 0) {
	// 		LED_TOGGLE(RED);
	// 		digitalWrite(TEST_POINT,!digitalRead(TEST_POINT));
	// 		Serial1.readBytes(buffer,64);
	// 	}
	// 	delay(100);
	// 	LED_OFF(RED);
	// }
	// LED_TOGGLE(GREEN);
	delay(500);

}

// Interrupt service routine for the ADC completion
ISR(ADC_vect){
   	// Must read low first
 	adcValue = ADCL | (ADCH << 8);

	if(digitalRead(PULSE_TEST_POINT)){
		digitalWrite(PULSE_TEST_POINT, LOW);
	} else {
		digitalWrite(PULSE_TEST_POINT, HIGH);
	}

	if(timer > 0){
		//if we are still filling buffer before dump
		++timer;

		//Time padding
		digitalWrite(TRIGGER_TEST_POINT, LOW);

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
		/*
		if above THRESHOLD, allow buffer to fill n number of times, dump on the serial port, then continue
		in either case, fill buffer once and increment pointer
		*/	
	
		if(adcValue >= THRESHOLD){
			//start timer
			timer = 1;
			digitalWrite(TRIGGER_TEST_POINT, HIGH);


			//Waste a few clock cycles to note on the oscilloscope that we have started the timer
			//delayMicroseconds(1);
		} else {
			//make sure timer is stopped
			timer = 0;
			digitalWrite(TRIGGER_TEST_POINT, LOW);
		}
	}

	//Add value to buffer
	currentElementIndex = (currentElementIndex + 1) % CIRCULAR_BUFFER_LENGTH;
	circularbuffer[currentElementIndex] = adcValue;
  
	// Not needed because free-running mode is enabled.
	// Set ADSC in ADCSRA (0x7A) to start another ADC conversion
	// ADCSRA |= B01000000;
}

int main(void) {
	init();
	setup();
	for(;;) {
		loop();
	}
	return 0;
}
