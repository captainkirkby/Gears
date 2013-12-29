#include <Arduino.h>
#include <Adafruit_BMP085_U.h>

#include "packet.h"

//Constants
const uint16_t BAUD_RATE = 9600;

//Sensor object
Adafruit_BMP085_Unified sensor = Adafruit_BMP085_Unified(10085);

Packet packet;

void setup() {
	Serial.begin(BAUD_RATE);
	if(!sensor.begin()){
		Serial.println("No BMP085 Pressure Sensor detected... Check wiring.");
		while(true) {}
	} else {
		Serial.println("Initialized.\nData format: Pressure Temperature\nUnits: \nPressure = Pa\nTemperature = Celsius");
	}
}

void loop() {
	sensor.getTemperature(&packet.temperature);
	sensor.getPressure(&packet.pressure);
	Serial.print(packet.temperature);
	Serial.print(" ");
	Serial.print(packet.pressure);
	Serial.println();

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
