/*
 * Programmer.h
 *
 *  Created on: May 11, 2019
 *      Author: lieven
 */

#ifndef COMPONENTS_STM32PROGRAMMER_PROGRAMMER_H_
#define COMPONENTS_STM32PROGRAMMER_PROGRAMMER_H_
#include <Akka.h>
#include <Hardware.h>
#include <Mqtt.h>
#include <STM32.h>
class Programmer : public Actor {
		Connector* _connector;
		ActorRef& _mqtt;
		STM32 _stm32;
		Uid _timer1;
public:
	Programmer(Connector* ,ActorRef& );
	virtual ~Programmer() ;
	void preStart();
	Receive& createReceive();
};

#endif /* COMPONENTS_STM32PROGRAMMER_PROGRAMMER_H_ */
