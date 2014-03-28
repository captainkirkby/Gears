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

// Based on the Arduino Wire library and the avr-gcc twi example:
// https://github.com/arduino/Arduino/blob/master/libraries/Wire/utility/twi.c
// http://www.nongnu.org/avr-libc/examples/twitest/twitest.c

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

// Transmits a START condition synchronously and returns 1 if successful, else 0.
inline uint8_t twiStart() {
	// start transmitting start condition
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA);
	// wait for the start condition to complete transmitting
	while (!(TWCR & _BV(TWINT)));
	// check that the start condition was transmitted successfully
	// (note that we are assuming that this is not a repeated start here)
	if((TWSR & TW_STATUS_MASK) != TW_START) return 0;
	return 1;
}

// Transmits a STOP condition synchronously. There is no error condition to check
// so this has no return value, but will block until the stop condition has
// finished transmitting.
inline void twiStop() {
	// start transmitting stop condition
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
	// wait for stop condition to finish transmitting (TWINT is not set after a STOP!)
	while(TWCR & _BV(TWSTO));
}

#define TWI_WRITE_START_ERROR	0x1
#define TWI_WRITE_ADDR_ERROR	0x2
#define TWI_WRITE_DATA_ERROR	0x3
#define TWI_READ_START_ERROR	0x4
#define TWI_READ_ADDR_ERROR		0x5
#define TWI_READ_DATA_ERROR		0x6
#define TWI_READ_LAST_ERROR		0x7

// Perform a synchronous write of the specified data to the specified TWI bus address.
// This code follows the example in section 21.6. Returns an error code 1-3 or 0.
uint8_t twiWrite(uint8_t address, const uint8_t *data, size_t len) {
	// send START condition
	if(!twiStart()) return TWI_WRITE_START_ERROR;
	// load the data register with the address provided
	TWDR = (address&0xFE) | TW_WRITE;
	// start transmitting the address
	TWCR = _BV(TWINT) | _BV(TWEN);
	// wait for the address to complete transmitting
	while(!(TWCR & _BV(TWINT)));
	// check that the address was transmitted successfully and acknowledged with an ACK
	if((TWSR & TW_STATUS_MASK) != TW_MT_SLA_ACK) return TWI_WRITE_ADDR_ERROR;
	// loop over data bytes to write
	while(len--) {
		// load the data register with the next byte to write
		TWDR = *data++;
		// start transmitting this data byte
		TWCR = _BV(TWINT) | _BV(TWEN);
		// wait for the data byte to finish transmitting
		while(!(TWCR & _BV(TWINT)));
		// check that the data was transmitted successfully
		if((TWSR & TW_STATUS_MASK) != TW_MT_DATA_ACK) return 3;
	}
	// send STOP condition
	twiStop();
	// all done
	return 0;
}

// Perform a synchronous read into the buffer provided from the specified TWI bus address.
uint8_t twiRead(uint8_t address, uint8_t *data, size_t len) {
	// send START condition
	if(!twiStart()) return TWI_READ_START_ERROR;
	// load the data register with the address provided
	TWDR = (address&0xFE) | TW_READ;
	// start transmitting the address
	TWCR = _BV(TWINT) | _BV(TWEN);
	// wait for the address to complete transmitting
	while(!(TWCR & _BV(TWINT)));
	// check that the address was transmitted successfully and acknowledged with an ACK
	if((TWSR & TW_STATUS_MASK) != TW_MR_SLA_ACK) return TWI_READ_ADDR_ERROR;
	// loop over data bytes to read
	while(len--) {
		// send NOT ACK if this is the last byte, else ACK
		if(len > 0) {
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
		}
		else {
			TWCR = _BV(TWINT) | _BV(TWEN);
		}
		// wait for the next byte from the slave to finish transmitting
		while(!(TWCR & _BV(TWINT)));
		// save the data register into the next buffer byte
		*data++ = TWDR;
		// check for the expected response
		if(len > 0) {
			if((TWSR & TW_STATUS_MASK) != TW_MR_DATA_ACK) {
				return TWI_READ_DATA_ERROR;
			}
		}
		else {
			if((TWSR & TW_STATUS_MASK) != TW_MR_DATA_NACK) {
				return TWI_READ_LAST_ERROR;
			}
		}
	}
	// send STOP condition
	twiStop();
	// all done
	return 0;
}

#endif
