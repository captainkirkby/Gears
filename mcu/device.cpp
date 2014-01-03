#include <Arduino.h>
#include <Adafruit_BMP085_U.h>
#include <avr/eeprom.h>

#include "packet.h"
#include "pins.h"
#include "leds.h"

//Constants
#define BAUD_RATE 115200

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

void setup() {

	// Initializes our I/O pins
	pinMode(LED_GREEN,OUTPUT);
	pinMode(LED_YELLOW,OUTPUT);
	pinMode(LED_RED,OUTPUT);
	analogWrite(PWM_IR_OUT,0); // pinMode init not necessary for PWM function

	// Turns all LEDs on then off (0.5s + 0.5s)
	LED_ON(GREEN); LED_ON(YELLOW); LED_ON(RED);
	delay(500);
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	delay(500);

	// Initializes serial communications
	Serial.begin(BAUD_RATE);

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
	Serial.write((const uint8_t*)&dataPacket,sizeof(dataPacket));
	// Waits for about 1 sec...
	delay(1000);
	LED_TOGGLE(YELLOW);
}

int main(void) {
	init();
	setup();
	for(;;) {
		loop();
	}
	return 0;
}
