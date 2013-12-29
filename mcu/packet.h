#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define PACKET_HEADER 0xFEFEFEFE

// Defines the packet data structure
typedef struct {
	// Header
	uint32_t header;
	// BMP180 sensor measurements
	int32_t temperature;	// divide by 160 to get degrees C
	int32_t pressure;		// in Pascals
} Packet;

#endif
