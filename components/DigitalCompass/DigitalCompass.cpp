/*
 * DigitalDigitalCompass.cpp
 *
 *  Created on: May 4, 2019
 *      Author: lieven
 */

#include "DigitalCompass.h"

DigitalCompass::DigitalCompass(Connector* connector, ActorRef& publisher) :
		_connector(connector), _publisher(publisher), _i2c(
				_connector->getI2C()), pfilter(0.005), rfilter(0.005) {
	_hmc = new HMC5883L(_i2c);
	_mpu = new MPU6050(_i2c);
	ax = ay = az = gx = gy = gz = 0.0;
	pitch = roll = fpitch = froll = 0.0;
}
;

DigitalCompass::~DigitalCompass() {
	delete _hmc;
	delete _mpu;
	delete _connector;
}
void DigitalCompass::preStart() {
	_i2c.setClock(400000);
	_i2c.init();
	if (!_mpu->init())
		ERROR(" MPU6050 init failed");
	if (!_hmc->init())
		ERROR("HMC5883L init failed");

	_hmc->setRange(HMC5883L_RANGE_1_3GA);

	// Set measurement mode
	_hmc->setMeasurementMode(HMC5883L_CONTINOUS);

	// Set data rate
	_hmc->setDataRate(HMC5883L_DATARATE_30HZ);

	// Set number of samples averaged
	_hmc->setSamples(HMC5883L_SAMPLES_8);

	// Set calibration offset. See HMC5883L_calibration.ino
	_hmc->setOffset(0, 0);
	_measureTimer = timers().startPeriodicTimer("measureTimer",
			Msg("measureTimer"), 1000);
}

Receive& DigitalCompass::createReceive() {
	return receiveBuilder().match(MsgClass("measureTimer"),
			[this](Msg& msg) {
				ax = -_mpu->getAccX();
				ay = -_mpu->getAccY();
				az = -_mpu->getAccZ();
				gx = _mpu->getGyroX();
				gy = _mpu->getGyroY();
				gz = _mpu->getGyroZ();
				pitch = atan(ax/az)*57.2958;
				roll = atan(ay/az)*57.2958;
				fpitch = pfilter.filter(pitch, gy);
				froll = rfilter.filter(roll, -gx);
				INFO("mpu6050 Acc: ( %.3f, %.3f, %.3f) Gyro: ( %.3f, %.3f, %.3f)", ax, ay, az,gx, gy, gz);
				INFO("mpu6050 Pitch: %.3f Roll: %.3f FPitch: %.3f FRoll: %.3f", pitch,roll,fpitch,froll);
				vMag = _hmc->readRaw();
				calc();
				_publisher.tell(msgBuilder(Publisher::Publish)("e",1),self());
			}).match(MsgClass::Properties(),
			[this](Msg& msg) {sender().tell(replyBuilder(msg)("roll",roll)("pitch",pitch)("acc/x",ax)("acc/y",ay)("acc/z",az),self());}).build();
}

void DigitalCompass::calc() {
	static int16_t mag_x, mag_y, mag_z;
	static int16_t acc_x, acc_y, acc_z;
	static double roll;
	static double pitch;
	static double azimuth;
	static double X_h, Y_h;
	static int8_t x, y;
	static uint8_t acc_buffer[8];

	mag_x = vMag.XAxis;
	mag_y = vMag.YAxis;
	mag_z = vMag.ZAxis;
	_mpu->getRaw(acc_buffer);

	acc_x = (acc_buffer[0] << 8) + (acc_buffer[1]);
	acc_y = (acc_buffer[2] << 8) + (acc_buffer[3]);
	acc_z = (acc_buffer[4] << 8) + (acc_buffer[5]);

	INFO("%d,%d,%d  %d,%d,%d", mag_x, mag_y, mag_z, acc_x, acc_y, acc_z);

	/* Calculate pitch and roll, in the range (-pi,pi) */
	pitch = atan2((double) -acc_x,
			sqrt((long) acc_z * (long) acc_z + (long) acc_y * (long) acc_y));
	roll = atan2((double) acc_y,
			sqrt((long) acc_z * (long) acc_z + (long) acc_x * (long) acc_x));

	/* Calculate Azimuth:
	 * Magnetic horizontal components, after compensating for Roll(r) and Pitch(p) are:
	 * X_h = X*cos(p) + Y*sin(r)*sin(p) + Z*cos(r)*sin(p)
	 * Y_h = Y*cos(r) - Z*sin(r)
	 * Azimuth = arcTan(Y_h/X_h)
	 */
	X_h = (double) mag_x * cos(pitch) + (double) mag_y * sin(roll) * sin(pitch)
			+ (double) mag_z * cos(roll) * sin(pitch);
	Y_h = (double) mag_y * cos(roll) - (double) mag_z * sin(roll);
	azimuth = atan2(Y_h, X_h);
	if (azimuth < 0) { /* Convert Azimuth in the range (0, 2pi) */
		azimuth = 2 * M_PI + azimuth;
	}
	x = 32 + 24 * sin(azimuth);
	y = 32 - 24 * cos(azimuth);

	/* Update display */
	INFO(" azimuth : %d pitch :%d roll:%d ", (int16_t )(azimuth * 180.0 / 3.14),
			(int16_t )(pitch * 180.0 / 3.14), (int16_t )(roll * 180.0 / 3.14));
	/*   oled_set_font(font_16pt);
	 oled_update_number(&oled_azimuth, (int16_t)(azimuth * 180.0 / 3.14), 0);
	 oled_putchar('~');
	 oled_clear_area(OLED_PAGE_RANGE(1, 1), oled_get_column(), OLED_LAST_COLUMN);
	 oled_update_number(&oled_pitch, (int16_t)(pitch * 180.0 / 3.14), 0);
	 oled_putchar('~');
	 oled_clear_area(OLED_PAGE_RANGE(4, 4), oled_get_column(), OLED_LAST_COLUMN);
	 oled_update_number(&oled_roll, (int16_t)(roll * 180.0 / 3.14), 0);
	 oled_putchar('~');
	 oled_clear_area(OLED_PAGE_RANGE(6, 6), oled_get_column(), OLED_LAST_COLUMN);
	 if((x != x_old) || (y != y_old)) {
	 oled_clear_area(OLED_PAGE_RANGE(1, 6), 8, 57);
	 oled_line(32, 32, x, y);
	 }
	 x_old = x;
	 y_old = y;
	 //RED_LED_TOGGLE();*/
}
