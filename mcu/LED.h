#ifndef LED_H
#define LED_H

// the port that the LEDs are connected to
#define LED_PORT	PORTC
#define LED_DDR		DDRC

// bit numbers for each LED color
#define LED_GREEN	4
#define LED_YELLOW	5
#define LED_RED		6

// use upper-case color names (with the LED_ prefix) in these macros
#define LED_ON(X) 	{ LED_PORT &= ~_BV(LED_##X); }
#define LED_OFF(X)	{ LED_PORT |= _BV(LED_##X); }
#define LED_TOGGLE(X) { LED_PORT ^= _BV(LED_##X); }

// initializes the LEDs at startup
void initLEDs() {
	// configure pins for output
	LED_DDR |= _BV(LED_GREEN) | _BV(LED_YELLOW) | _BV(LED_RED);
	// turn all LEDs on (low) to test that they are working
	LED_PORT &= ~(_BV(LED_GREEN) | _BV(LED_YELLOW) | _BV(LED_RED));
	_delay_ms(250);
	// turn all LEDs off (high)
	LED_PORT |= _BV(LED_GREEN) | _BV(LED_YELLOW) | _BV(LED_RED);
	_delay_ms(250);
}

void rippleUp() {
	// Quickly ripple through GREEN-YELLOW-RED to indicate the start of a new digit
	LED_ON(GREEN); _delay_ms(50);
	LED_OFF(GREEN); LED_ON(YELLOW); _delay_ms(50);
	LED_OFF(YELLOW); LED_ON(RED); _delay_ms(50);
	LED_OFF(RED); _delay_ms(350);	
}

void rippleDown() {
	// Quickly ripple through GREEN-YELLOW-RED to indicate the start of a new digit
	LED_ON(RED); _delay_ms(50);
	LED_OFF(RED); LED_ON(YELLOW); _delay_ms(50);
	LED_OFF(YELLOW); LED_ON(GREEN); _delay_ms(50);
	LED_OFF(GREEN); _delay_ms(350);	
}

void flashDigit(int digit) {
	// Turn all LEDs off
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	rippleUp();
	// Add up the number of LEDs turned on to get the digit
	while(digit > 3) {
		LED_ON(GREEN); LED_ON(YELLOW); LED_ON(RED);
		_delay_ms(500);
		LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
		_delay_ms(500);
		digit -= 3;
	}
	switch(digit) {
		case 3:
			LED_ON(RED);
		case 2:
			LED_ON(YELLOW);
		case 1:
			LED_ON(GREEN);
	}
	_delay_ms(500);
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	_delay_ms(500);
}

void flashNumber(uint32_t value) {
	while(value) {
		flashDigit(value % 10);
		value /= 10;
	}
	rippleDown();
}

#endif
