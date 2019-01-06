#ifndef COMPASS_H
#define COMPASS_H
#include <Akka.h>
#include <HMC5883L.h>
#include <Publisher.h>

class Compass : public Actor {
		Connector _uext;
		HMC5883L* _hmc;
		struct Vector _v;
		int32_t _x,_y,_z;
		ActorRef _publisher;
		Uid _measureTimer;
	public:
		Compass(va_list args);
		virtual ~Compass() {};
		void preStart();
		Receive& createReceive();
};

#endif // COMPASS_H
