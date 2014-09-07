#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "LED.h"
#include "IR.h"
#include "UART.h"

#include "packet.h"

#define DLE 0x10

void readResponse(uint8_t numRxBytes, uint8_t *rxBytes);

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
    uint8_t manualNumRxBytes = 9;
    uint8_t manualRxBytes[manualNumRxBytes];
    readResponse(manualNumRxBytes, manualRxBytes);

    // serialWriteUSB((const uint8_t*)&manualRxBytes,sizeof(manualRxBytes));

    // Confirm response
    uint8_t expectedManualRxBytes[] = {TSIP_START_BYTE,0x8F,manualFetchPacket.packetSubType,0,0,0,0,TSIP_STOP_BYTE1,TSIP_STOP_BYTE2};
    for (int i = 0; i < manualNumRxBytes; ++i) {
        if(expectedManualRxBytes[i] != manualRxBytes[i] && expectedManualRxBytes[i] != 0xBE) {      // 0xBE is the arbitrary wildcard
            LED_ON(RED);
        }
    }





    // Declare and define a TSIP command packet that asks for the GPS time
    TsipCommandPacket timePacket;
    timePacket.header = TSIP_START_BYTE;

    timePacket.command = 0x21;

    timePacket.stop[0] = TSIP_STOP_BYTE1;
    timePacket.stop[1] = TSIP_STOP_BYTE2;

    // Send Packet
    serialWriteGPS((const uint8_t*)&timePacket,sizeof(timePacket));

    // Read back 14 bytes of response
    uint8_t commandNumRxBytes = 15;
    uint8_t commandRxBytes[commandNumRxBytes];
    readResponse(commandNumRxBytes, commandRxBytes);

    serialWriteUSB((const uint8_t*)&commandRxBytes,sizeof(commandRxBytes));


    // Confirm response
    uint8_t expectedCommandRxBytes[] = {
        TSIP_START_BYTE,
        0x41,
        0xBE,0xBE,0xBE,0xBE,        // Time of week (float)
        0x07,0xBE,                  // Week number (uint16_t)       // should be 1807 (as of now...)
        0x41,0x80,0x00,0x00,        // GPS-UTC offset (float)       // should be 16.00 ms
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };
    for (int i = 0; i < commandNumRxBytes; ++i) {
        if(expectedCommandRxBytes[i] != commandRxBytes[i] && expectedCommandRxBytes[i] != 0xBE) {     // 0xBE is the arbitrary wildcard
            LED_ON(RED);
        }
    }

    uint16_t week = (commandRxBytes[6] << 8) | commandRxBytes[7];
    uint16_t weekCopy = week;
    week = week/2;
    // week = week+1;
    // serialWriteUSB((const uint8_t*)&week,sizeof(week));
    week = weekCopy+1;
    // serialWriteUSB((const uint8_t*)&week,sizeof(week));
    // serialWriteUSB((const uint8_t*)&weekCopy,sizeof(weekCopy));






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
        0xBE,                       // Reciever Mode
        0xBE,                       // Disciplining Mode
        0xBE,                       // Self Survey Progress
        0xBE,0xBE,0xBE,             // Holdover Duration
        0xBE,0xBE,                  // Critical Alarms
        0xBE,0xBE,                  // Minor Alarms
        0xBE,                       // GPS Decoding Status
        0xBE,                       // Disciplining Activity
        0xBE,                       // Spare 1
        0xBE,                       // Spare 2
        0xBE,0xBE,0xBE,0xBE,        // PPS Offset (float) in ns
        0xBE,0xBE,0xBE,0xBE,        // Clock Offset (float) in ppb
        0xBE,0xBE,0xBE,0xBE,        // DAC Value (uint32_t)
        0xBE,0xBE,0xBE,0xBE,        // DAC Voltage (float)
        0xBE,0xBE,0xBE,0xBE,        // Temperature (float)
        0xBE,0xBE,0xBE,0xBE,        // Temperature (float)
        0xBE,0xBE,0xBE,0xBE,        // Latitude (double)
        0xBE,0xBE,0xBE,0xBE,        
        0xBE,0xBE,0xBE,0xBE,        // Longitude (double)
        0xBE,0xBE,0xBE,0xBE,       
        0xBE,0xBE,0xBE,0xBE,        // Altitude (double)
        0xBE,0xBE,0xBE,0xBE,       
        0xBE,0xBE,0xBE,0xBE,        // PPS Quantization Error (float)
        0xBE,0xBE,0xBE,0xBE,        // Spare 3-6
        TSIP_STOP_BYTE1,TSIP_STOP_BYTE2
    };

    serialWriteUSB((const uint8_t*)&healthRxBytes,sizeof(healthRxBytes));

    for (int i = 0; i < healthNumRxBytes; ++i) {
        if(expectedHealthRxBytes[i] != healthRxBytes[i] && expectedHealthRxBytes[i] != 0xBE) {     // 0xBE is the arbitrary wildcard
            LED_ON(RED);
        }
    }

    // Infinite Loop
    while(1);
}

// RxBytes is a pointer to a PRE-ALLOCATED array of length numRxBytes
void readResponse(uint8_t numRxBytes, uint8_t rxBytes[]) {
    int ch;
    uint8_t c = 0;
    uint8_t startReading = 1;
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
            LED_ON(GREEN);
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

