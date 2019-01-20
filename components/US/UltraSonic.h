#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "HCSR04.h"
#include <Akka.h>
#include <Publisher.h>

class UltraSonic : public Actor {
    Connector _uext;
    HCSR04* _hcsr;
    int32_t _distance;
    int32_t _delay;
    Label _measureTimer;
    ActorRef& _publisher;

  public:
    UltraSonic(ActorRef& ref);
    virtual ~UltraSonic(){};
    void preStart();
    Receive& createReceive();
};

#endif // ULTRASONIC_H
