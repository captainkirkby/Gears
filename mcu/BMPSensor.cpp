#include <Arduino.h>
#include <Adafruit_BMP085_U.h>

#include "packet.h"

//Constants
#define BAUD_RATE 115200

//Sensor object
Adafruit_BMP085_Unified sensor = Adafruit_BMP085_Unified(10085);

Packet packet;

void setup() {
	Serial.begin(BAUD_RATE);
	if(!sensor.begin()){
		Serial.println("No BMP085 Pressure Sensor detected... Check wiring.");
		while(true) {}
	}
	packet.start[0] = START_BYTE;
	packet.start[1] = START_BYTE;
	packet.start[2] = START_BYTE;
	packet.type = DATA_PACKET;
}

void loop() {
	// Reads BMP180 sensors
	sensor.getTemperature(&packet.temperature);
	sensor.getPressure(&packet.pressure);
	// Sends binary packet data
	Serial.write((const uint8_t*)&packet,sizeof(packet));
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
