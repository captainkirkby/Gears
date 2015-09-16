// Header file for using the BME280 integrated environmental sensor
// Written by Dylan Kirkby 
// 8/27/15

#ifndef BME280_H
#define BME280_H

#include "BMEConstants.h"

// Function prototypes
// "Private" functions
uint8_t bme280UncompensatedTempPresHum(int32_t *temp, int32_t *pres, int32_t *hum);
int32_t bme280CompensatedTemperature(int32_t uncomp_temp);
int16_t bme280CompensatedTemperature16BitOutput(int32_t uncomp_temp);
uint32_t bme280CompensatedPressure(int32_t uncomp_hum);
uint32_t bme280CompensatedHumidity(int32_t uncomp_hum);
uint8_t readBME280CalibrationParameters();
uint8_t readBME280Register(uint8_t regAddress, uint8_t *data, size_t len);
uint8_t writeBME280Register(uint8_t regAddress, uint8_t data);
// "Public" functions
uint8_t initBME280(void);
uint8_t startBME280Conversion16XOversampling();
uint8_t readoutFinishedBME280Conversion(int16_t *temp, uint32_t *pres, uint32_t *hum);

// The BME280 calibration data format
typedef struct {
	// Temperature Calibration (6 Bytes)
	uint16_t	dig_T1;
	int16_t		dig_T2;
	int16_t		dig_T3;
	// Pressure Calibration (18 Bytes)
	uint16_t	dig_P1;
	int16_t		dig_P2;
	int16_t		dig_P3;
	int16_t		dig_P4;
	int16_t		dig_P5;
	int16_t		dig_P6;
	int16_t		dig_P7;
	int16_t		dig_P8;
	int16_t		dig_P9;
	// Humidity Calibration (9 Bytes)
	uint8_t		dig_H1;
	int16_t		dig_H2;
	uint8_t		dig_H3;
	int16_t		dig_H4;
	int16_t		dig_H5;
	int8_t		dig_H6;
	// Carries fine temperature globally (4 bytes)
	int32_t		t_fine;
} BME280CalibrationData;
static BME280CalibrationData cal;

// The BME280 command request format
typedef struct {
	uint8_t	regAddress;
	uint8_t command;
} BMP280CommandRequest;

// Initializes communications with the BMP sensor. The possible returned error codes are:
// 1X = readback of TWI chip ID failed, probably indicating general TWI problem
// 20 = got unexpected chip ID
// 3X = error reading calibration data
uint8_t initBME280(void)
{
	uint8_t error =	0;
	uint8_t id = 0xFE;
	// Read chip ID
	error = readBME280Register(BME280_REGISTER_CHIPID, &id, sizeof(id));
	if(error) return 10 + error;
	// Check that we got the expected chip ID
	if(id != BME280_CHIPID) return 20;
	// Get calibration data
	readBME280CalibrationParameters();
	return 0;
}

// Starts the BME280 doing a 16X oversampled conversion of
// Temperature, pressure, and humidity.
// Note: after 112.8 ms (see Appendix 9.1) the sensor registers
//    are ready to be read out and compensated
// 
// This function is meant to be called in conjunction with the function
// getFinishedBME280Readout().
// Example:
// 		startBME280Readout16XOversampling();
// 		_delay_ms(113);
//		getFinishedBME280Readout(&t,&p,&h);
uint8_t startBME280Readout16XOversampling()
{
	uint8_t error = 0;

	// Set IIR Filter to off
	error = writeBME280Register(BME280_REGISTER_FILTER, 0b00000000);
	if(error) return error;

	// Set Oversampling to 1x (Humidity)
	error = writeBME280Register(BME280_REGISTER_HUM_OVERSAMPLING, 0b00000101);
	if(error) return error;
	// Set Oversampling to 1x (Temperature)
	// Set Oversampling to 1x (Pressure)
	// Set Mode to Forced to start conversion
	error = writeBME280Register(BME280_REGISTER_TEMP_AND_PRES_OVERSAMPLING_AND_MODE, 0b10110101);
	if(error) return error;

	return error;
}

// Reads out and compensates the temperature, pressure, and humidity
// registers on the BME280.
//
// This function is meant to be called in conjunction with the function
// getFinishedBME280Readout().
// Example:
// 		startBME280Readout16XOversampling();
// 		_delay_ms(113);
//		getFinishedBME280Readout(&t,&p,&h);
uint8_t getFinishedBME280Readout(int16_t *temp, uint32_t *pres, uint32_t *hum)
{
	uint8_t error = 0;
	uint32_t uncomp_temp;
	uint32_t uncomp_pres;
	uint32_t uncomp_hum;
	uint8_t buffer[BME280_ALL_DATA_SIZE];
	// Read temperature, pressure, humidity
	// Array holding the MSB and LSB values
	// buffer[0] - Pressure MSB
	// buffer[1] - Pressure LSB
	// buffer[2] - Pressure XLSB
	// buffer[3] - Temperature MSB
	// buffer[4] - Temperature LSB
	// buffer[5] - Temperature XLSB
	// buffer[6] - Humidity MSB
	// buffer[7] - Humidity LSB

	// Read into buffer
	error = readBME280Register(BME280_REGISTER_PRES_MSB,buffer,BME280_ALL_DATA_SIZE);
	if(error) return error;

	// Construct the signed 32 bit int pressure from buffer
	uncomp_pres = (int32_t) 
			((((uint32_t) buffer[BME280_PRES_MSB]) << 12) |
			(((uint32_t) buffer[BME280_PRES_LSB]) << 4) |
			(((uint32_t) buffer[BME280_PRES_XLSB]) >> 4));

	// Construct the signed 32 bit int temperature from buffer
	uncomp_temp = (int32_t) 
			((((uint32_t) buffer[BME280_TEMP_MSB]) << 12) |
			(((uint32_t) buffer[BME280_TEMP_LSB]) << 4) |
			(((uint32_t) buffer[BME280_TEMP_XLSB]) >> 4));

	// Construct the signed 32 bit int humidity from buffer
	uncomp_hum = (int32_t) 
			((((uint32_t) buffer[BME280_HUM_MSB]) << 8) |
			((uint32_t) buffer[BME280_HUM_LSB]));


	*temp = bme280CompensatedTemperature16BitOutput(uncomp_temp);
	*pres = bme280CompensatedPressure(uncomp_pres);
	*hum  = bme280CompensatedHumidity(uncomp_hum);

	return error;
}

// Reads out the temperature registers on the chip.  The actual value
// doesn't have any meaning without the calibration data inputted to
// the compensation equations.  Don't call this function if you want
// the temperature!
uint8_t bme280UncompensatedTempPresHum(int32_t *uncomp_temp, int32_t *uncomp_pres, int32_t *uncomp_hum)
{
	uint8_t error = 0;

	// Set IIR Filter to off
	error = writeBME280Register(BME280_REGISTER_FILTER, 0b00000000);
	if(error) return error;

	// Set Oversampling to 1x (Humidity)
	error = writeBME280Register(BME280_REGISTER_HUM_OVERSAMPLING, 0b00000101);
	if(error) return error;
	// Set Oversampling to 1x (Temperature)
	// Set Oversampling to 1x (Pressure)
	// Set Mode to Forced to start conversion
	error = writeBME280Register(BME280_REGISTER_TEMP_AND_PRES_OVERSAMPLING_AND_MODE, 0b10110101);
	if(error) return error;

	// Wait 112.8 ms (Depends on Appendix 9.1)
	_delay_ms(113);

	// Read temperature
	// Array holding the MSB and LSB values
	// buffer[0] - Pressure MSB
	// buffer[1] - Pressure LSB
	// buffer[2] - Pressure XLSB
	// buffer[3] - Temperature MSB
	// buffer[4] - Temperature LSB
	// buffer[5] - Temperature XLSB
	// buffer[6] - Humidity MSB
	// buffer[7] - Humidity LSB
	uint8_t buffer[BME280_ALL_DATA_SIZE];

	// Read into buffer
	error = readBME280Register(BME280_REGISTER_PRES_MSB,buffer,BME280_ALL_DATA_SIZE);
	if(error) return error;

	// Construct the signed 32 bit int pressure from buffer
	*uncomp_pres = (int32_t) 
			((((uint32_t) buffer[BME280_PRES_MSB]) << 12) |
			(((uint32_t) buffer[BME280_PRES_LSB]) << 4) |
			(((uint32_t) buffer[BME280_PRES_XLSB]) >> 4));

	// Construct the signed 32 bit int temperature from buffer
	*uncomp_temp = (int32_t) 
			((((uint32_t) buffer[BME280_TEMP_MSB]) << 12) |
			(((uint32_t) buffer[BME280_TEMP_LSB]) << 4) |
			(((uint32_t) buffer[BME280_TEMP_XLSB]) >> 4));

	// Construct the signed 32 bit int humidity from buffer
	*uncomp_hum = (int32_t) 
			((((uint32_t) buffer[BME280_HUM_MSB]) << 8) |
			((uint32_t) buffer[BME280_HUM_LSB]));

	return error;
}

// Gets the compensated temperature as an absolute 
// 32 bit number in 0.01 degree C per LSB
// Example: output value of "5123" equals 51.23 DegC.
// Note: this is not the highest precision temperature!
int32_t bme280CompensatedTemperature(int32_t uncomp_temp)
{
	int32_t x1 = 0;
	int32_t x2 = 0;
	int32_t temperature = 0;

	// Calculate x1
	x1  = ((((uncomp_temp >> 3) -
	((int32_t)cal.dig_T1 << 1))) *
	((int32_t)cal.dig_T2)) >> 11;

	// Calculate x2
	x2  = (((((uncomp_temp >> 4) -
	((int32_t)cal.dig_T1)) * 
	((uncomp_temp >> 4) -
	((int32_t)cal.dig_T1))) >> 12) *
	((int32_t)cal.dig_T3)) >> 14;
	// Calculate t_fine
	cal.t_fine = x1 + x2;
	// Calculate temperature
	temperature  = (cal.t_fine * 5 + 128) >> 8;
	return temperature;
}

// Gets the compensated temperature as an absolute 
// 16 bit number in 0.002 degree C per LSB centered at 24DegC
// Example: output value of "5123" equals (5123/500)+24 = 34.246DegC
// Note: highest precision temperature is here!
int16_t bme280CompensatedTemperature16BitOutput(int32_t uncomp_temp)
{
	int16_t temperature = 0;
	// Get t_fine
	bme280CompensatedTemperature(uncomp_temp);
	temperature = (int16_t)((((cal.t_fine - 122880) * 25) + 128) >> 8);
	return temperature;
}

// Gets the absolute compensated pressure in Pascals from uncompensated pressure
// Example: output value of "96386" equals 96386 Pa = 963.86 millibar
uint32_t bme280CompensatedPressure(int32_t uncomp_pres)
{
	int32_t x1 = 0;
	int32_t x2 = 0;
	uint32_t pressure = 0;

	// Calculate x1
	x1 = (((int32_t)cal.t_fine)
	>> 1) - (int32_t)64000;
	// Calculate x2
	x2 = (((x1 >> 2)
	* (x1 >> 2)
	) >> 11)
	* ((int32_t)cal.dig_P6);
	// Calculate x2
	x2 = x2 + ((x1 *
	((int32_t)cal.dig_P5))
	<< 1);
	// Calculate x2
	x2 = (x2 >> 2) +
	(((int32_t)cal.dig_P4)
	<< 16);
	// Calculate x1
	x1 = (((cal.dig_P3 *
	(((x1 >> 2) *
	(x1 >> 2))
	>> 13))
	>> 3) +
	((((int32_t)cal.dig_P2) *
	x1) >> 1))
	>> 18;
	// Calculate x1
	x1 = ((((32768 + x1)) *
	((int32_t)cal.dig_P1))
	>> 15);
	// Calculate Pressure
	pressure =
	(((uint32_t)(((int32_t)1048576) - uncomp_pres)
	- (x2 >> 12))) * 3125;
	if (pressure
	< 0x80000000)
		// Avoid undefined behavior caused by dividing by zero
		if (x1 != 0)
			pressure =
			(pressure
			<< 1) /
			((uint32_t)x1);
		else
			return 0xFFFFFFFF;
	else
		// Avoid undefined behavior caused by dividing by zero
		if (x1 != 0)
			pressure = (pressure
			/ (uint32_t)x1) * 2;
		else
			return 0xFFFFFFFF;

	// Final recursive compensation
	x1 = (((int32_t)cal.dig_P9) *
	((int32_t)(((pressure >> 3)
	* (pressure >> 3))
	>> 13)))
	>> 12;
	x2 = (((int32_t)(pressure
	>> 2)) *
	((int32_t)cal.dig_P8))
	>> 13;
	pressure = (uint32_t)((int32_t)pressure +
	((x1 + x2 + cal.dig_P7)
	>> 4));

	return pressure;
}

// Gets the absolute humidity from uncompensated humidity and
// returns the % relative huidity as an unsigned 32 bit integer
// Example: output value "42313" equals 42313 / 1024 = 41.321 %rH
uint32_t bme280CompensatedHumidity(int32_t uncomp_hum)
{
	int32_t x1 = 0;

	// Calculate x1
	x1 = (cal.t_fine - ((int32_t)76800));
	// Calculate x1
	x1 = (((((uncomp_hum
	<< 14) -
	(((int32_t)cal.dig_H4)
	<< 20) -
	(((int32_t)cal.dig_H5) * x1)) +
	((int32_t)16384)) >> 15)
	* (((((((x1 *
	((int32_t)cal.dig_H6))
	>> 10) *
	(((x1 * ((int32_t)cal.dig_H3))
	>> 11) + ((int32_t)32768)))
	>> 10) + ((int32_t)2097152)) *
	((int32_t)cal.dig_H2) + 8192) >> 14));
	x1 = (x1 - (((((x1
	>> 15) *
	(x1 >> 15))
	>> 7) *
	((int32_t)cal.dig_H1))
	>> 4));
	x1 = (x1 < 0 ? 0 : x1);
	x1 = (x1 > 419430400 ?
	419430400 : x1);
	return (uint32_t)(x1 >> 12);
}

// Get the calibration parameters and store it in the
// static cal struct.
uint8_t readBME280CalibrationParameters()
{
	uint8_t data[BME280_MAX_CALIB_BLOCK_SIZE];
	uint8_t error;
	error = readBME280Register(BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG, data, BME280_LOWER_CALIB_BLOCK_SIZE);
	if(error) return error;
	
	cal.dig_T1 = (uint16_t)(((
	(uint16_t)((uint8_t)data[
	BME280_TEMPERATURE_CALIB_DIG_T1_MSB])) << 8)
	| data[BME280_TEMPERATURE_CALIB_DIG_T1_LSB]);

	cal.dig_T2 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_TEMPERATURE_CALIB_DIG_T2_MSB])) << 8)
	| data[BME280_TEMPERATURE_CALIB_DIG_T2_LSB]);

	cal.dig_T3 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_TEMPERATURE_CALIB_DIG_T3_MSB])) << 8)
	| data[BME280_TEMPERATURE_CALIB_DIG_T3_LSB]);

	cal.dig_P1 = (uint16_t)(((
	(uint16_t)((uint8_t)data[
	BME280_PRESSURE_CALIB_DIG_P1_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P1_LSB]);

	cal.dig_P2 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P2_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P2_LSB]);

	cal.dig_P3 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P3_MSB])) << 8)
	| data[
	BME280_PRESSURE_CALIB_DIG_P3_LSB]);

	cal.dig_P4 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P4_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P4_LSB]);

	cal.dig_P5 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P5_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P5_LSB]);

	cal.dig_P6 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P6_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P6_LSB]);

	cal.dig_P7 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P7_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P7_LSB]);

	cal.dig_P8 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P8_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P8_LSB]);

	cal.dig_P9 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_PRESSURE_CALIB_DIG_P9_MSB])) << 8)
	| data[BME280_PRESSURE_CALIB_DIG_P9_LSB]);

	cal.dig_H1 =
	data[BME280_HUMIDITY_CALIB_DIG_H1];

	error = readBME280Register(BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG, data, BME280_UPPER_CALIB_BLOCK_SIZE);
	if(error) return error;

	cal.dig_H2 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_HUMIDITY_CALIB_DIG_H2_MSB])) << 8)
	| data[BME280_HUMIDITY_CALIB_DIG_H2_LSB]);

	cal.dig_H3 =
	data[BME280_HUMIDITY_CALIB_DIG_H3];

	cal.dig_H4 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_HUMIDITY_CALIB_DIG_H4_MSB])) << 4) |
	(((uint8_t)BME280_MASK_DIG_H4) &
	data[BME280_HUMIDITY_CALIB_DIG_H4_LSB]));

	cal.dig_H5 = (int16_t)(((
	(int16_t)((int8_t)data[
	BME280_HUMIDITY_CALIB_DIG_H5_MSB])) << 4) |
	(data[BME280_HUMIDITY_CALIB_DIG_H4_LSB] >> 4));

	cal.dig_H6 =
	(int8_t)data[BME280_HUMIDITY_CALIB_DIG_H6];

	return 0;
}

// Synchronously read a register from the BME 280.  First write the
// address of the register you want to read, then read the byte(s)
// starting at the given register address
uint8_t readBME280Register(uint8_t regAddress, uint8_t *data, size_t len)
{
	uint8_t error = 0;
	// Write address of the desired register
	error = twiWrite(BME280_ADDRESS << 1, &regAddress, sizeof(regAddress));
	if(error) return error;
	error = twiRead(BME280_ADDRESS << 1, data, len);
	if(error) return error;
	return 0;
}

// Synchronously write a register to the BME 280.  First write the
// address of the register you want to write, then write the value
uint8_t writeBME280Register(uint8_t regAddress, uint8_t data)
{
	uint8_t error = 0;
	BMP280CommandRequest cmd;
	cmd.regAddress = regAddress;
	cmd.command = data;
	// Write address of the desired register
	error = twiWrite(BME280_ADDRESS << 1, (uint8_t*)&cmd, sizeof(cmd));
	if(error) return error;
	return 0;
}

#endif
