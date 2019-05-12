/*
 * Stm32.h
 *
 *  Created on: Jun 28, 2016
 *      Author: lieven
 */

#ifndef STM32_H_
#define STM32_H_

#include <ArduinoJson.h>
#include <Base64.h>
#include <Hardware.h>
#include <Log.h>
#include <Bytes.h>

class STM32 {
	UART& _uart;
	DigitalOut& _reset;
	DigitalOut& _program;
	uint32_t _usartRxd;
	static uint64_t _timeout;
	Bytes _in;
	uint32_t _byteDelay;
	uint32_t _baudrate;
public:
	typedef enum {
		M_UNDEFINED,M_SYSTEM, M_FLASH
	} Mode;
private:
	Mode _mode;
public:
	enum Op {
		X_WAIT_ACK = 0x40,
		X_SEND = 0x41,
		X_RECV = 0x42,
		X_RECV_VAR = 0x43,
		X_RECV_VAR_MIN_1 = 0x44,
		X_RESET,
		X_BOOT0
	};
	STM32(UART& ,DigitalOut& ,DigitalOut& ) ;
	void init();
	Erc begin();

	Erc resetFlash();
	Erc resetSystem();
	Erc getId(uint16_t& id);
	Erc getVersion(uint8_t& version);
	Erc get(uint8_t& version, Bytes& cmds);
	Erc writeMemory(uint32_t address, Bytes& data);
	Erc readMemory(uint32_t address, uint32_t length, Bytes& data);
	Erc eraseMemory(Bytes& pages);
	Erc extendedEraseMemory();
	Erc eraseAll();
	Erc writeProtect(Bytes& sectors);
	Erc writeUnprotect();
	Erc readoutProtect();
	Erc readoutUnprotect();
	Erc go(uint32_t address);
	int getMode() {
		return _mode;
	}

	void loop();

	Erc waitAck(Bytes& out, Bytes& in,  uint32_t timeout);
	Erc readVar(Bytes& in, uint32_t max, uint32_t timeout);
	Erc read(Bytes& in, uint32_t lenghth, uint32_t timeout);
	Erc setBoot0(bool);
	Erc setAltSerial(bool);
	Erc engine(Bytes& reply, Bytes& req);
	Erc boot0Flash();
	Erc boot0System();bool timeout();
	void timeout(uint32_t delta);
	void flush();
};

#endif /* STM32_H_ */
