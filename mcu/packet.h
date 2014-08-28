#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define START_BYTE 0xFE
#define BOOT_PACKET 0x00
#define DATA_PACKET 0x01
#define PACKET_LENGTH 2048

// Defines the boot packet structure
typedef struct {
	// Header
	uint8_t start[3];
	uint8_t type;
	// Serial number read from EEPROM at startup
	uint32_t serialNumber;
	// Stores the status code returned by initBMP180()
	uint8_t bmpSensorStatus;
	// Non-zero if our GPS serial communication was successfully initialized
	uint8_t gpsSerialOk;
	// Non-zero if our IR Sensor block is plugged in and working
	uint8_t sensorBlockOK;
	// Git info about the code we are running.
    uint32_t commitTimestamp;
    uint8_t commitID[20];
    uint8_t commitStatus;
} BootPacket;

// Defines the data packet structure
typedef struct {
	// Header
	uint8_t start[3];
	uint8_t type;
	// Packet sequence number		// Read offset:
	uint32_t sequenceNumber;		// (0)
	// GPS timing info
	uint16_t gpsAlarms;				// (4)
	uint16_t gpsStatus;				// (6)
	int16_t utcOffset;				// (8)
	uint16_t weekNumber;			// (10)
	uint32_t timeOfWeek;			// (12)
	// Relative timing info (in ADC conversions (128*13/10MHz = 1.664e-4 s)
	uint16_t timeSinceLastReading;	// (16) 
	// BMP180 sensor measurements
	int32_t temperature;			// (18) divide by 160 to get degrees C
	int32_t pressure;				// (22) in Pascals
	// block thermistor reading
	uint16_t thermistor;			// (26)
	// relative humidity reading
	uint16_t humidity;				// (28)
	// PIN diode IR light level
	uint16_t irLevel;				// (30)
	// IR sensor raw ADC readings
	uint16_t rawPhase;				// (32)
	volatile uint8_t raw[PACKET_LENGTH];	// (34)
} DataPacket;

typedef struct {
	// Struct elements
} tsipPacket;

#endif
