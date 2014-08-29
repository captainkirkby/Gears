#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "LED.h"
#include "IR.h"
#include "UART.h"

#include "packet.h"

int main(void) {
	initLEDs();
	initUARTs();

	// Declare and define a TSIP Packet to stop automatic packet transmission
    TsipPacket tsipPacket;
    tsipPacket.header = TSIP_START_BYTE;
    
    tsipPacket.packetType = 0x8E;
    tsipPacket.packetSubType = 0xA5;

    tsipPacket.data[0] = 0x00; 
    tsipPacket.data[1] = 0x00;
    tsipPacket.data[2] = 0x00;
    tsipPacket.data[3] = 0x00;

    tsipPacket.stop[0] = TSIP_STOP_BYTE1;
    tsipPacket.stop[1] = TSIP_STOP_BYTE2;

	// uint32_t msg = 0xDEADBEEF;
	LED_ON(GREEN);
	serialWriteGPS((const uint8_t*)&tsipPacket,sizeof(tsipPacket));

	int ch;
	while(1){
		ch = getc1();
		if(ch > -2){
			LED_ON(YELLOW)
			putc0(ch);
		} else {
			LED_ON(RED);
		}
	}
}
