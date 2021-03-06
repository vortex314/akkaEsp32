/*
 * Programmer.cpp
 *
 *  Created on: May 11, 2019
 *      Author: lieven
 */

#include "Programmer.h"
#include "Base64.h"

Programmer::Programmer(Connector* connector, ActorRef& mqtt)
		: _connector(connector), _mqtt(mqtt), _stm32(connector->getUART(), connector
				->getDigitalOut(LP_SCL), connector->getDigitalOut(LP_SDA)) {
}

Programmer::~Programmer() {
}

void Programmer::preStart() {
	_stm32.init();
	_timer1 = timers().startPeriodicTimer("timer1", Msg("timer1"), 100);

}

Receive& Programmer::createReceive() {
	return receiveBuilder()

	.match(UID("ping"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg));
	})

	.match(UID("getId"), [this](Msg& msg) {
		uint16_t id;
		Erc erc = _stm32.getId(id);
		sender().tell(replyBuilder(msg)("erc",erc)("id",id));
	})

	.match(UID("getVersion"), [this](Msg& msg) {
		uint8_t version;
		Erc erc =_stm32.getVersion(version);
		std::string str;
		string_format(str,"%X",version);
		sender().tell(replyBuilder(msg)("erc",erc)("version",str));
	})

	.match(UID("get"), [this](Msg& msg) {uint8_t version;
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

	.match(UID("readMemory"), [this](Msg& msg) {
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
			sender().tell(replyBuilder(msg)("erc",erc)("data",str)("addressHex",address));
		} else {
			sender().tell(replyBuilder(msg)("erc",ENOENT));
		}
	})

	.match(UID("writeMemory"), [this](Msg& msg) {
		std::string addressHex,data;
		if ( msg.get(H("addressHex"),addressHex) && msg.get("data",data)) {
			Bytes bytes(256);
			Base64::decode(bytes,data);
			uint32_t addr;
			sscanf(addressHex.c_str(),"%X",&addr);
			Erc erc =_stm32.writeMemory(addr,bytes);
			sender().tell(replyBuilder(msg)("erc",erc)("addressHex",addressHex));
			INFO(" writeMemory addr : %s = %d ",addressHex.c_str(),erc);
		} else {
			sender().tell(replyBuilder(msg)("erc",ENOENT));
			WARN("cannot read arguments : %s ",((Xdr)msg).toString().c_str());
		}
	})

	.match(UID("writeUnprotect"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.writeUnprotect()));
	})

	.match(UID("readUnprotect"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.readoutUnprotect()));
	})

	.match(UID("eraseAll"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.eraseAll()));
	})

	.match(UID("resetFlash"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.resetFlash()));
	})

	.match(UID("resetSystem"), [this](Msg& msg) {
		sender().tell(replyBuilder(msg)("erc",_stm32.resetSystem()));
	})

	.match(MsgClass("timer1"), [this](Msg& msg) {
		if ( _stm32.getMode()==STM32::M_FLASH) {
			Bytes data(1024);
			_stm32.read(data,1024,10);
			if ( data.length() ) {
				std::string s((char*)data.data(),data.length());
//				Base64::encode(s,data);
				_mqtt.tell(msgBuilder(Mqtt::Publish)
						("topic","src/ESP32-12857/programmer/uart")("message",s),self());
			}
		}
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		const char* modes[]= {"undefined","system","application"};
		sender().tell(replyBuilder(msg)("mode",modes[_stm32.getMode()]),self());
	})

	.build();
}
