/*
 * Triac.h
 *
 *  Created on: May 5, 2019
 *      Author: lieven
 */

#ifndef COMPONENTS_TRIAC_TRIAC_H_
#define COMPONENTS_TRIAC_TRIAC_H_

#include "HCSR04.h"
#include <Akka.h>
#include <Publisher.h>

class Triac : public Actor {
    Connector* _uext;
    Label _measureTimer;
    ActorRef& _publisher;
    DigitalIn& _zeroDetect;
    uint32_t _zeroCounter;

  public:
    Triac(Connector*,ActorRef& );
    virtual ~Triac();
    void preStart();
    Receive& createReceive();

    static void zeroDetectHandler(void*);
};

#endif /* COMPONENTS_TRIAC_TRIAC_H_ */
