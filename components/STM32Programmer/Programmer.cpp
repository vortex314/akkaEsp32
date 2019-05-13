/*
 * Programmer.cpp
 *
 *  Created on: May 11, 2019
 *      Author: lieven
 */

#include "Programmer.h"
#include "Base64.h"

Programmer::Programmer(Connector* connector, ActorRef& publisher)
		: _connector(connector), _publisher(publisher), _stm32(connector
				->getUART(), connector->getDigitalOut(LP_SCL), connector
				->getDigitalOut(LP_SDA)) {
}

Programmer::~Programmer() {
}

void Programmer::preStart() {
	_stm32.init();
	_timer1 = timers().startPeriodicTimer("timer1", Msg("timer1"), 10000);

}

Receive& Programmer::createReceive() {
	return receiveBuilder()

	.match(LABEL("getId"), [this](Msg& msg) {
		uint16_t id;
		Erc erc = _stm32.getId(id);
		sender().tell(replyBuilder(msg)("erc",erc)("id",id));
	})

	.match(LABEL("getVersion"), [this](Msg& msg) {
		uint8_t version;
		Erc erc =_stm32.getVersion(version);
		std::string str;
		string_format(str,"%X",version);
		sender().tell(replyBuilder(msg)("erc",erc)("version",str));
	})

	.match(LABEL("get"), [this](Msg& msg) {uint8_t version;
		Bytes cmds(100);
		std::string response;
		Erc erc = _stm32.get(version,cmds);
		for(uint32_t i=0;i<cmds.length();i++){
			if ( i !=0 ) response +=':';
			response+= _stm32.findSymbol(cmds.peek(i));
		}
		INFO("STM32 version: 0x%X  cmds : %s ",version,response.c_str());
		sender().tell(replyBuilder(msg)("erc",erc)("cmds",response));
	})

	.match(LABEL("readMemory"), [this](Msg& msg) {
		std::string address;
		INFO(" readMemory %s",msg.toString().c_str());
		if ( msg.get(H("addressHex"),address)) {
			INFO(" readMemory addr : %s ",address.c_str());
			uint32_t addr;
			sscanf(address.c_str(),"%X",&addr);
			Bytes data(256);
			Erc erc =_stm32.readMemory(addr,256,data);
			std::string str;
			Base64::encode(str,data);
			sender().tell(replyBuilder(msg)("erc",erc)("data",str));
		} else {
			sender().tell(replyBuilder(msg)("erc",ENOENT));
		}
	})

	.match(MsgClass("timer1"), [this](Msg& msg) {
		_stm32.resetSystem();
		uint16_t id=0;
		if ( _stm32.getId(id) ) {
			ERROR("stm32.getId() failed.");
		} else {
			INFO("STM32 id = 0x%X ",id);
		}
		_publisher.tell(msgBuilder(Publisher::Publish)("id",id),self());
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {sender().tell(replyBuilder(msg)("x",1),self());})
			.build();
}
