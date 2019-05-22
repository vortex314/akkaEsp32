#ifndef SYSTEM__H
#define SYSTEM__H

#include <Akka.h>
#include <Hardware.h>

class System : public Actor {
		Label _ledTimer;
		Label _reportTimer;
		DigitalOut& _ledGpio;
		uint32_t _interval = 100;
		ActorRef& _mqtt;

	public:
		static MsgClass LedPulseOn;
		static MsgClass LedPulseOff;
		System(ActorRef& mqtt);
		~System();
		void preStart();
		Receive& createReceive();
};

#endif // SYSTEM_H
