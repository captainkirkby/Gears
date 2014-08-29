#ifndef UART_H
#define UART_H

#define FTDI_BAUD_RATE	57600
#define GPSDO_BAUD_RATE	9600

// Writes one byte to the specified port (0 or 1). This is a synchronous operation
// and will block until the transmit buffer is available.
inline void putc0(uint8_t byte) {
	loop_until_bit_is_set(UCSR0A,UDRE0);
	UDR0 = byte;
}
inline void putc1(uint8_t byte) {
	loop_until_bit_is_set(UCSR1A,UDRE1);
	UDR1 = byte;
}

// Returns the next available byte from the specified port (0 or 1) or returns -1
// if the receive buffer is empty, -2 for a framing error or -3 for a data overrun.
inline int getc0() {
	if(UCSR0A & _BV(RXC0)) {
		// Rx buffer is full
		if(UCSR0A & _BV(FE0)) return -2; // framing error
		if(UCSR0A & _BV(DOR0)) return -3; // data overrun
		return (int)UDR0; // return contents of Rx buffer
	}
	else {
		return -1; // Rx buffer is empty
	}
}
inline int getc1() {
	if(UCSR1A & _BV(RXC1)) {
		// Rx buffer is full
		if(UCSR1A & _BV(FE1)) return -2; // framing error
		if(UCSR1A & _BV(DOR1)) return -3; // data overrun
		return (int)UDR1; // return contents of Rx buffer
	}
	else {
		return -1; // Rx buffer is empty
	}
}



void initUARTs() {
	#define BAUD FTDI_BAUD_RATE
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	// we are not using a double-speed baud rate clock
	UCSR0A &= ~_BV(U2X0);
	// enable Tx and Rx but not their interrupts
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	// frame format is 8-n-1
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	#define BAUD GPSDO_BAUD_RATE
	#include <util/setbaud.h>
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	UCSR1A &= ~_BV(U2X1);
	// enable Tx and Rx but not their interrupts
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
	// frame format is 8-n-1
	UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
}

// Performs a synchronous write of the specified binary data to the specified UART
void serialWriteUSB(const uint8_t *data, size_t len) {
	while(len--) putc0(*data++);
}
void serialWriteGPS(const uint8_t *data, size_t len) {
	while(len--) putc1(*data++);
}

#endif
