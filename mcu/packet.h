#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define START_BYTE 0xFE
#define BOOT_PACKET 0x00
#define DATA_PACKET 0x01
#define CIRCULAR_BUFFER_LENGTH 3072
#define TSIP_START_BYTE 0x10
#define TSIP_STOP_BYTE1 0x10
#define TSIP_STOP_BYTE2 0x03

// Defines the boot packet structure
typedef struct {
    // Header
    uint8_t start[3];
    uint8_t type;
    // Serial number read from EEPROM at startup
    uint32_t serialNumber;          // Read offset  (0)
    // Stores the status code returned by initBMP180()
    uint8_t bmpSensorStatus;        //              (4)
    // Non-zero if our GPS serial communication was successfully initialized
    uint8_t gpsSerialOk;            //              (5)
    // Non-zero if our IR Sensor block is plugged in and working
    uint8_t sensorBlockOK;          //              (6)
    // Git info about the code we are running.
    uint32_t commitTimestamp;       //              (7)
    uint8_t commitID[20];           //              (11)
    uint8_t commitStatus;           //              (31)
    // Position information
    uint64_t latitude;          // double           (32)
    uint64_t longitude;         // double           (40)
    uint64_t altitude;          // double           (48)
    int32_t utcOffset;          // float            (56)
    uint16_t weekNumber;        //                  (60)
    uint32_t timeOfWeek;        // float            (62)
} BootPacket;

// Defines the data packet structure
typedef struct {
    // Header
    uint8_t start[3];
    uint8_t type;
    // Packet sequence number       // Read offset:
    uint32_t sequenceNumber;        // (0)
    // GPS timing info
    uint16_t gpsAlarms;             // (4)
    uint16_t gpsStatus;             // (6)
    int16_t utcOffset;              // (8)
    uint16_t weekNumber;            // (10)
    uint32_t timeOfWeek;            // (12)
    // Relative timing info (in ADC conversions (64*13/10MHz = 8.32e-5 s)
    // Now with respect to the last Boot Packet
    uint64_t timeSinceLastBootPacket;  // (16) 
    // BMP180 sensor measurements
    int32_t temperature;            // (24) divide by 160 to get degrees C
    int32_t pressure;               // (28) in Pascals
    // block thermistor reading
    uint16_t thermistor;            // (32)
    // relative humidity reading
    uint16_t humidity;              // (34)
    // PIN diode IR light level
    uint16_t irLevel;               // (36)
    // IR sensor raw ADC readings
    uint16_t rawPhase;              // (38)
    // GPS status updates
    uint8_t recieverMode;           // (40)
    uint8_t discipliningMode;       // (41)
    uint16_t criticalAlarms;        // (42)
    uint16_t minorAlarms;           // (44)
    uint8_t gpsDecodingStatus;      // (46)
    uint8_t discipliningActivity;   // (47)
    uint32_t clockOffset;           // (48)     // 4 byte float
    volatile uint8_t raw[CIRCULAR_BUFFER_LENGTH];    // (52)
} DataPacket;

// Command packet for turning auto messages off
typedef struct {
    // Header
    uint8_t header;
    uint8_t packetType;
    uint8_t packetSubType;
    // Payload
    uint8_t data[4];
    // Stop sequence
    uint8_t stop[2];
} TsipAutoManualPacket;

// Command packet for turning auto messages off
typedef struct {
    // Header
    uint8_t header;
    uint8_t packetType;
    // Payload
    uint8_t data[4];
    // Stop sequence
    uint8_t stop[2];
} TsipAutoManualPacketNoSubType;

// Command packet for requesting time
typedef struct {
    // Header
    uint8_t header;
    // Payload
    uint8_t command;
    // Stop sequence
    uint8_t stop[2];
} TsipCommandPacket;

// Command packet for requesting health
typedef struct {
    // Header
    uint8_t header;
    // Payload
    uint8_t packetType;
    uint8_t packetSubType;
    uint8_t data;
    // Stop sequence
    uint8_t stop[2];
} TsipHealthPacket;

// Response packet for auto messages
typedef struct {
    // Header
    uint8_t header;
    // Payload
    uint8_t packetType;
    uint8_t packetSubType;
    uint8_t data;
    // Stop sequence
    uint8_t stop[2];
} TsipAutoManualResponsePacket;

// Response packet for time
typedef struct {
    // Header
    uint8_t header;
    // Payload
    uint8_t responseCode;
    uint32_t timeOfWeek;
    uint16_t weekNumber;
    uint32_t gpsOffset;
    // Stop sequence
    uint8_t stop[2];
} TsipCommandResponsePacket;


// Response packet for health
typedef struct {
    // Header
    uint8_t header;
    // Payload
    uint8_t packetType;
    uint8_t packetSubType;
    uint8_t recieverMode;
    uint8_t discipliningMode;
    uint8_t selfSurveyProgress;
    uint32_t holdoverDuration;
    uint16_t criticalAlarms;
    uint16_t minorAlarms;
    uint8_t gpsDecodingStatus;
    uint8_t discipliningActivity;
    uint8_t spare1;
    uint8_t spare2;
    uint32_t ppsOffset;         // float
    uint32_t clockOffset;       // float
    uint32_t dacValue;
    uint32_t dacVoltage;        // float
    uint32_t temperature;       // float
    uint64_t latitude;          // double
    uint64_t longitude;         // double
    uint64_t altitude;          // double
    uint32_t ppsError;
    uint32_t spare3;
    // Stop sequence
    uint8_t stop[2];
} TsipHealthResponsePacket;

#endif
