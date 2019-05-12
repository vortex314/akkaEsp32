/*
 * Programmer.cpp
 *
 *  Created on: May 11, 2019
 *      Author: lieven
 */

#include "Programmer.h"

Programmer::Programmer(Connector* connector, ActorRef& publisher) :
		_connector(connector),
		_publisher(publisher),
		_stm32(connector->getUART(),connector->getDigitalOut(LP_SCL),connector->getDigitalOut(LP_SDA)) {
}

Programmer::~Programmer() {
}

void Programmer::preStart() {
_stm32.init();
_timer1 = timers().startPeriodicTimer("timer1", Msg("timer1"), 1000);

}

Receive& Programmer::createReceive() {
	return receiveBuilder()
	.match(MsgClass("timer1"),	[this](Msg& msg) {
		_stm32.resetSystem();
		uint16_t id=0;
		INFO(" >>>> stm32.getId() ");
		Erc erc = _stm32.getId(id);
		INFO(" <<<< stm32.getId()=%d => id = 0x%X ",erc,id);
		_publisher.tell(msgBuilder(Publisher::Publish)("id",id),self());
	})
	.match(MsgClass::Properties(),[this](Msg& msg) {sender().tell(replyBuilder(msg)("x",1),self());})
	.build();
}
