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

#include "packet.h"

// Declares and initializes our boot info packet
BootPacket bootPacket = {
    START_BYTE, START_BYTE, START_BYTE, BOOT_PACKET,
    0,0,0,
#ifdef COMMIT_INFO
    COMMIT_INFO
#else
    0, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 0
#endif
};

// Declares the data packet we will transmit periodically
DataPacket dataPacket;

int main(void)
{
	initLEDs();
	initUARTs();

	putc0('0');
	putc1('1');

    while(1) {
    	// Copy any input on port 0 to port 1
    	int c0 = getc0();
    	if(c0 < -1) {
    		LED_TOGGLE(RED);
    	}
    	else if(c0 >= 0) {
    		LED_TOGGLE(GREEN);
    		putc1((char)c0);
    	}
    	// Copy any input on port 1 to port 0
    	int c1 = getc1();
    	if(c1 < -1) {
    		LED_TOGGLE(RED);
    	}
    	else if(c1 >= 0) {
    		LED_TOGGLE(YELLOW);
    		putc0((char)c1);
    	}
    }
    return 0; // never actually reached
}
