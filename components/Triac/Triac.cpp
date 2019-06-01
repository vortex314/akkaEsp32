/*
 * Triac.cpp
 *
 *  Created on: May 5, 2019
 *      Author: lieven
 */

#include "Triac.h"
#include "driver/gpio.h" // to get the IRAM_ATTR

void IRAM_ATTR Triac::zeroDetectHandler(void* me) {
	Triac* triac = (Triac*) me;
	triac->_zeroCounter++;
}

Triac::Triac(Connector* connector, ActorRef& bridge) :
	_bridge(bridge), _zeroDetect(connector->getDigitalIn(LP_SCL)), _triacGate(
	    connector->getDigitalOut(LP_TXD)),_currentSense(connector->getADC(LP_SDA)) {
	_connector = connector;
	_zeroCounter = 0;
}

Triac::~Triac() {
	delete _connector;
}

void Triac::preStart() {
	_zeroDetect.onChange(DigitalIn::DIN_RAISE, zeroDetectHandler, this);
	_zeroDetect.init();
	_triacGate.init();
	_triacGate.write(1);
	_currentSense.init();
	_measureTimer = timers().startPeriodicTimer("measureTimer",
	                Msg("measureTimer"), 1000);
}

Receive& Triac::createReceive() {
	return receiveBuilder()

	       .match(MsgClass("measureTimer"),
	[this](Msg& msg) {
		static bool triacGate=false;
		if ( triacGate ) {
			_triacGate.write(1);
			triacGate=false;
		} else {
			_triacGate.write(0);
			triacGate=true;
		}
		INFO(" zeroCounter : %u current : %d",_zeroCounter,_currentSense.getValue());
		_bridge.tell(msgBuilder(Bridge::Publish)("zeroDetect",_zeroDetect.read())("zeros",_zeroCounter),self());
	})

	.match(MsgClass::Properties(),
	[this](Msg& msg) {sender().tell(replyBuilder(msg)("triac","BT130"),self());}).build();
}
