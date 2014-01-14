#ifndef UART_H
#define UART_H

//#define FTDI_BAUD_RATE	76800
#define FTDI_BAUD_RATE	9600
#define GPSDO_BAUD_RATE	9600

// writes one byte to the specified port (0 or 1)
#define PUTC(PORT,BYTE) { \
	// wait until the transmit buffer is empty \
	while(!(UCSR##PORT##A & _BV(UDRE##PORT))); \
	// write this byte to the transmit buffer \
	UDR##PORT = BYTE; \
}

void initUARTs() {
	#define BAUD FTDI_BAUD_RATE
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	// we are not using a double-speed baud rate clock
	UCSR0A &= ~(1 << U2X0);
	// enable Tx and Rx
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	// frame format is 8-n-1
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	#define BAUD GPSDO_BAUD_RATE
	#include <util/setbaud.h>
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	UCSR1A &= ~(1 << U2X1);
	// enable Tx and Rx
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
	// frame format is 8-n-1
	UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
}

#endif
