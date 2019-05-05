/*
 * Triac.cpp
 *
 *  Created on: May 5, 2019
 *      Author: lieven
 */

#include "Triac.h"
#include "driver/gpio.h" // to get the IRAM_ATTR

 void IRAM_ATTR Triac::zeroDetectHandler(void* me) {
	Triac* triac=(Triac*)me;
	triac->_zeroCounter++;
}

Triac::Triac(Connector* connector, ActorRef& publisher) :
		_publisher(publisher), _zeroDetect(connector->getDigitalIn(LP_SCL)) {
	_uext = connector;
	_zeroCounter = 0;
}

Triac::~Triac() {
	delete _uext;
}

void Triac::preStart() {
	_zeroDetect.onChange(DigitalIn::DIN_FALL, zeroDetectHandler, this);
	_zeroDetect.init();
	_measureTimer = timers().startPeriodicTimer("measureTimer",
			Msg("measureTimer"), 1000);
}

Receive& Triac::createReceive() {
	return receiveBuilder()

	.match(MsgClass("measureTimer"),
			[this](Msg& msg) {
				INFO(" zeroCounter : %u",_zeroCounter);
				_publisher.tell(msgBuilder(Publisher::Publish)("zeroDetect",_zeroDetect.read())("zeros",_zeroCounter),self());
			})

	.match(MsgClass::Properties(),
			[this](Msg& msg) {sender().tell(replyBuilder(msg)("triac","BT130"),self());}).build();
}

