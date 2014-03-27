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
	TWCR = _BV(TWEA)|_BV(TWEN);
}

#endif
