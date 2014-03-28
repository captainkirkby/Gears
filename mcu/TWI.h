#ifndef TWI_H
#define TWI_H

#include <util/twi.h>

// the port used for TWI
#define TWI_PORT	PORTC
#define TWI_DDR		DDRC

// TWI pins
#define PIN_SCL	0
#define PIN_SDA 1

// the TWI clock frequency to use
#define TWI_FREQ 100000L

// Based on the Arduino Wire library:
// https://github.com/arduino/Arduino/blob/master/libraries/Wire/utility/twi.c

void initTWI() {
	// configure TWI lines (SCL,SDA) as inputs with internal pullup
	TWI_DDR &= ~(_BV(PIN_SCL)|_BV(PIN_SDA));
	TWI_PORT |= _BV(PIN_SCL)|_BV(PIN_SDA);

	// initialize TWI prescaler value to 0 (section 21.9.3)
	TWSR &= ~(_BV(TWPS0)|_BV(TWPS1));

	// Calculate the TWI bitrate that gives the desired clock frequency for our system clock
	// (section 21.5.2). For F_CPU = 10MHz, TWI_FREQ = 0.1MHz, we use TWBR = 42.
	// TWBR must be >= 10 for master mode.
	TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

	// Enable the TWI module, which turns on slew-rate limiters and spike filters (section 21.9.2).
	// Note that we do not enable TWI interrupts.
	TWCR = _BV(TWEN);
}

// Perform a synchronous write of the specified data to the specified TWI bus address.
// This code follows the example in section 21.6
uint8_t twiWrite(uint8_t address, const uint8_t *data, size_t len) {
	// start transmitting start condition
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA);
	// wait for the start condition to complete transmitting
	while (!(TWCR & _BV(TWINT)));
	// check that the start condition was transmitted successfully
	if((TWSR & TW_STATUS_MASK) != TW_START) {
		flashDigit(1);
		return 1;
	}
	// load the data register with the address we want, and set the WRITE direction bit (SLA+W)
	TWDR = address | TW_WRITE;
	// start transmitting the address
	TWCR = _BV(TWINT) | _BV(TWEN);
	// wait for the address to complete transmitting
	while(!(TWCR & _BV(TWINT)));
	// check that the address was transmitted successfully and acknowledged with an ACK
	if((TWSR & TW_STATUS_MASK) != TW_MT_SLA_ACK) {
		flashDigit(2);
		return 2;
	}
	// loop over data bytes to write
	while(len--) {
		// load the data register with the next byte to write
		TWDR = *data++;
		// start transmitting this data byte
		TWCR = _BV(TWINT) | _BV(TWEN);
		// wait for the data byte to finish transmitting
		while(!(TWCR & _BV(TWINT)));
		// check that the data was transmitted successfully
		if((TWSR & TW_STATUS_MASK) != TW_MT_DATA_ACK) {
			flashDigit(3);
			return 3;
		}
	}
	// start transmitting stop condition
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
	// wait for stop condition to finish transmitting (TWINT is not set after a STOP!)
	while(TWCR & _BV(TWSTO));
	// all done
	return 0;
}

/*
static void read8(byte reg, uint8_t *value)
{
	Wire.beginTransmission((uint8_t)BMP085_ADDRESS);
	Wire.write((uint8_t)reg);
	Wire.endTransmission();
	Wire.requestFrom((uint8_t)BMP085_ADDRESS, (byte)1);
	*value = Wire.read();
	Wire.endTransmission();
}
*/

#endif
