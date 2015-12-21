#ifndef BMECONSTANTS_H
#define BMECONSTANTS_H

// The I2C address used by the BME280
// Note: the SDO pin is tied to ground which sets the
// LSB of the address to zero.
#define BME280_ADDRESS			0x76

// The hardcoded chip ID returned by a BME280
#define BME280_CHIPID			0x60

// The BME280 register addresses (from datasheet Table 18)
// Partial Memory Map
// Sensor Readouts
#define BME280_REGISTER_HUM_LSB								0xFE
#define BME280_REGISTER_HUM_MSB								0xFD
#define BME280_REGISTER_TEMP_XLSB							0xFC
#define BME280_REGISTER_TEMP_LSB							0xFB
#define BME280_REGISTER_TEMP_MSB							0xFA
#define BME280_REGISTER_PRES_XLSB							0xF9
#define BME280_REGISTER_PRES_LSB							0xF8
#define BME280_REGISTER_PRES_MSB							0xF7
// IIR Filter Setting
#define BME280_REGISTER_FILTER								0xF5
// Oversampling Setting
#define BME280_REGISTER_TEMP_OVERSAMPLING					0xF4
#define BME280_REGISTER_PRES_OVERSAMPLING					0xF4
#define BME280_REGISTER_TEMP_AND_PRES_OVERSAMPLING_AND_MODE	0xF4
#define BME280_REGISTER_HUM_OVERSAMPLING					0xF2
// Calibration Data Block 1 (Humidity)
#define BME280_HUMIDITY_CALIB_DIG_H6_REG                    0xE7
#define BME280_HUMIDITY_CALIB_DIG_H5_MSB_REG                0xE6
#define BME280_HUMIDITY_CALIB_DIG_H4_LSB_REG                0xE5
#define BME280_HUMIDITY_CALIB_DIG_H4_MSB_REG                0xE4
#define BME280_HUMIDITY_CALIB_DIG_H3_REG                    0xE3
#define BME280_HUMIDITY_CALIB_DIG_H2_MSB_REG                0xE2
#define BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG                0xE1
// Hardcoded Chip ID
#define BME280_REGISTER_CHIPID								0xD0
// Calibration Data Block 2 (Humidity, Pressure, Temperature)
#define BME280_HUMIDITY_CALIB_DIG_H1_REG                    0xA1
#define BME280_NOTHING_TO_SEE_HERE_REG						0xA0
#define BME280_PRESSURE_CALIB_DIG_P9_MSB_REG                0x9F
#define BME280_PRESSURE_CALIB_DIG_P9_LSB_REG                0x9E
#define BME280_PRESSURE_CALIB_DIG_P8_MSB_REG                0x9D
#define BME280_PRESSURE_CALIB_DIG_P8_LSB_REG                0x9C
#define BME280_PRESSURE_CALIB_DIG_P7_MSB_REG                0x9B
#define BME280_PRESSURE_CALIB_DIG_P7_LSB_REG                0x9A
#define BME280_PRESSURE_CALIB_DIG_P6_MSB_REG                0x99
#define BME280_PRESSURE_CALIB_DIG_P6_LSB_REG                0x98
#define BME280_PRESSURE_CALIB_DIG_P5_MSB_REG                0x97
#define BME280_PRESSURE_CALIB_DIG_P5_LSB_REG                0x96
#define BME280_PRESSURE_CALIB_DIG_P4_MSB_REG                0x95
#define BME280_PRESSURE_CALIB_DIG_P4_LSB_REG                0x94
#define BME280_PRESSURE_CALIB_DIG_P3_MSB_REG                0x93
#define BME280_PRESSURE_CALIB_DIG_P3_LSB_REG                0x92
#define BME280_PRESSURE_CALIB_DIG_P2_MSB_REG                0x91
#define BME280_PRESSURE_CALIB_DIG_P2_LSB_REG                0x90
#define BME280_PRESSURE_CALIB_DIG_P1_MSB_REG                0x8F
#define BME280_PRESSURE_CALIB_DIG_P1_LSB_REG                0x8E
#define BME280_TEMPERATURE_CALIB_DIG_T3_MSB_REG             0x8D
#define BME280_TEMPERATURE_CALIB_DIG_T3_LSB_REG             0x8C
#define BME280_TEMPERATURE_CALIB_DIG_T2_MSB_REG             0x8B
#define BME280_TEMPERATURE_CALIB_DIG_T2_LSB_REG             0x8A
#define BME280_TEMPERATURE_CALIB_DIG_T1_MSB_REG             0x89
#define BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG             0x88

// Sensor Array Indices
#define BME280_HUM_LSB										7
#define BME280_HUM_MSB										6
#define BME280_TEMP_XLSB									5
#define BME280_TEMP_LSB										4
#define BME280_TEMP_MSB										3
#define BME280_PRES_XLSB									2
#define BME280_PRES_LSB										1
#define BME280_PRES_MSB										0

// Calibration Array Indices
#define	BME280_TEMPERATURE_CALIB_DIG_T1_LSB					0
#define	BME280_TEMPERATURE_CALIB_DIG_T1_MSB					1
#define	BME280_TEMPERATURE_CALIB_DIG_T2_LSB					2
#define	BME280_TEMPERATURE_CALIB_DIG_T2_MSB					3
#define	BME280_TEMPERATURE_CALIB_DIG_T3_LSB					4
#define	BME280_TEMPERATURE_CALIB_DIG_T3_MSB					5
#define	BME280_PRESSURE_CALIB_DIG_P1_LSB       				6
#define	BME280_PRESSURE_CALIB_DIG_P1_MSB       				7
#define	BME280_PRESSURE_CALIB_DIG_P2_LSB       				8
#define	BME280_PRESSURE_CALIB_DIG_P2_MSB       				9
#define	BME280_PRESSURE_CALIB_DIG_P3_LSB       				10
#define	BME280_PRESSURE_CALIB_DIG_P3_MSB       				11
#define	BME280_PRESSURE_CALIB_DIG_P4_LSB       				12
#define	BME280_PRESSURE_CALIB_DIG_P4_MSB       				13
#define	BME280_PRESSURE_CALIB_DIG_P5_LSB       				14
#define	BME280_PRESSURE_CALIB_DIG_P5_MSB       				15
#define	BME280_PRESSURE_CALIB_DIG_P6_LSB       				16
#define	BME280_PRESSURE_CALIB_DIG_P6_MSB       				17
#define	BME280_PRESSURE_CALIB_DIG_P7_LSB       				18
#define	BME280_PRESSURE_CALIB_DIG_P7_MSB       				19
#define	BME280_PRESSURE_CALIB_DIG_P8_LSB       				20
#define	BME280_PRESSURE_CALIB_DIG_P8_MSB       				21
#define	BME280_PRESSURE_CALIB_DIG_P9_LSB       				22
#define	BME280_PRESSURE_CALIB_DIG_P9_MSB       				23
#define	BME280_HUMIDITY_CALIB_DIG_H1           				25
#define	BME280_HUMIDITY_CALIB_DIG_H2_LSB					0
#define	BME280_HUMIDITY_CALIB_DIG_H2_MSB					1
#define	BME280_HUMIDITY_CALIB_DIG_H3						2
#define	BME280_HUMIDITY_CALIB_DIG_H4_MSB					3
#define	BME280_HUMIDITY_CALIB_DIG_H4_LSB					4
#define	BME280_HUMIDITY_CALIB_DIG_H5_MSB					5
#define	BME280_HUMIDITY_CALIB_DIG_H6						6
#define	BME280_MASK_DIG_H4									0x0F

// Size constants
#define BME280_TEMP_DATA_SIZE			3
#define BME280_PRES_DATA_SIZE			3
#define BME280_HUM_DATA_SIZE			2
#define BME280_ALL_DATA_SIZE			8
#define BME280_MAX_CALIB_BLOCK_SIZE		26
#define BME280_LOWER_CALIB_BLOCK_SIZE	26
#define BME280_UPPER_CALIB_BLOCK_SIZE	7
#define BME280_CALIB_DATA_SIZE			37

#endif
