#ifndef BMP180_H
#define BMP180_H

// The TWI address to use (with the LSB set to zero, as for a write).
// See section 5.2 of the datasheet.
#define BMP180_ADDRESS 0xEE

// The hardcoded chip ID returned by a BMP180
#define BMP180_CHIPID 0x55

// The BMP180 register addresses (from datasheet Figure 6)
#define BMP180_REGISTER_CAL_AC1         0xAA
#define BMP180_REGISTER_CAL_AC2         0xAC
#define BMP180_REGISTER_CAL_AC3         0xAE
#define BMP180_REGISTER_CAL_AC4         0xB0
#define BMP180_REGISTER_CAL_AC5         0xB2
#define BMP180_REGISTER_CAL_AC6         0xB4
#define BMP180_REGISTER_CAL_B1          0xB6
#define BMP180_REGISTER_CAL_B2          0xB8
#define BMP180_REGISTER_CAL_MB          0xBA
#define BMP180_REGISTER_CAL_MC          0xBC
#define BMP180_REGISTER_CAL_MD          0xBE
#define BMP180_REGISTER_CHIPID          0xD0
#define BMP180_REGISTER_VERSION         0xD1
#define BMP180_REGISTER_SOFTRESET       0xE0
#define BMP180_REGISTER_CONTROL         0xF4
#define BMP180_REGISTER_OUTPUT_MSB      0xF6
#define BMP180_REGISTER_OUTPUT_LSB      0xF7
#define BMP180_REGISTER_OUTPUT_XLSB     0xF8

// Predefined values for writing into the control register (from datasheet Table 8)
#define BMP180_READ_TEMPERATURE         0x2E
#define BMP180_READ_PRESSURE_OSS0       0x34
#define BMP180_READ_PRESSURE_OSS1       0x74
#define BMP180_READ_PRESSURE_OSS2       0xB4
#define BMP180_READ_PRESSURE_OSS3       0xF4

// Max conversion times in millsecs (from datasheet Table 8, rounded up to nearest int)
#define BMP180_MAX_CONV_TIME_TEMPERATURE     5
#define BMP180_MAX_CONV_TIME_PRESSURE_OSS0   5
#define BMP180_MAX_CONV_TIME_PRESSURE_OSS1   8
#define BMP180_MAX_CONV_TIME_PRESSURE_OSS2  14
#define BMP180_MAX_CONV_TIME_PRESSURE_OSS3  26

// The BMP180 calibration data format
typedef struct {
	int16_t  ac1;
	int16_t  ac2;
	int16_t  ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t  b1;
	int16_t  b2;
	int16_t  mb;
	int16_t  mc;
	int16_t  md;
} BMP180CalibrationData;
static BMP180CalibrationData calib;

// The BMP180 command request format
typedef struct {
	uint8_t	regAddress; // should always be BMP180_REGISTER_CONTROL
	uint8_t command;
} BMP180CommandRequest;
static BMP180CommandRequest cmdreq;

// Swaps nswap consecutive pairs of bytes in place, e.g. with nswap=3:
// B1 B2 B3 B4 B5 B5 => B2 B1 B4 B3 B6 B5
// This is necessary to convert the TWI buffer byte order into the order required for (u)int16_t
inline void swapBytes(uint8_t *buffer, uint8_t nswap) {
	uint8_t msb;
	// swap MSB LSB => LSB MSB nswap times
	while(nswap--) {
		// save MSB
		msb = *buffer;
		// overwrite MSB with LSB
		*buffer++ = *buffer;
		// restore MSB
		*buffer++ = msb;
	}
}

// Performs a synchronous read of the specified register and saves the results in
// the buffer provided. Returns a TWI error or 0 if successful.
uint8_t readBMP180Register(uint8_t regAddress, uint8_t *data, size_t len) {
	uint8_t error;
	error = twiWrite(BMP180_ADDRESS,&regAddress,sizeof(regAddress));
	if(error) return error;
	error = twiRead(BMP180_ADDRESS,data,len);
	if(error) return error;
	return 0;
}

// Sends the specified command and associated value synchronously to the BMP180 device.
uint8_t sendBMP180Command(uint8_t command) {
	// initialize the command request payload
	cmdreq.regAddress = BMP180_REGISTER_CONTROL;
	cmdreq.command = command;
	uint8_t error;
	error = twiWrite(BMP180_ADDRESS,(uint8_t*)&cmdreq,sizeof(cmdreq));
	if(error) return error;	
	return 0;
}

// Initializes communications with the BMP sensor. The possible returned error codes are:
// 1X = readback of TWI chip ID failed, probably indicating general TWI problem
// 20 = got unexpected chip ID
// 3X = error reading calibration data
uint8_t initBMP180() {
	// read the BMP chip ID
	uint8_t error,id;
	error = readBMP180Register(BMP180_REGISTER_CHIPID,&id,sizeof(id));
	if(error) return 10+error;
	// check that we got the expected chip ID
	if(id != BMP180_CHIPID) return 20;
	// read the chip's internal calibration coefficients
	error =
		readBMP180Register(BMP180_REGISTER_CAL_AC1, (uint8_t*)&calib.ac1, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_AC2, (uint8_t*)&calib.ac2, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_AC3, (uint8_t*)&calib.ac3, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_AC4, (uint8_t*)&calib.ac4, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_AC5, (uint8_t*)&calib.ac5, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_AC6, (uint8_t*)&calib.ac6, 2) |
		readBMP180Register(BMP180_REGISTER_CAL_B1,  (uint8_t*)&calib.b1,  2) |
		readBMP180Register(BMP180_REGISTER_CAL_B2,  (uint8_t*)&calib.b2,  2) |
		readBMP180Register(BMP180_REGISTER_CAL_MB,  (uint8_t*)&calib.mb,  2) |
		readBMP180Register(BMP180_REGISTER_CAL_MC,  (uint8_t*)&calib.mc,  2) |
		readBMP180Register(BMP180_REGISTER_CAL_MD,  (uint8_t*)&calib.md,  2);
	if(error) return 30+error;
	// swap bytes
	swapBytes((uint8_t*)&calib.ac1,1);
	swapBytes((uint8_t*)&calib.ac2,1);
	swapBytes((uint8_t*)&calib.ac3,1);
	swapBytes((uint8_t*)&calib.ac4,1);
	swapBytes((uint8_t*)&calib.ac5,1);
	swapBytes((uint8_t*)&calib.ac6,1);
	swapBytes((uint8_t*)&calib.b1, 1);
	swapBytes((uint8_t*)&calib.b2, 1);
	swapBytes((uint8_t*)&calib.mb, 1);
	swapBytes((uint8_t*)&calib.mc, 1);
	swapBytes((uint8_t*)&calib.md, 1);
	// all done with no errors
	return 0;
}

// Performs a synchronous read of the BMP180 temperature and pressure sensors using
// the maximum (8x) oversampling for pressure. Results are stored using the pointers provided.
// Units are Pascals and degC * 160.  Returns an error code or zero if successful.
// Invalid results will be signalled as 0xFFFFFFFF. Possible error codes are:
//
//   1X = failed to start temperature measurement
//   2X = failed to read temperature measurement
//   3X = failed to start pressure measurement
//   4X = failed to read pressure measurement
//
// where X = 1-7 is one of the TWI_*_ERROR values defined in TWI.h. The total time required
// by this function is about 32ms.
uint8_t readBMP180Sensors(int32_t *temperature, int32_t *pressure) {
	// initialize the results to be invalid.
	*temperature = 0xFFFFFFFF;
	*pressure = 0xFFFFFFFF;
	uint8_t error;
	// initiate a temperature reading
	error = sendBMP180Command(BMP180_READ_TEMPERATURE);
	if(error) return 10+error;
	// wait for the temperature conversion to complete
	_delay_ms(BMP180_MAX_CONV_TIME_TEMPERATURE);
	// read back the raw 16-bit temperature ADC value
	uint16_t raw;
	error = readBMP180Register(BMP180_REGISTER_OUTPUT_MSB,(uint8_t*)&raw,sizeof(raw));
	if(error) return 20+error;
	// swap bytes
	swapBytes((uint8_t*)&raw,1);
	int32_t UT = raw;
	// convert the ADC value to degC * 160 (following Fig.4 of the datasheet)
	int32_t X1 = (((int32_t)UT - calib.ac6)*calib.ac5) >> 15;
	int32_t X2 = (((int32_t)calib.mc) << 11)/(X1+calib.md);
	*temperature = X1 + X2 + 8;
	/*
	int32_t X1 = (UT - (int32_t)calib.ac6) * ((int32_t)calib.ac5) / pow(2,15);
	int32_t X2 = ((int32_t)calib.mc * pow(2,11)) / (X1+(int32_t)calib.md);
	int32_t B5 = X1 + X2;
	*temperature = B5 + 8;
	*/
	// initiate a pressure reading with maximum oversampling (8x)
	error = sendBMP180Command(BMP180_READ_PRESSURE_OSS3);
	if(error) return 30+error;
	// wait for the pressure conversion to complete
	_delay_ms(BMP180_MAX_CONV_TIME_PRESSURE_OSS3);
	// read back the raw 19-bit pressure ADC value (we only fill 3 of the 4 bytes)
	uint32_t UP;
	error = readBMP180Register(BMP180_REGISTER_OUTPUT_MSB,(uint8_t*)&UP,3);
	if(error) return 40+error;
	*pressure = UP;
	// all done
	return 0;
}

#endif
