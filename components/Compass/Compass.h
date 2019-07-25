#ifndef COMPASS_H
#define COMPASS_H
#include <Akka.h>
#include <HMC5883L.h>
#include <Bridge.h>

class Compass : public Actor {
		Connector* _uext;
		HMC5883L* _hmc;
		struct Vector<float> _v;
		int32_t _x,_y,_z;
		ActorRef& _bridge;
		Uid _measureTimer;
	public:
		Compass(Connector*,ActorRef& );
		virtual ~Compass() ;
		void preStart();
		Receive& createReceive();
};

#endif // COMPASS_H
