#ifndef TRIMBLE_H
#define TRIMBLE_H

#include <stdint.h>
#include <avr/io.h>

#include "LED.h"
#include "UART.h"
#include "packet.h"

#define DLE 0x10
#define WILD 0xDB

// RxBytes is a pointer to a PRE-ALLOCATED array of length numRxBytes
void readResponse(uint8_t numRxBytes, uint8_t rxBytes[]) {
    int ch;
    uint8_t c = 0;
    uint8_t escapeFlag = 0;
    // Read the given number of bytes
    while(c < numRxBytes){
        ch = getc1();
        if(ch > -1){            // Got Data
            uint8_t byte = (ch & 0xff);
            if(escapeFlag){
                escapeFlag = 0;
                // If two DLE's in a row, only record one
                if(byte == DLE) continue;
            }
            // Handle special DLE character
            if(!escapeFlag && byte == DLE) escapeFlag = 1;
            
            // Write to array
            // LED_ON(GREEN);
            // putc0(byte);
            rxBytes[c] = byte;
            c++;
        } else if(ch == -2){    // Framing Error
            LED_ON(YELLOW);
        } else if(ch == -3){    // Data Overrun
            LED_ON(RED);
        }
    }
}

uint8_t turnOffGPSAutoPackets(){
    uint8_t returnValue = 1;
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
            returnValue=0;           // error
        }
    }

    // Mask packet 35 to all zero too
    TsipAutoManualPacketNoSubType manualFetchPacket2;
    manualFetchPacket2.header = TSIP_START_BYTE;

    manualFetchPacket2.packetType = 0x35;

    manualFetchPacket2.data[0] = 0x00; 
    manualFetchPacket2.data[1] = 0x00;
    manualFetchPacket2.data[2] = 0x00;
    manualFetchPacket2.data[3] = 0x00;

    manualFetchPacket2.stop[0] = TSIP_STOP_BYTE1;
    manualFetchPacket2.stop[1] = TSIP_STOP_BYTE2;

    // Send Packet
    serialWriteGPS((const uint8_t*)&manualFetchPacket2,sizeof(manualFetchPacket2));

    // Read back 8 bytes of response
    uint8_t manualNumRxBytes2 = 8;
    uint8_t manualRxBytes2[manualNumRxBytes2];
    readResponse(manualNumRxBytes2, manualRxBytes2);

    // Confirm response
    uint8_t expectedManualRxBytes2[] = {
        TSIP_START_BYTE,
        0x55,                               // Packet Type
        0x00,0x00,0x00,0x00,                // Echo settings
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };
    for (int i = 0; i < manualNumRxBytes2; ++i) {
        if(expectedManualRxBytes2[i] != manualRxBytes2[i] && expectedManualRxBytes2[i] != WILD) {      // WILD is the arbitrary wildcard
            returnValue=0;           // error
        }
    }

    return returnValue;
}

TsipHealthResponsePacket getGPSHealth(){
    // Declare and define an (loosely) documented TSIP command packet that gets reciever health
    TsipHealthPacket healthPacket;
    healthPacket.header = TSIP_START_BYTE;
    healthPacket.packetType = 0x8E;
    healthPacket.packetSubType = 0xAC;
    healthPacket.data = 0;                          // This byte determines when the packet is sent.  0 = immediately
    healthPacket.stop[0] = TSIP_STOP_BYTE1;
    healthPacket.stop[1] = TSIP_STOP_BYTE2;

    // Send Packet
    serialWriteGPS((const uint8_t*)&healthPacket,sizeof(healthPacket));

    // Read back 72 bytes of response
    uint8_t healthNumRxBytes = 72;
    uint8_t healthRxBytes[healthNumRxBytes];
    readResponse(healthNumRxBytes, healthRxBytes);

    // Confirm response
    uint8_t expectedHealthRxBytes[] = {
        TSIP_START_BYTE,
        0x8F,                       // Packet Type
        0xAC,                       // Subtype
        WILD,                       // Reciever Mode
        WILD,                       // Disciplining Mode
        WILD,                       // Self Survey Progress
        WILD,WILD,WILD,WILD,        // Holdover Duration
        WILD,WILD,                  // Critical Alarms
        WILD,WILD,                  // Minor Alarms
        WILD,                       // GPS Decoding Status
        WILD,                       // Disciplining Activity
        WILD,                       // Spare 1
        WILD,                       // Spare 2
        WILD,WILD,WILD,WILD,        // PPS Offset (float) in ns
        WILD,WILD,WILD,WILD,        // Clock Offset (float) in ppb
        WILD,WILD,WILD,WILD,        // DAC Value (uint32_t)
        WILD,WILD,WILD,WILD,        // DAC Voltage (float)
        WILD,WILD,WILD,WILD,        // Temperature (float)
        WILD,WILD,WILD,WILD,        // Temperature (float)
        WILD,WILD,WILD,WILD,        // Latitude (double)
        WILD,WILD,WILD,WILD,        
        WILD,WILD,WILD,WILD,        // Longitude (double)
        WILD,WILD,WILD,WILD,       
        WILD,WILD,WILD,WILD,        // Altitude (double)
        WILD,WILD,WILD,WILD,       
        WILD,WILD,WILD,WILD,        // PPS Quantization Error (float)
        WILD,WILD,WILD,WILD,        // Spare 3-6
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };

    // Cast to pointer to struct type
    // packet = (TsipHealthResponsePacket *)healthRxBytes;

    // // Cast to struct type
    TsipHealthResponsePacket health = *((TsipHealthResponsePacket *)&healthRxBytes[0]);
    // uint64_t latitude = health.latitude;
    // uint64_t longitude = health.longitude;
    // uint64_t altitude = health.altitude;


    // serialWriteUSB((const uint8_t*)&healthRxBytes,sizeof(healthRxBytes));
    // serialWriteUSB((const uint8_t*)&latitude,sizeof(latitude));
    // serialWriteUSB((const uint8_t*)&longitude,sizeof(longitude));
    // serialWriteUSB((const uint8_t*)&altitude,sizeof(altitude));


    for (int i = 0; i < healthNumRxBytes; ++i) {
        if(expectedHealthRxBytes[i] != healthRxBytes[i] && expectedHealthRxBytes[i] != WILD) {     // WILD is the arbitrary wildcard
            // BAD!
        }
    }
    return health;
}

TsipCommandResponsePacket getTime(){
    TsipCommandPacket timePacket;
    timePacket.header = TSIP_START_BYTE;
    timePacket.command = 0x21;
    timePacket.stop[0] = TSIP_STOP_BYTE1;
    timePacket.stop[1] = TSIP_STOP_BYTE2;

    // Send Packet
    serialWriteGPS((const uint8_t*)&timePacket,sizeof(timePacket));

    // Read back 14 bytes of response
    uint8_t commandNumRxBytes = 14;
    uint8_t commandRxBytes[commandNumRxBytes];
    readResponse(commandNumRxBytes, commandRxBytes);

    serialWriteUSB((const uint8_t*)&commandRxBytes,sizeof(commandRxBytes));


    // Confirm response
    uint8_t expectedCommandRxBytes[] = {
        TSIP_START_BYTE,
        0x41,
        0xBE,0xBE,0xBE,0xBE,        // Time of week (float)
        0xBE,0xBE,                  // Week number (uint16_t)       // should be 1807 (as of now...)
        0xBE,0xBE,0xBE,0xBE,        // GPS-UTC offset (float)       // should be 16.00 ms
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };
    for (int i = 0; i < commandNumRxBytes; ++i) {
        if(expectedCommandRxBytes[i] != commandRxBytes[i] && expectedCommandRxBytes[i] != 0xBE) {     // 0xBE is the arbitrary wildcard
            LED_ON(RED);
        }
    }

    // Cast to struct type
    TsipCommandResponsePacket time = *((TsipCommandResponsePacket *)&commandRxBytes[0]);
    return time;
}

#endif