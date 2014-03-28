#ifndef TWI_H
#define TWI_H

#include <util/twi.h>

#define TWI_READY 0
#define TWI_MRX   1
#define TWI_MTX   2
#define TWI_SRX   3
#define TWI_STX   4

static volatile uint8_t twi_state;

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
	// initialize our state
	twi_state = TWI_READY;

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

// This code follows the example in section 21.6
void twiRead(uint8_t address, const uint8_t *data, size_t len) {
	// send start condition
	TWCR = _BV(TWINT)|_BV(TWEN)|_BV(TWSTA);
	// wait for the start condition to complete transmitting
	while (!(TWCR & _BV(TWINT)));
	// check that the start condition was transmitted successfully
	if((TWSR & TW_STATUS_MASK) != TW_START) {
		LED_ON(RED);
		_delay_ms(2000);
	}
	// load the data register with the address we want, and set the READ direction bit
	TWDR = address | TW_READ;
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
