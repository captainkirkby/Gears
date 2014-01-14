/***************************************************************************
 * Top-level microcontroller firmware program.
 * 
 * Authors: David & Dylan Kirkby / Jan 2014.
 ***************************************************************************/

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

#include "LED.h"
#include "UART.h"

int main(void)
{
	initLEDs();
	initUARTs();

    for(;;){
    	_delay_ms(500);
    	LED_TOGGLE(YELLOW);
    }
    return 0;   /* never reached */
}
