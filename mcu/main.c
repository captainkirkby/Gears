/***************************************************************************
 * Top-level microcontroller firmware program.
 * 
 * Authors: David & Dylan Kirkby / Jan 2014.
 ***************************************************************************/

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

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

    // Copies our serial number from EEPROM address 0x10 into the boot packet
    bootPacket.serialNumber = eeprom_read_dword((uint32_t*)0x10);

    // Sends our boot packet.
    LED_ON(RED);
    serialWriteUSB((const uint8_t*)&bootPacket,sizeof(bootPacket));
    _delay_ms(500);
    LED_OFF(RED);

    while(1) {
        rippleUp();
        rippleDown();
    }
    return 0; // never actually reached
}
