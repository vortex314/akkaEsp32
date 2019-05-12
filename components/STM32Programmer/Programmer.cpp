/*
 * Programmer.cpp
 *
 *  Created on: May 11, 2019
 *      Author: lieven
 */

#include "Programmer.h"

Programmer::Programmer(Connector* connector, ActorRef& publisher)
		: _connector(connector), _publisher(publisher), _stm32(connector
				->getUART(), connector->getDigitalOut(LP_SCL), connector
				->getDigitalOut(LP_SDA)) {
}

Programmer::~Programmer() {
}

void Programmer::preStart() {
	_stm32.init();
	_timer1 = timers().startPeriodicTimer("timer1", Msg("timer1"), 1000);

}

Receive& Programmer::createReceive() {
	return receiveBuilder().match(MsgClass("timer1"), [this](Msg& msg) {
		_stm32.resetSystem();
		uint16_t id=0;
		if ( _stm32.getId(id) ) {
			ERROR("stm32.getId() failed.");
		} else {
			INFO("STM32 id = 0x%X ",id);
		}
		uint8_t version;
		Bytes cmds(100);
		std::string str;

		if ( _stm32.get(version,cmds) ) {
			ERROR(" stm32.get() failed.");
		} else {
			bytesToHex(str,cmds.data(),cmds.length());
			INFO("STM32 version: 0x%X  cmds : %s ",version,str.c_str());
		}
		if ( _stm32.getVersion(version)) {
			ERROR("stm32.getVersion() failed.");
		} else {
			INFO("STM32 version:0x%X",version);
		}
		Bytes data(256);
		if ( _stm32.readMemory(0x08000000,256,data)) {
			ERROR("stm32.readMemory() failed.");
		} else {
			bytesToHex(str,data.data(),data.length());
			INFO("STM32 readmemory  : %s ",str.c_str());
		}
		_publisher.tell(msgBuilder(Publisher::Publish)("id",id),self());
	}).match(MsgClass::Properties(), [this](Msg& msg) {sender().tell(replyBuilder(msg)("x",1),self());})
			.build();
}
