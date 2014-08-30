#ifndef TRIMBLE_H
#define TRIMBLE_H

#include <stdint.h>
#include <avr/io.h>

#include "LED.h"
#include "UART.h"
#include "packet.h"

// RxBytes is a pointer to a PRE-ALLOCATED array of length numRxBytes
void readResponse(uint8_t numRxBytes, uint8_t rxBytes[]) {
    int ch;
    uint8_t c = 0;
    uint8_t startReading = 1;
    // Read the given number of bytes
    while(c < numRxBytes){
        ch = getc1();
        if(ch > -1){            // Got Data
            uint8_t byte = (ch & 0xff);     // Lower Byte
            if(startReading){
                LED_ON(GREEN);
                // putc0(byte);
                rxBytes[c] = byte;
                c++;
            }
        } else if(ch == -2){    // Framing Error
            LED_ON(YELLOW);
        } else if(ch == -3){    // Data Overrun
            LED_ON(RED);
        }
    }
}

uint8_t turnOffGPSAutoPackets(){
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
    uint8_t manualNumRxBytes = 9;
    uint8_t manualRxBytes[manualNumRxBytes];
    readResponse(manualNumRxBytes, manualRxBytes);

        // Confirm response
    uint8_t expectedManualRxBytes[] = {
        TSIP_START_BYTE,
        0x8F,                               // Packet Type
        0xA5,                               // Subpacket
        0x00,0x00,0x00,0x00,                // Echo settings
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };
    for (int i = 0; i < manualNumRxBytes; ++i) {
        if(expectedManualRxBytes[i] != manualRxBytes[i] && expectedManualRxBytes[i] != WILD) {      // WILD is the arbitrary wildcard
            return 0;           // error
        }
    }
    return 1;
}

#endif