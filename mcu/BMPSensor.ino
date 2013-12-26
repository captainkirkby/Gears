#include <Wire.h>
#include <Adafruit_BMP085.h>

//Constants
const uint16_t BAUD_RATE = 9600;

//Sensor object
Adafruit_BMP085 sensor;

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
	Serial.print(sensor.readPressure());
	Serial.print(" ");
	Serial.print(sensor.readTemperature());
	Serial.println();

	delay(1000);
}
