#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define START_BYTE 0xFE
#define DATA_PACKET 0x01
#define NUM_RAW_BYTES 8

// Defines the packet data structure
typedef struct {
	// Header
	uint8_t start[3];
	uint8_t type;
	// GPS timing info
	uint16_t gpsAlarms;
	uint16_t gpsStatus;
	int16_t utcOffset;
	uint16_t weekNumber;
	uint32_t timeOfWeek;
	// BMP180 sensor measurements
	int32_t temperature;	// divide by 160 to get degrees C
	int32_t pressure;		// in Pascals
	// block thermistor reading
	uint16_t thermistor;	// in ADC counts
	// IR sensor raw ADC readings
	uint16_t rawPhase;
	uint8_t raw[NUM_RAW_BYTES];
} Packet;

#endif
