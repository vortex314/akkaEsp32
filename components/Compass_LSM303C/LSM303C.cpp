#include "LSM303C.h"

#include "stdint.h"

LSM303C::LSM303C(Connector* connector, ActorRef& publisher)
		: _publisher(publisher), _i2c(connector->getI2C()) {
	_uext = connector;
	_mag= {};
	_accel= {};
	_temp = 0;
}

LSM303C::~LSM303C() {
	delete _uext;
}
void LSM303C::preStart() {
	if (begin() != IMU_SUCCESS) WARN(" initialization LSM303C failed ! ");
	_measureTimer =
			timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 300);
}

void LSM303C::mqttPublish(Label name, float value) {
	Msg msg(Publisher::Publish);
	msg(name, value);
	_publisher.tell(msg, self());
}

Receive& LSM303C::createReceive() {
	return receiveBuilder()

	.match(MsgClass("measureTimer"), [this](Msg& msg) {
		_accel.x = readAccel(xAxis);
		_accel.y = readAccel(yAxis);
		_accel.z = readAccel(zAxis);
		_mag.x =readMag(xAxis);
		_mag.y =readMag(yAxis);
		_mag.z =readMag(zAxis);
		_temp = readTempC();
		mqttPublish("mag/X",_mag.x);
		mqttPublish("mag/Y",_mag.y);
		mqttPublish("mag/Z",_mag.z);
		mqttPublish("accel/X",_accel.x);
		mqttPublish("accel/Y",_accel.y);
		mqttPublish("accel/Z",_accel.z);
		mqttPublish("temp",_temp);
		calc();
//		INFO(" temp:%f magnetic x: %f y:%f z:%f accel x: %f y:%f z:%f",_temp,_mag.x,_mag.y,_mag.z,_accel.x,_accel.y,_accel.z);
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {sender().tell(replyBuilder(msg)
				("mag/X",_mag.x)
				("mag/Y",_mag.y)
				("mag/Z",_mag.z)
				("accel/X",_accel.x)
				("accel/Y",_accel.y)
				("accel/Z",_accel.z)
				("temp",_temp),self());}).build();
}

// Public methods
status_t LSM303C::begin() {
	return begin(
	// Initialize magnetometer output data rate to 0.625 Hz (turn on device)
	MAG_DO_40_Hz,
	// Initialize magnetic field full scale to +/-16 gauss
	MAG_FS_4_Ga,
	// Enabling block data updating
	MAG_BDU_ENABLE,
	// Initialize magnetometer X/Y axes ouput data rate to high-perf mode
	MAG_OMXY_HIGH_PERFORMANCE,
	// Initialize magnetometer Z axis performance mode
	MAG_OMZ_HIGH_PERFORMANCE,
	// Initialize magnetometer run mode. Also enables I2C (bit 7 = 0)
	MAG_MD_CONTINUOUS,
	// Initialize acceleration full scale to +/-2g
	ACC_FS_2g,
	// Enable block data updating
	ACC_BDU_ENABLE,
	// Enable X, Y, and Z accelerometer axes
	ACC_X_ENABLE | ACC_Y_ENABLE | ACC_Z_ENABLE,
	// Initialize accelerometer output data rate to 100 Hz (turn on device)
	ACC_ODR_100_Hz);
}

status_t LSM303C::begin(MAG_DO_t modr, MAG_FS_t mfs, MAG_BDU_t mbu,
		MAG_OMXY_t mxyodr, MAG_OMZ_t mzodr, MAG_MD_t mm, ACC_FS_t afs,
		ACC_BDU_t abu, uint8_t aea, ACC_ODR_t aodr) {
	uint8_t successes = 0;
	// Select I2C or SPI

	_i2c.setClock(400000);
	_i2c.init();

	uint8_t data;
	if (I2C_ByteRead(ACC_I2C_ADDR, ACC_WHO_AM_I, data) != IMU_SUCCESS
			|| data != 0x41) ERROR(" LSM303C not connected (ACC)");
	if (I2C_ByteRead(MAG_I2C_ADDR, MAG_WHO_AM_I, data) != IMU_SUCCESS
			|| data != 0x3D) ERROR(" LSM303C not connected (MAG)");;

	////////// Initialize Magnetometer //////////
	// Initialize magnetometer output data rate
	successes += MAG_SetODR(modr);
	// Initialize magnetic field full scale
	successes += MAG_SetFullScale(mfs);
	// Enabling block data updating
	successes += MAG_BlockDataUpdate(mbu);
	// Initialize magnetometer X/Y axes ouput data rate
	successes += MAG_XY_AxOperativeMode(mxyodr);
	// Initialize magnetometer Z axis performance mode
	successes += MAG_Z_AxOperativeMode(mzodr);
	// Initialize magnetometer run mode.
	successes += MAG_SetMode(mm);

	////////// Initialize Accelerometer //////////
	// Initialize acceleration full scale
	successes += ACC_SetFullScale(afs);
	// Enable block data updating
	successes += ACC_BlockDataUpdate(abu);
	// Enable X, Y, and Z accelerometer axes
	successes += ACC_EnableAxis(aea);
	// Initialize accelerometer output data rate
	successes += ACC_SetODR(aodr);

	return (successes == IMU_SUCCESS) ? IMU_SUCCESS : IMU_HW_ERROR;
}

float LSM303C::readMagX() {
	return readMag(xAxis);
}

float LSM303C::readMagY() {
	return readMag(yAxis);
}

float LSM303C::readMagZ() {
	return readMag(zAxis);
}

float LSM303C::readAccelX() {
	uint8_t flag_ACC_STATUS_FLAGS;
	status_t response = ACC_Status_Flags(flag_ACC_STATUS_FLAGS);

	if (response != IMU_SUCCESS) {
		WARN(AERROR);
		return NAN;
	}

	// Check for new data in the status flags with a mask
	// If there isn't new data use the last data read.
	// There are valid cases for this, like reading faster than refresh rate.
	if (flag_ACC_STATUS_FLAGS & ACC_X_NEW_DATA_AVAILABLE) {
		uint8_t valueL;
		uint8_t valueH;

		if (ACC_ReadReg(ACC_OUT_X_H, valueH)) {
			return IMU_HW_ERROR;
		}

		if (ACC_ReadReg(ACC_OUT_X_L, valueL)) {
			return IMU_HW_ERROR;
		}

		DEBUG("Fresh raw data");

		//convert from LSB to mg
		return int16_t(((valueH << 8) | valueL)) * SENSITIVITY_ACC;
	}

	// Should never get here
	WARN("Returning NAN");
	return NAN;
}

float LSM303C::readAccelY() {
	uint8_t flag_ACC_STATUS_FLAGS;
	status_t response = ACC_Status_Flags(flag_ACC_STATUS_FLAGS);

	if (response != IMU_SUCCESS) {
		WARN(AERROR);
		return NAN;
	}

	// Check for new data in the status flags with a mask
	// If there isn't new data use the last data read.
	// There are valid cases for this, like reading faster than refresh rate.
	if (flag_ACC_STATUS_FLAGS & ACC_Y_NEW_DATA_AVAILABLE) {
		uint8_t valueL;
		uint8_t valueH;

		if (ACC_ReadReg(ACC_OUT_Y_H, valueH)) {
			return IMU_HW_ERROR;
		}

		if (ACC_ReadReg(ACC_OUT_Y_L, valueL)) {
			return IMU_HW_ERROR;
		}

		DEBUG("Fresh raw data");

		//convert from LSB to mg
		return int16_t(((valueH << 8) | valueL)) * SENSITIVITY_ACC;
	}

	// Should never get here
	WARN("Returning NAN");
	return NAN;
}

float LSM303C::readAccelZ() {
	uint8_t flag_ACC_STATUS_FLAGS;
	status_t response = ACC_Status_Flags(flag_ACC_STATUS_FLAGS);

	if (response != IMU_SUCCESS) {
		WARN(AERROR);
		return NAN;
	}

	// Check for new data in the status flags with a mask
	// If there isn't new data use the last data read.
	// There are valid cases for this, like reading faster than refresh rate.
	if (flag_ACC_STATUS_FLAGS & ACC_Z_NEW_DATA_AVAILABLE) {
		uint8_t valueL;
		uint8_t valueH;

		if (ACC_ReadReg(ACC_OUT_Z_H, valueH)) {
			return IMU_HW_ERROR;
		}

		if (ACC_ReadReg(ACC_OUT_Z_L, valueL)) {
			return IMU_HW_ERROR;
		}

		DEBUG("Fresh raw data");

		//convert from LSB to mg
		return (int16_t(((valueH << 8) | valueL)) * SENSITIVITY_ACC);
	}

	// Should never get here
	WARN("Returning NAN");
	return NAN;
}

float LSM303C::readTempC() {
	uint8_t valueL;
	uint8_t valueH;
	float temperature;

	// Make sure temperature sensor is enabled
	if (MAG_TemperatureEN(MAG_TEMP_EN_ENABLE)) {
		return NAN;
	}

	if (MAG_ReadReg(MAG_TEMP_OUT_L, valueL)) {
		return NAN;
	}

	if (MAG_ReadReg(MAG_TEMP_OUT_H, valueH)) {
		return NAN;
	}

	temperature = (float) ((valueH << 8) | valueL);
	temperature /= 8; // 8 digits/˚C
	temperature += 25; // Reads 0 @ 25˚C

//	INFO("TEMP %2X %2X %f ",valueH,valueL,temperature);

	return temperature;
}

float LSM303C::readTempF() {
	return ((readTempC() * 9.0 / 5.0) + 32.0);
}

////////////////////////////////////////////////////////////////////////////////
////// Protected methods

float LSM303C::readAccel(AXIS_t dir) {
	uint8_t flag_ACC_STATUS_FLAGS;
	status_t response = ACC_Status_Flags(flag_ACC_STATUS_FLAGS);

	if (response != IMU_SUCCESS) {
		WARN(AERROR);
		return NAN;
	}

	// Check for new data in the status flags with a mask
	// If there isn't new data use the last data read.
	// There are valid cases for this, like reading faster than refresh rate.
	if (flag_ACC_STATUS_FLAGS & ACC_ZYX_NEW_DATA_AVAILABLE) {
		response = ACC_GetAccRaw(accel);
		// INFO("ACC raw %d %d %d",accel.xAxis,accel.yAxis,accel.zAxis);
	}
	//convert from LSB to mg
	switch (dir) {
		case xAxis:
			return accXrange.scale(accel.xAxis);
			return accel.xAxis * SENSITIVITY_ACC;
			break;
		case yAxis:
			return accYrange.scale(accel.yAxis);

			return accel.yAxis * SENSITIVITY_ACC;
			break;
		case zAxis:
			return accZrange.scale(accel.zAxis);

			return accel.zAxis * SENSITIVITY_ACC;
			break;
		default:
			return NAN;
	}

	// Should never get here
	WARN("Returning NAN");
	return NAN;
}

float LSM303C::readMag(AXIS_t dir) {
	MAG_XYZDA_t flag_MAG_XYZDA;
	status_t response = MAG_XYZ_AxDataAvailable(flag_MAG_XYZDA);

	if (response != IMU_SUCCESS) {
		WARN(MERROR);
		return NAN;
	}

	// Check for new data in the status flags with a mask
	if (flag_MAG_XYZDA & MAG_XYZDA_YES) {
		response = MAG_GetMagRaw(magData);
		DEBUG("Fresh raw data");
	}
	//convert from LSB to Gauss
	switch (dir) {
		case xAxis:
			return magXrange.scale(magData.xAxis);
			return magData.xAxis * SENSITIVITY_MAG;
			break;
		case yAxis:
			return magYrange.scale(magData.yAxis);
			return magData.yAxis * SENSITIVITY_MAG;
			break;
		case zAxis:
			return magZrange.scale(magData.zAxis);
			return magData.zAxis * SENSITIVITY_MAG;
			break;
		default:
			return NAN;
	}

	// Should never get here
	WARN("Returning NAN");
	return NAN;
}

status_t LSM303C::MAG_GetMagRaw(AxesRaw_t& buff) {
	uint8_t valueL;
	uint8_t valueH;

	if (MAG_ReadReg(MAG_OUTX_L, valueL)) {
		return IMU_HW_ERROR;
	}

	if (MAG_ReadReg(MAG_OUTX_H, valueH)) {
		return IMU_HW_ERROR;
	}

	buff.xAxis = (int16_t) ((valueH << 8) | valueL);

	if (MAG_ReadReg(MAG_OUTY_L, valueL)) {
		return IMU_HW_ERROR;
	}

	if (MAG_ReadReg(MAG_OUTY_H, valueH)) {
		return IMU_HW_ERROR;
	}

	buff.yAxis = (int16_t) ((valueH << 8) | valueL);

	if (MAG_ReadReg(MAG_OUTZ_L, valueL)) {
		return IMU_HW_ERROR;
	}

	if (MAG_ReadReg(MAG_OUTZ_H, valueH)) {
		return IMU_HW_ERROR;
	}

	buff.zAxis = (int16_t) ((valueH << 8) | valueL);

//	INFO("MAG raw %d %d %d ",buff.xAxis,buff.yAxis,buff.zAxis);

	return IMU_SUCCESS;
}

// Methods required to get device up and running
status_t LSM303C::MAG_SetODR(MAG_DO_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG1, value)) {
		WARN("Failed Read from MAG_CTRL_REG1");
		return IMU_HW_ERROR;
	}

	// Mask and only change DO0 bits (4:2) of MAG_CTRL_REG1
	value &= ~MAG_DO_80_Hz;
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_SetFullScale(MAG_FS_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG2, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~MAG_FS_16_Ga; //mask
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG2, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_BlockDataUpdate(MAG_BDU_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG5, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~MAG_BDU_ENABLE; //mask
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG5, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_XYZ_AxDataAvailable(MAG_XYZDA_t& value) {
	if (MAG_ReadReg(MAG_STATUS_REG, (uint8_t&) value)) {
		return IMU_HW_ERROR;
	}

	value = (MAG_XYZDA_t) ((int8_t) value & (int8_t) MAG_XYZDA_YES);

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_XY_AxOperativeMode(MAG_OMXY_t val) {

	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG1, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~MAG_OMXY_ULTRA_HIGH_PERFORMANCE; //mask
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_Z_AxOperativeMode(MAG_OMZ_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG4, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~MAG_OMZ_ULTRA_HIGH_PERFORMANCE; //mask
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG4, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_SetMode(MAG_MD_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG3, value)) {
		WARN("Failed to read MAG_CTRL_REG3. 'Read': 0x%X", value);
		return IMU_HW_ERROR;
	}

	value &= ~MAG_MD_POWER_DOWN_2;
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG3, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::ACC_SetFullScale(ACC_FS_t val) {
	uint8_t value;

	if (ACC_ReadReg(ACC_CTRL4, value)) {
		WARN("Failed ACC read");
		return IMU_HW_ERROR;
	}

	value &= ~ACC_FS_8g;
	value |= val;

	if (ACC_WriteReg(ACC_CTRL4, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::ACC_BlockDataUpdate(ACC_BDU_t val) {
	uint8_t value;

	if (ACC_ReadReg(ACC_CTRL1, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~ACC_BDU_ENABLE;
	value |= val;

	if (ACC_WriteReg(ACC_CTRL1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::ACC_EnableAxis(uint8_t val) {
	uint8_t value;

	if (ACC_ReadReg(ACC_CTRL1, value)) {
		WARN(AERROR);
		return IMU_HW_ERROR;
	}

	value &= ~0x07;
	value |= val;

	if (ACC_WriteReg(ACC_CTRL1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::ACC_SetODR(ACC_ODR_t val) {
	uint8_t value;

	if (ACC_ReadReg(ACC_CTRL1, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~ACC_ODR_MASK;
	value |= val;

	if (ACC_WriteReg(ACC_CTRL1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_TemperatureEN(MAG_TEMP_EN_t val) {
	uint8_t value;

	if (MAG_ReadReg(MAG_CTRL_REG1, value)) {
		return IMU_HW_ERROR;
	}

	value &= ~MAG_TEMP_EN_ENABLE; //mask
	value |= val;

	if (MAG_WriteReg(MAG_CTRL_REG1, value)) {
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::MAG_ReadReg(MAG_REG_t reg, uint8_t& data) {
	status_t ret = IMU_GENERIC_ERROR;

	ret = I2C_ByteRead(MAG_I2C_ADDR, reg, data);
	DEBUG("MAG read reg 0x%X : 0x%X", reg, data);

	return ret;
}

uint8_t LSM303C::MAG_WriteReg(MAG_REG_t reg, uint8_t data) {
	uint8_t ret;

	ret = I2C_ByteWrite(MAG_I2C_ADDR, reg, data);
	DEBUG("MAG write reg 0x%X : 0x%X", reg, data);

	return ret;
}

status_t LSM303C::ACC_ReadReg(ACC_REG_t reg, uint8_t& data) {
	status_t ret;
	ret = I2C_ByteRead(ACC_I2C_ADDR, reg, data);
	DEBUG("ACC Reading address 0x%X : 0x%X", reg, data);

	return ret;
}

uint8_t LSM303C::ACC_WriteReg(ACC_REG_t reg, uint8_t data) {
	uint8_t ret;
	ret = I2C_ByteWrite(ACC_I2C_ADDR, reg, data);
	DEBUG("ACC Writing address 0x%X : 0x%X", reg, data);

	return ret;
}

uint8_t LSM303C::I2C_ByteWrite(I2C_ADDR_t slaveAddress, uint8_t reg,
		uint8_t data) {
	_i2c.setSlaveAddress(slaveAddress);
	// returns num bytes written
	uint8_t msg[] = { reg, data };
	auto erc = _i2c.write(msg, 2);
	if (erc) return IMU_HW_ERROR;
	return IMU_SUCCESS;
}

status_t LSM303C::I2C_ByteRead(I2C_ADDR_t slaveAddress, uint8_t reg,
		uint8_t& data) {
	status_t ret = IMU_GENERIC_ERROR;
	DEBUG("Reading from I2C address: 0x%X, register 0x%X", slaveAddress, reg);
	_i2c.setSlaveAddress(slaveAddress); // Initialize the Tx buffer
	if (_i2c.write(reg) == E_OK)  // Put slave register address in Tx buff
	{
		if (_i2c.read(&data, 1) == E_OK) {
			DEBUG("Read: 0x%X", data);
			ret = IMU_SUCCESS;
		} else {
			WARN("IMU_HW_ERROR");
			ret = IMU_HW_ERROR;
		}
	} else {
		WARN("Error: couldn't send slave register address");
	}
	return ret;
}

status_t LSM303C::ACC_Status_Flags(uint8_t& val) {
	DEBUG("Getting accel status");
	if (ACC_ReadReg(ACC_STATUS, val)) {
		WARN(AERROR);
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

status_t LSM303C::ACC_GetAccRaw(AxesRaw_t& buff) {
	uint8_t valueL;
	uint8_t valueH;

	if (ACC_ReadReg(ACC_OUT_X_H, valueH)) {
		return IMU_HW_ERROR;
	}

	if (ACC_ReadReg(ACC_OUT_X_L, valueL)) {
		return IMU_HW_ERROR;
	}

	buff.xAxis = (int16_t) ((valueH << 8) | valueL);

	if (ACC_ReadReg(ACC_OUT_Y_H, valueH)) {
		return IMU_HW_ERROR;
	}

	if (ACC_ReadReg(ACC_OUT_Y_L, valueL)) {
		return IMU_HW_ERROR;
	}

	buff.yAxis = (int16_t) ((valueH << 8) | valueL);

	if (ACC_ReadReg(ACC_OUT_Z_H, valueH)) {
		return IMU_HW_ERROR;
	}

	if (ACC_ReadReg(ACC_OUT_Z_L, valueL)) {
		return IMU_HW_ERROR;
	}

	buff.zAxis = (int16_t) ((valueH << 8) | valueL);

	return IMU_SUCCESS;
}

/*
 *  This is a single sketch for tilt compensated compass using an LSM303DLHC.
 *  It does not use any libraries, other than wire, because I found a number of
 *  errors in libraries that are out there.The biggest problem is with libraries
 *  that rely on the application note for the lsm303.  That application note seems to
 *  include assumptions that are wrong, and I was not able to get it to produce
 *  reliable tilt compensated heading values.  If you look at how pitch, roll and heading
 *  are calculated below, you will see that the formula have some of the factors from
 *  the application note, but other factors are not included.
 *
 *  Somehow the matrix manupulations that are described in the application note
 *  introduce extra factors that are wrong.  For example, the pitch and roll calculation should
 *  not depend on whether you rotate pitch before roll, or vice versa. But if you
 *  go through the math using the application note, you will find that they give
 *  different formula
 *
 *  Same with Heading.  When I run this sketch on my Adafruit break-out board, I get very
 *  stable pitch, roll and heading, and the heading is very nicely tilt compensated, regardless
 *  of pitch or roll.
 */

void LSM303C::calc() {

	int Ax, Ay, Az, Mx, My, Mz;

	float Xm_print, Ym_print, Zm_print;
	float Xm_off, Ym_off, Zm_off, Xm_cal, Ym_cal, Zm_cal, Norm_m;

	float Xa_print, Ya_print, Za_print;
	float Xa_off, Ya_off, Za_off, Xa_cal, Ya_cal, Za_cal, Norm_a;

	const float alpha = 0.15;
	float fXa = 0;
	float fYa = 0;
	float fZa = 0;
	float fXm = 0;
	float fYm = 0;
	float fZm = 0;
	float pitch, pitch_print, roll, roll_print, Heading;
	float fXm_comp, fYm_comp;

	Ax = accel.xAxis;
	Ay = accel.yAxis;
	Az = accel.zAxis;

	Xa_off = Ax / 16.0 + 14.510699; // add/subtract bias calculated by Magneto 1.2. Search the web and you will
	Ya_off = Ay / 16.0 - 17.648453; // find this Windows application.  It works very well to find calibrations
	Za_off = Az / 16.0 - 6.134981;
	Xa_cal = 1.006480 * Xa_off - 0.012172 * Ya_off + 0.002273 * Za_off; // apply scale factors calculated by Magneto1.2
	Ya_cal = -0.012172 * Xa_off + 0.963586 * Ya_off - 0.006436 * Za_off;
	Za_cal = 0.002273 * Xa_off - 0.006436 * Ya_off + 0.965482 * Za_off;
	Norm_a = sqrt(Xa_cal * Xa_cal + Ya_cal * Ya_cal + Za_cal * Za_cal); //original code did not appear to normalize, and this seems to help
	Xa_cal = Xa_cal / Norm_a;
	Ya_cal = Ya_cal / Norm_a;
	Za_cal = Za_cal / Norm_a;

	Ya_cal = -1.0 * Ya_cal; // This sign inversion is needed because the chip has +Z up, while algorithms assume +Z down
	Za_cal = -1.0 * Za_cal; // This sign inversion is needed for the same reason and to preserve right hand rotation system

// Low-Pass filter accelerometer
	fXa = Xa_cal * alpha + (fXa * (1.0 - alpha));
	fYa = Ya_cal * alpha + (fYa * (1.0 - alpha));
	fZa = Za_cal * alpha + (fZa * (1.0 - alpha));

	Mx = magData.xAxis;
	Mz = magData.zAxis;
	My = magData.yAxis;

	Xm_off = Mx * (100000.0 / 1100.0) - 617.106577; // Gain X [LSB/Gauss] for selected sensor input field range (1.3 in these case)
	Ym_off = My * (100000.0 / 1100.0) - 3724.617984; // Gain Y [LSB/Gauss] for selected sensor input field range
	Zm_off = Mz * (100000.0 / 980.0) - 16432.772031; // Gain Z [LSB/Gauss] for selected sensor input field range
	Xm_cal = 0.982945 * Xm_off + 0.012083 * Ym_off + 0.014055 * Zm_off; // same calibration program used for mag as accel.
	Ym_cal = 0.012083 * Xm_off + 0.964757 * Ym_off - 0.001436 * Zm_off;
	Zm_cal = 0.014055 * Xm_off - 0.001436 * Ym_off + 0.952889 * Zm_off;
	Norm_m = sqrt(Xm_cal * Xm_cal + Ym_cal * Ym_cal + Zm_cal * Zm_cal); // original code did not appear to normalize  This seems to help
	Xm_cal = Xm_cal / Norm_m;
	Ym_cal = Ym_cal / Norm_m;
	Zm_cal = Zm_cal / Norm_m;

	Ym_cal = -1.0 * Ym_cal; // This sign inversion is needed because the chip has +Z up, while algorithms assume +Z down
	Zm_cal = -1.0 * Zm_cal; // This sign inversion is needed for the same reason and to preserve right hand rotation system

// Low-Pass filter magnetometer
	fXm = Xm_cal * alpha + (fXm * (1.0 - alpha));
	fYm = Ym_cal * alpha + (fYm * (1.0 - alpha));
	fZm = Zm_cal * alpha + (fZm * (1.0 - alpha));

// Pitch and roll
	pitch = asin(fXa);
	roll = -asin(fYa);
	pitch_print = pitch * 180.0 / M_PI;
	roll_print = roll * 180.0 / M_PI;

// Tilt compensated magnetic sensor measurements
	fXm_comp = fXm * cos(pitch) + fZm * sin(pitch);
	fYm_comp = fYm * cos(roll) - fZm * sin(roll);

	Heading = (atan2(-fYm_comp, fXm_comp) * 180.0) / M_PI;

	if (Heading < 0) Heading += 360;

	INFO(" Heading : %f Pitch (X): %f Roll (Y): %f ", Heading, pitch_print, roll_print);

}
