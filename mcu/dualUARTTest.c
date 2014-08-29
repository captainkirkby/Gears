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
    TsipAutoManualPacket manualFetchPacket;
    manualFetchPacket.header = TSIP_START_BYTE;
    
    manualFetchPacket.packetType = 0x8E;
    manualFetchPacket.packetSubType = 0xA5;

    manualFetchPacket.data[0] = 0x00; 
    manualFetchPacket.data[1] = 0x00;
    manualFetchPacket.data[2] = 0x00;
    manualFetchPacket.data[3] = 0x00;

    manualFetchPacket.stop[0] = TSIP_STOP_BYTE1;
    manualFetchPacket.stop[1] = TSIP_STOP_BYTE2;

    // Send Packet
	serialWriteGPS((const uint8_t*)&manualFetchPacket,sizeof(manualFetchPacket));

	// Read back 9 bytes of response
	uint8_t numRxBytes = 9;
	uint8_t rxBytes[numRxBytes];
	int ch;
	uint8_t c = 0;
	uint8_t startReading = 0;
	while(c < numRxBytes){
		ch = getc1();
		if(ch > -1){			// Got Data
			uint8_t byte = (ch & 0xff);
			// Don't advance count until we see the start sequence
			if(byte == TSIP_START_BYTE){
				startReading = 1;
			} 
			if(startReading){
				LED_ON(GREEN);
				putc0(byte);
				rxBytes[c] = byte;
				c++;
			}
		} else if(ch == -2){	// Framing Error
			LED_ON(YELLOW);
		} else if(ch == -3){	// Data Overrun
			LED_ON(RED);
		}
	}

	// Confirm response
	uint8_t expectedRxBytes[] = {TSIP_START_BYTE,0x8F,manualFetchPacket.packetSubType,0,0,0,0,TSIP_STOP_BYTE1,TSIP_STOP_BYTE2};
	for (int i = 0; i < numRxBytes; ++i) {
		if(expectedRxBytes[i] != rxBytes[i]) {
			LED_ON(RED);
		}
	}



	// Declare and define a TSIP command packet that asks for the GPS time
	TsipCommandPacket commandPacket;
	commandPacket.header = TSIP_START_BYTE;

	commandPacket.command = 0x21;

    commandPacket.stop[0] = TSIP_STOP_BYTE1;
    commandPacket.stop[1] = TSIP_STOP_BYTE2;


	// Infinite Loop
	while(1);
}
