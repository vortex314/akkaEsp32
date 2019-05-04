/*
 * Mpu6050.cpp
 *
 *  Created on: May 4, 2019
 *      Author: lieven
 */

#include "MPU6050.h"


#define ORIGINAL_OUTPUT			 (0)
#define ACC_FULLSCALE        	 (2)
#define GYRO_FULLSCALE			 (250)

#if ORIGINAL_OUTPUT == 0
	#if  ACC_FULLSCALE  == 2
		#define AccAxis_Sensitive (float)(16384)
	#elif ACC_FULLSCALE == 4
		#define AccAxis_Sensitive (float)(8192)
	#elif ACC_FULLSCALE == 8
		#define AccAxis_Sensitive (float)(4096)
	#elif ACC_FULLSCALE == 16
		#define AccAxis_Sensitive (float)(2048)
	#endif

	#if   GYRO_FULLSCALE == 250
		#define GyroAxis_Sensitive (float)(131.0)
	#elif GYRO_FULLSCALE == 500
		#define GyroAxis_Sensitive (float)(65.5)
	#elif GYRO_FULLSCALE == 1000
		#define GyroAxis_Sensitive (float)(32.8)
	#elif GYRO_FULLSCALE == 2000
		#define GyroAxis_Sensitive (float)(16.4)
	#endif

#else
	#define AccAxis_Sensitive  (1)
	#define GyroAxis_Sensitive (1)
#endif

MPU6050::MPU6050(I2C& i2c):_i2c(i2c) {
}

MPU6050::~MPU6050() {
}

bool MPU6050::init() {
	_i2c.setSlaveAddress(MPU6050_ADDR);
	uint8_t data[]={PWR_MGMT_1,0x00};
	_i2c.write(data,2);
	uint8_t data2[]= {SMPLRT_DIV  , 0x07};
	_i2c.write(data2,2);
	uint8_t data3[] = {CONFIG      , 0x07};
	_i2c.write(data3,2);
	uint8_t data4[] = {GYRO_CONFIG , 0x18};
	_i2c.write(data4,2);
	uint8_t data5[] = {ACCEL_CONFIG, 0x01};
	_i2c.write(data5,2);
    return true;
}

void MPU6050::getRaw(uint8_t* data){
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(ACCEL_XOUT_H);
    _i2c.read(data,6);
}

float MPU6050::getAccX() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(ACCEL_XOUT_H);
    _i2c.read(r,2);
    short accx = r[0] << 8 | r[1];
    return (float)accx / AccAxis_Sensitive;
}

float MPU6050::getAccY() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(ACCEL_YOUT_H);
    _i2c.read(r,2);
    short accy = r[0] << 8 | r[1];
    return (float)accy / AccAxis_Sensitive;
}

float MPU6050::getAccZ() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(ACCEL_ZOUT_H);
    _i2c.read(r,2);
    short accz = r[0] << 8 | r[1];
    return (float)accz / AccAxis_Sensitive;
}

float MPU6050::getGyroX() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(GYRO_XOUT_H);
    _i2c.read(r,2);
    short gyrox = r[0] << 8 | r[1];
    return (float)gyrox / GyroAxis_Sensitive;
}

float MPU6050::getGyroY() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(GYRO_YOUT_H);
    _i2c.read(r,2);
    short gyroy = r[0] << 8 | r[1];
    return (float)gyroy / GyroAxis_Sensitive;
}

float MPU6050::getGyroZ() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(GYRO_ZOUT_H);
    _i2c.read(r,2);
    short gyroz = r[0] << 8 | r[1];
    return (float)gyroz / GyroAxis_Sensitive;
}

short MPU6050::getTemp() {
    uint8_t r[2];
    _i2c.setSlaveAddress(MPU6050_ADDR);
    _i2c.write(TEMP_OUT_H);
    _i2c.read(r,2);
    return r[0] << 8 | r[1];
}
