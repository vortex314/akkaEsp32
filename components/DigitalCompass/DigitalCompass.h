/*
 * DigitalCompass.h
 *
 *  Created on: May 4, 2019
 *      Author: lieven
 */

#ifndef DIGITALCOMPASS_H_
#define DIGITALCOMPASS_H_

#include <math.h>
#include <MPU6050.h>
#include <HMC5883L.h>
#include <Kalman.h>
#include <Akka.h>
#include <Bridge.h>

class DigitalCompass : public Actor {
		Connector* _connector;
		ActorRef& _publisher;
		HMC5883L* _hmc;
		MPU6050* _mpu;
		I2C& _i2c;
		MsgClass _measureTimer;
		float ax,ay,az,gx,gy,gz;
		float pitch, roll;
		float fpitch, froll;

		KALMAN pfilter;
		KALMAN rfilter;

		uint32_t lasttime = 0;
		Vector<int16_t> vMag;

	public:
		DigitalCompass(Connector*,ActorRef& );
		virtual ~DigitalCompass() ;
		void preStart();
		Receive& createReceive();
		int16_t calc();
};

#endif /* DIGITALCOMPASS_H_ */
