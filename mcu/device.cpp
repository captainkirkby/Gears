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
	analogWrite(PWM_IR_LED,0); // pinMode init not necessary for PWM function

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
	Serial.write((const uint8_t*)&bootPacket,sizeof(bootPacket));

	// Initializes the constant header of our data packet
	dataPacket.start[0] = START_BYTE;
	dataPacket.start[1] = START_BYTE;
	dataPacket.start[2] = START_BYTE;
	dataPacket.type = DATA_PACKET;
}

void loop() {
	if(bootPacket.bmpSensorOk) {
		// Reads BMP sensors
		bmpSensor.getTemperature(&dataPacket.temperature);
		bmpSensor.getPressure(&dataPacket.pressure);
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
