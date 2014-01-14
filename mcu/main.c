/***************************************************************************
 * Top-level microcontroller firmware program.
 * 
 * Authors: David & Dylan Kirkby / Jan 2014.
 ***************************************************************************/

#include <avr/io.h>
#include <util/delay.h>

// bit numbers in port C
#define LED_GREEN	5
#define LED_YELLOW	6
#define LED_RED		7

void initLEDs() {
	// initialize port C pins for output
	DDRC = _BV(LED_GREEN) | _BV(LED_YELLOW) | _BV(LED_RED);
	// turn all LEDs off (high)
	PORTC = _BV(LED_GREEN) | _BV(LED_YELLOW) | _BV(LED_RED);
}

int main(void)
{
	initLEDs();

    for(;;){
    	_delay_ms(500);
    	PORTC ^= ~_BV(LED_YELLOW);
    }
    return 0;   /* never reached */
}
