#ifndef UART_H
#define UART_H

// #define FTDI_BAUD_RATE 78125
// #define GPSDO_BAUD_RATE	9600
// NOTE: these values are just for documentation,
// to actually set them, set UBRR0 and UBRR1 
// in the initUARTs() function below

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
	// Set baud rate of UART0 (computer) to 78125
	// Assuming we are not using a double-speed baud rate
	// clock the equation for baud rate is shown below:
	//		Baud Rate = F_CPU / (16 * (UBRR + 1))
	//		Example 1: 78125 = 10MHz / (16 * (7+1))
	//		Example 2: ~9600 = 10MHz / (16 * (64+1))	(within .2 %)
	UBRR0H = 0;
	UBRR0L = 7;
	// we are not using a double-speed baud rate clock
	UCSR0A &= ~_BV(U2X0);
	// enable Tx and Rx but not their interrupts
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	// frame format is 8-n-1
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	// Set baud rate of UART1 (gps) to 9600
	UBRR1H = 0;
	UBRR1L = 64;
	// we are not using a double-speed baud rate clock
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
