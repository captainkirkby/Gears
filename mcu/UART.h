#ifndef UART_H
#define UART_H

//#define FTDI_BAUD_RATE	76800
#define FTDI_BAUD_RATE	9600
#define GPSDO_BAUD_RATE	9600

void initUARTs() {
	#define BAUD FTDI_BAUD_RATE
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0A &= ~(1 << U2X0);
	#define BAUD GPSDO_BAUD_RATE
	#include <util/setbaud.h>
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	UCSR1A &= ~(1 << U2X1);
}

#endif
