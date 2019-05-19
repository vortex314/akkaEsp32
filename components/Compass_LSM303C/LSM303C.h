// Test derrived class for base class SparkFunIMU
#ifndef __SPARKFUN_LSM303C_H__
#define __SPARKFUN_LSM303C_H__

#include <Log.h>
#include <Hardware.h>
#include <Akka.h>
#include <Publisher.h>

#include "../Compass_LSM303C/IMU.h"
#include "../Compass_LSM303C/LSM303CTypes.h"


#define SENSITIVITY_ACC   0.06103515625   // LSB/mg
#define SENSITIVITY_MAG   0.00048828125   // LSB/Ga

// Define a few error messages to save on space
static const char AERROR[] = "Accel Error";
static const char MERROR[] = "Mag Error";

// Define SPI pins (Pro Mini)
//  D10 -> SDI/SDO
//  D11 -> SCLK
//  D12 -> CS_XL
//  D13 -> CS_MAG
#define CSPORT_MAG PORTB
#define CSBIT_MAG  5
#define CSPORT_XL  PORTB
#define CSBIT_XL   4
#define CLKPORT    PORTB
#define CLKBIT     3
#define DATAPORTI  PINB
#define DATAPORTO  PORTB
#define DATABIT    2
#define DIR_REG    DDRB
// End SPI pin definitions

typedef struct {
	float x,y,z;
} Vector3D;

template <typename T> class Range {
		T _min;
		T _max;

	public:
		Range() {
			_min=INT16_MAX;
			_max=INT16_MIN;
		}
		void learn(T v ) {
			if ( v < _min ) _min=v;
			if ( v> _max) _max=v;
		}
		float scale(T v) {
			if ( v < _min ) _min=v;
			if ( v> _max) _max=v;
			if ( _max == _min ) return 0;
			return ((2.0*(v-_min))/(_max-_min))-1.0;
		}
};

class LSM303C : public Actor {
	public:
		// These are the only methods are the only methods the user can use w/o mods
		LSM303C(Connector* connector,ActorRef& publisher);
		virtual ~LSM303C() ;
		status_t begin(void);
		// Begin contains hardware specific code (Pro Mini)
		status_t begin(MAG_DO_t, MAG_FS_t, MAG_BDU_t, MAG_OMXY_t, MAG_OMZ_t,
		               MAG_MD_t, ACC_FS_t, ACC_BDU_t, uint8_t, ACC_ODR_t);
		float readAccelX(void);
		float readAccelY(void);
		float readAccelZ(void);
		float readMagX(void);
		float readMagY(void);
		float readMagZ(void);
		float readTempC(void);
		float readTempF(void);
		void calc();
		void preStart();
		Receive& createReceive();
		void mqttPublish(Label topic,float value);

	protected:
		Connector* _uext;
		MsgClass _measureTimer;
		ActorRef& _publisher;
		I2C& _i2c;

		float _temp;
		Vector3D _mag;
		Vector3D _accel;




		// Variables to store the most recently read raw data from sensor
		AxesRaw_t accData = { 0, 0, 0 };
		AxesRaw_t magData = { 0, 0, 0 };
		Range<int16_t> magXrange;
		Range<int16_t> magYrange;
		Range<int16_t> magZrange;
		Range<int16_t> accXrange;
		Range<int16_t> accYrange;
		Range<int16_t> accZrange;


		// The LSM303C functions over both I2C or SPI. This library supports both.
		// Interface mode used must be set!
		InterfaceMode_t interfaceMode = MODE_I2C;  // Set a default...

		// Hardware abstraction functions (Pro Mini)
		uint8_t I2C_ByteWrite(I2C_ADDR_t, uint8_t, uint8_t);
		status_t I2C_ByteRead(I2C_ADDR_t, uint8_t, uint8_t&);

		// Methods required to get device up and running
		status_t MAG_SetODR(MAG_DO_t);
		status_t MAG_SetFullScale(MAG_FS_t);
		status_t MAG_BlockDataUpdate(MAG_BDU_t);
		status_t MAG_XY_AxOperativeMode(MAG_OMXY_t);
		status_t MAG_Z_AxOperativeMode(MAG_OMZ_t);
		status_t MAG_SetMode(MAG_MD_t);
		status_t ACC_SetFullScale(ACC_FS_t);
		status_t ACC_BlockDataUpdate(ACC_BDU_t);
		status_t ACC_EnableAxis(uint8_t);
		status_t ACC_SetODR(ACC_ODR_t);

		status_t ACC_Status_Flags(uint8_t&);
		status_t ACC_GetAccRaw(AxesRaw_t&);
		float readAccel(AXIS_t); // Reads the accelerometer data from IC

		status_t MAG_GetMagRaw(AxesRaw_t&);
		status_t MAG_TemperatureEN(MAG_TEMP_EN_t);
		status_t MAG_XYZ_AxDataAvailable(MAG_XYZDA_t&);
		float readMag(AXIS_t);   // Reads the magnetometer data from IC

		status_t MAG_ReadReg(MAG_REG_t, uint8_t&);
		uint8_t MAG_WriteReg(MAG_REG_t, uint8_t);
		status_t ACC_ReadReg(ACC_REG_t, uint8_t&);
		uint8_t ACC_WriteReg(ACC_REG_t, uint8_t);
};

#endif
