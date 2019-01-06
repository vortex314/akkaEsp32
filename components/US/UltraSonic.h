#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <Akka.h>
#include "HCSR04.h"
#include <Publisher.h>

class UltraSonic : public Actor {
		Connector _uext;

		HCSR04* _hcsr;
		int32_t _distance;
		int32_t _delay;
		Uid _measureTimer;
		ActorRef _publisher;

	public:
		UltraSonic(va_list args);
		virtual ~UltraSonic() {};
		void preStart();
		Receive& createReceive();
};

#endif // ULTRASONIC_H
