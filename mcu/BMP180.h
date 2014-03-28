#ifndef BMP180_H
#define BMP180_H

// The TWI address to use (with the LSB set to zero, as for a write).
// See section 5.2 of the datasheet.
#define BMP180_ADDRESS 0xEE

// The hardcoded chip ID returned by a BMP180
#define BMP180_CHIPID 0x55

// The BMP180 register addresses
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
#define BMP180_REGISTER_TEMPDATA        0xF6
#define BMP180_REGISTER_PRESSUREDATA    0xF6
#define BMP180_REGISTER_READTEMPCMD     0x2E
#define BMP180_REGISTER_READPRESSURECMD 0x34

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

// Performs a synchronous read of the specified register and saves the results in
// the buffer provided. Returns a TWI error or 0 if successful.
uint8_t readBMP180(uint8_t reg, uint8_t *data, size_t len) {
	uint8_t error;
	error = twiWrite(BMP180_ADDRESS,&reg,sizeof(reg));
	if(error) return error;
	error = twiRead(BMP180_ADDRESS,data,len);
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
	error = readBMP180(BMP180_REGISTER_CHIPID,&id,sizeof(id));
	if(error) return 10+error;
	// check that we got the expected chip ID
	if(id != BMP180_CHIPID) return 20;
	// read the chip's internal calibration coefficients
	error =
		readBMP180(BMP180_REGISTER_CAL_AC1, (uint8_t*)&calib.ac1,2) |
		readBMP180(BMP180_REGISTER_CAL_AC1, (uint8_t*)&calib.ac1,2) |
		readBMP180(BMP180_REGISTER_CAL_AC2, (uint8_t*)&calib.ac2,2) |
		readBMP180(BMP180_REGISTER_CAL_AC3, (uint8_t*)&calib.ac3,2) |
		readBMP180(BMP180_REGISTER_CAL_AC4, (uint8_t*)&calib.ac4,2) |
		readBMP180(BMP180_REGISTER_CAL_AC5, (uint8_t*)&calib.ac5,2) |
		readBMP180(BMP180_REGISTER_CAL_AC6, (uint8_t*)&calib.ac6,2) |
		readBMP180(BMP180_REGISTER_CAL_B1,  (uint8_t*)&calib.b1, 2) |
		readBMP180(BMP180_REGISTER_CAL_B2,  (uint8_t*)&calib.b2, 2) |
		readBMP180(BMP180_REGISTER_CAL_MB,  (uint8_t*)&calib.mb, 2) |
		readBMP180(BMP180_REGISTER_CAL_MC,  (uint8_t*)&calib.mc, 2) |
		readBMP180(BMP180_REGISTER_CAL_MD,  (uint8_t*)&calib.md, 2);
	if(error) return 30+error;
	// all done with no errors
	return 0;
}

#endif
