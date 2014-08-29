#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>

#define LED_DDR		DDRC
#define LED_PORT	PORTC
#define LED_PIN		5

asm("  .section .text\n");

int main(void) {
	/*
	// configure the yellow LED pin as an output
	LED_DDR |= _BV(LED_PIN);
	// flash the LED
	for(uint8_t i = 0; i < 10; i++) {
		// turn the LED on
		LED_PORT = 0x00;
		// wait 200ms
		_delay_ms(200.0);
		// turn the LED off
		LED_PORT = 0xff;
	}
	// wait another second
	_delay_ms(1000.0);
	*/
	// jump to the main program
	asm("jmp 0x0000");
}
