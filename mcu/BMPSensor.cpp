#include <Arduino.h>
#include <Adafruit_BMP085_U.h>

#include "packet.h"

//Constants
#define BAUD_RATE 115200

//Sensor object
Adafruit_BMP085_Unified sensor = Adafruit_BMP085_Unified(10085);

// Declares and initializes our boot info packet.
BootPacket bootPacket = {
	START_BYTE, START_BYTE, START_BYTE, BOOT_PACKET,
#ifdef COMMIT_INFO
    COMMIT_INFO
#elif
    0, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 0
#endif
};

DataPacket dataPacket;

void setup() {
	Serial.begin(BAUD_RATE);
	if(!sensor.begin()){
		Serial.println("No BMP085 Pressure Sensor detected... Check wiring.");
		while(true) {}
	}
	// Initializes the constant header of our data packet.
	dataPacket.start[0] = START_BYTE;
	dataPacket.start[1] = START_BYTE;
	dataPacket.start[2] = START_BYTE;
	dataPacket.type = DATA_PACKET;
	// Sends our boot packet.
	Serial.write((const uint8_t*)&bootPacket,sizeof(bootPacket));
}

void loop() {
	// Reads BMP180 sensors
	sensor.getTemperature(&dataPacket.temperature);
	sensor.getPressure(&dataPacket.pressure);
	// Sends binary packet data
	Serial.write((const uint8_t*)&dataPacket,sizeof(dataPacket));
	// Waits for about 1 sec...
	delay(1000);
}

int main(void) {
	init();
	setup();
	for(;;) {
		loop();
	}
	return 0;
}
