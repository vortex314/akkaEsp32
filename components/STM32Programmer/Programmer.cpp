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

	.match(LABEL("ping"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg));
	})

	.match(LABEL("getId"), [this](Msg& msg) {
		_stm32.resetSystem();
		uint16_t id;
		Erc erc = _stm32.getId(id);
		sender().tell(replyBuilder(msg)("erc",erc)("id",id));
	})

	.match(LABEL("getVersion"), [this](Msg& msg) {
		_stm32.resetSystem();
		uint8_t version;
		Erc erc =_stm32.getVersion(version);
		std::string str;
		string_format(str,"%X",version);
		sender().tell(replyBuilder(msg)("erc",erc)("version",str));
	})

	.match(LABEL("get"), [this](Msg& msg) {uint8_t version;
		_stm32.resetSystem();
		Bytes cmds(100);
		std::string response;
		Erc erc = _stm32.get(version,cmds);
		for(uint32_t i=0;i<cmds.length();i++) {
			if ( i !=0 ) response +=':';
			response+= _stm32.findSymbol(cmds.peek(i));
		}
		INFO("STM32 version: 0x%X  cmds : %s ",version,response.c_str());
		sender().tell(replyBuilder(msg)("erc",erc)("cmds",response));
	})

	.match(LABEL("readMemory"), [this](Msg& msg) {
		std::string address;
		INFO(" readMemory %s",msg.toString().c_str());
		if ( msg.get(H("addressHex"),address)==0) {
			_stm32.resetSystem();
			INFO(" readMemory addr : %s ",address.c_str());
			uint32_t addr;
			sscanf(address.c_str(),"%X",&addr);
			Bytes data(256);
			Erc erc =_stm32.readMemory(addr,256,data);
			std::string str;
			Base64::encode(str,data);
			sender().tell(replyBuilder(msg)("erc",erc)("data",str)("addressHex",address));
		} else {
			sender().tell(replyBuilder(msg)("erc",ENOENT));
		}
	})

	.match(LABEL("writeMemory"), [this](Msg& msg) {
		std::string addressHex,data;
		INFO(" writeMemory %s",msg.toString().c_str());
		if ( msg.get(H("addressHex"),addressHex)==0 && msg.get("data",data)==0) {
			_stm32.resetSystem();
			Bytes bytes(256);
			Base64::decode(bytes,data);
			INFO(" writeMemory addr : %s ",addressHex.c_str());
			uint32_t addr;
			sscanf(addressHex.c_str(),"%X",&addr);
			Erc erc =_stm32.writeMemory(addr,bytes);
			sender().tell(replyBuilder(msg)("erc",erc)("addressHex",addressHex));
		} else {
			sender().tell(replyBuilder(msg)("erc",ENOENT));
		}
	})

	.match(LABEL("writeUnprotect"), [this](Msg& msg) {
		_stm32.resetSystem();
		sender().tell(replyBuilder(msg)("erc",_stm32.writeUnprotect()));
	})

	.match(LABEL("readUnprotect"), [this](Msg& msg) {
		_stm32.resetSystem();
		sender().tell(replyBuilder(msg)("erc",_stm32.readoutUnprotect()));
	})

	.match(LABEL("eraseAll"), [this](Msg& msg) {
		_stm32.resetSystem();
		sender().tell(replyBuilder(msg)("erc",_stm32.eraseAll()));
	})

	.match(LABEL("resetFlash"), [this](Msg& msg) {
		_stm32.resetSystem();
		sender().tell(replyBuilder(msg)("erc",_stm32.resetFlash()));
	})

	.match(LABEL("resetSystem"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.resetSystem()));
	})

	.match(MsgClass("timer1"), [this](Msg& msg) {
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		const char* modes[]= {"undefined","system","application"};
		sender().tell(replyBuilder(msg)("mode",modes[_stm32.getMode()]),self());
	})

	.build();
}
