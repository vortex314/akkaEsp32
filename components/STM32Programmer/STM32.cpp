/*
 * STM32.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "STM32.h"

#define PIN_RESET 4 // GPIO4 D2
#define PIN_BOOT0 5 // GPIO5 D1
#define ACK 0x79
#define NACK 0x1F
#define DELAY 1000

enum {
	BL_GET = 0,
	BL_GET_VERSION = 1,
	BL_GET_ID = 2,
	BL_READ_MEMORY = 0x11,
	BL_GO = 0x21,
	BL_WRITE_MEMORY = 0x31,
	BL_ERASE_MEMORY = 0x43,
	BL_EXTENDED_ERASE_MEMORY = 0x44,
	BL_WRITE_PROTECT = 0x63,
	BL_WRITE_UNPROTECT = 0x73,
	BL_READOUT_PROTECT = 0x82,
	BL_READOUT_UNPROTECT = 0x92
} BootLoaderCommand;
#define XOR(xxx) ((xxx) ^ 0xFF)

uint64_t STM32::_timeout = 0;

void logBytes(const char* location, Bytes& bytes) {
	std::string str;
	bytesToHex(str, bytes.data(), bytes.length());
	INFO(" %s = %d : %s", location, bytes.length(), str.c_str());
}

/*
 Erc STM32::setBoot0(bool flag) {
 if (_boot0 == flag)
 return E_OK;
 digitalWrite(PIN_BOOT0, flag);	// 1 : bootloader mode, 0: flash run
 _boot0 = flag;
 return E_OK;
 }*/

STM32::STM32(UART& uart, DigitalOut& reset, DigitalOut& program) :
		_uart(uart), _reset(reset), _program(program), _in(1024) {
	_usartRxd = 0;
	_mode = M_UNDEFINED;
	_baudrate = 9600;
	_byteDelay = (10 * 1000 / _baudrate) == 0 ? 1 : (10 * 1000 / _baudrate);
}

void STM32::init() {
	INFO("STM32 init UART ");
	Erc erc = _uart.setClock(115200 * 8);
	if (erc)
		ERROR("_uart.setClock(9600)=%d", erc);
	erc = _uart.mode("8E1");
	if (erc)
		ERROR("_uart.mode('8E1')=%d", erc);
	erc = _uart.init();
	if (erc)
		ERROR("_uart.init()=%d", erc);
	_reset.init();
	_program.init();
	_reset.write(1);
	resetSystem();
}

Erc STM32::boot0Flash() {
	_program.write(0);
	return E_OK;
}

Erc STM32::boot0System() {
	_program.write(1);
	return E_OK;
}

//Bytes in(300);

Erc STM32::waitAck(Bytes& out, Bytes& in, uint32_t time) {
	if ( out.length())_uart.write(out.data(), out.length());
	timeout(time);
	while (true) {
		if (timeout()) {
			logBytes("TIMEOUT", in);
			return ETIMEDOUT;
		};
		if (_uart.hasData()) {
			uint8_t b = 0;
			while (_uart.hasData()) {
				in.write(b = _uart.read());
				if (b == ACK)
					return E_OK;
			}
		}
	}
	return E_OK;
}

Erc STM32::readVar(Bytes& in, uint32_t count, uint32_t time) {
	timeout(time);
	while (count) {
		if (timeout()) {
			return ETIMEDOUT;
		};
		if (_uart.hasData()) {
			in.write(_uart.read());
			count--;
		}
	}
	return E_OK;
}

Erc STM32::read(Bytes& in, uint32_t count, uint32_t time) {
	timeout(time);
	while (count) {
		if (timeout()) {
			return ETIMEDOUT;
		};
		if (_uart.hasData()) {
			in.write(_uart.read());
			count--;
		}
	}
	return E_OK;
}

void STM32::flush() {
	while (_uart.hasData()) {
		INFO(" flush : 0x%X ", _uart.read());
	};
	_in.clear();
}

uint8_t xorBytes(uint8_t* data, uint32_t count) {
	uint8_t x = data[0];
	int i = 1;
	while (i < count) {
		x = x ^ data[i];
		i++;
	}
	return x;
}

uint8_t slice(uint32_t word, int offset) {
	return (uint8_t) ((word >> (offset * 8)) & 0xFF);
}

Erc STM32::resetFlash() {
	boot0Flash();
	_reset.write(0);
	Sys::delay(10);
	_reset.write(1);
	Sys::delay(10);
	_mode = M_FLASH;
	return E_OK;
}

Erc STM32::resetSystem() {
	if (_mode == M_SYSTEM)
		return E_OK;
	boot0System();
	_reset.write(0);
	Sys::delay(10);
	_reset.write(1);
	Sys::delay(10);
	INFO(" syncing with STM32");
	uint8_t sync[] = { 0x7F };
	Bytes syncBytes(sync, 1);
	Erc erc = waitAck(syncBytes, _in, DELAY);
	if (erc)
		ERROR(" sync with STM32 failed ");
	_mode = M_SYSTEM;
	return E_OK;
}

Erc STM32::getId(uint16_t& id) {
	uint8_t GET_ID[] = { BL_GET_ID, XOR(BL_GET_ID) };
	Bytes out(GET_ID, 2);
	flush();
	Erc erc = waitAck(out, _in, DELAY);
	if (erc == E_OK) {
		_in.clear();
		erc = readVar(_in, 4, DELAY);
		if (erc == E_OK) {
			id = _in.peek(1) * 256 + _in.peek(0);
		} else {
			ERROR("readVar failed");
		}
	} else {
		ERROR("waitAck failed");
	}
	return erc;
}

Erc STM32::get(uint8_t& version, Bytes& cmds) {
	uint8_t GET[] = { BL_GET, XOR(BL_GET) };
	Bytes buffer(30);
	Bytes out(GET, 2);
	flush();
	Erc erc = waitAck(out, _in, DELAY);
	if (erc == E_OK) {
		_in.clear();
		erc = readVar(buffer, 1, DELAY);
		int length =  buffer.peek(0);
		buffer.clear();
		erc = readVar(buffer, length+2, DELAY);
		if (erc == E_OK) {
			buffer.offset(0);
			version = buffer.read();
			while (buffer.hasData())
				cmds.write(buffer.read());
		} else {
			ERROR("readVar failed");
		}
	} else {
		ERROR("waitAck failed");
	}
	return erc;
}

Erc STM32::getVersion(uint8_t& version) {
	uint8_t GET_VERSION[] = { BL_GET_VERSION, XOR(BL_GET_VERSION) };
	Bytes out(GET_VERSION, 2);
	Bytes in(4);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	if ((erc = waitAck(out, in, DELAY)) == E_OK) {
		in.clear();
		if ((erc = waitAck(noData, in, DELAY)) == E_OK) {
			version = in.peek(0);
		}
	}
	return erc;
}

Erc STM32::writeMemory(uint32_t address, Bytes& data) {
	uint8_t GET[] = { BL_WRITE_MEMORY, XOR(BL_WRITE_MEMORY) };
	uint8_t ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address,
			1), slice(address, 0), xorBytes(ADDRESS, 4) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	if ((erc = waitAck(out.map(GET, 2), _in, DELAY)) == E_OK) {
		if ((erc = waitAck(out.map(ADDRESS, 5), _in, DELAY)) == E_OK) {
			_uart.write(data.length() - 1);
			_uart.write(data.data(), data.length());
			_uart.write(
					((uint8_t) (data.length() - 1))
							^ xorBytes(data.data(), data.length()));
			if ((erc = waitAck(noData, _in, 200)) == E_OK) {

			}
		}
	}
	return erc;
}

Erc STM32::readMemory(uint32_t address, uint32_t length, Bytes& data) {
	uint8_t READ_MEMORY[] = { BL_READ_MEMORY, XOR(BL_READ_MEMORY) };
	uint8_t ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address,
			1), slice(address, 0), xorBytes(ADDRESS, 4) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(READ_MEMORY, 2), _in, DELAY);
	if (erc)
		return erc;
	ADDRESS[4] = xorBytes(ADDRESS, 4);
	erc = waitAck(out.map(ADDRESS, 5), _in, DELAY);
	if (erc)
		return erc;
	_uart.write(length - 1);
	_uart.write(XOR(length - 1));
	erc = waitAck(noData, _in, DELAY);
	if (erc)
		return erc;
	if ((erc = read(data, length, 200)) == E_OK) {
	}
	return erc;
}

Erc STM32::go(uint32_t address) {
	uint8_t GO[] = { BL_GO, XOR(BL_GO) };
	uint8_t ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address,
			1), slice(address, 0), xorBytes(ADDRESS, 4) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(GO, 2), _in, DELAY);
	if (erc)
		return erc;
	ADDRESS[4] = xorBytes(ADDRESS, 4);
	erc = waitAck(out.map(ADDRESS, 5), _in, DELAY);
	return erc;
}

Erc STM32::eraseMemory(Bytes& pages) {
	uint8_t ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };

	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(ERASE_MEMORY, 2), _in, DELAY);
	if (erc)
		return erc;
	_uart.write(pages.length() - 1);
	_uart.write(pages.data(), pages.length());
	_uart.write(
			((uint8_t) (pages.length() - 1))
					^ xorBytes(pages.data(), pages.length()));
	erc = waitAck(noData, _in, 200);
	return erc;
}

Erc STM32::eraseAll() {
	uint8_t ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };
	uint8_t ALL_PAGES[] = { 0xFF, 0x00 };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(ERASE_MEMORY, 2), _in, DELAY);
	if (erc)
		return erc;
	erc = waitAck(out.map(ALL_PAGES, 2), _in, 200);
	return erc;
}

Erc STM32::extendedEraseMemory() {
	uint8_t EXTENDED_ERASE_MEMORY[] = { BL_EXTENDED_ERASE_MEMORY, XOR(
			BL_EXTENDED_ERASE_MEMORY) };
	uint8_t ALL_PAGES[] = { 0xFF, 0xFF, 0 };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(EXTENDED_ERASE_MEMORY, 2), _in, DELAY);
	if (erc)
		return erc;
	erc = waitAck(out.map(ALL_PAGES, 3), _in, 200);
	return erc;
}

Erc STM32::writeProtect(Bytes& sectors) {
	uint8_t WP[] = { BL_WRITE_PROTECT, XOR(BL_WRITE_PROTECT) };

	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(WP, 2), _in, DELAY);
	if (erc)
		return erc;
	_uart.write(sectors.length() - 1);
	_uart.write(sectors.data(), sectors.length());
	_uart.write(
			((uint8_t) (sectors.length() - 1))
					^ xorBytes(sectors.data(), sectors.length()));
	erc = waitAck(noData, _in, 200);
	return erc;
}

Erc STM32::writeUnprotect() {
	uint8_t WUP[] = { BL_WRITE_UNPROTECT, XOR(BL_WRITE_UNPROTECT) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(WUP, 2), _in, DELAY);
	if (erc)
		return erc;
	erc = waitAck(noData, _in, DELAY);
	return erc;
}

Erc STM32::readoutProtect() {
	uint8_t RDP[] = { BL_READOUT_PROTECT, XOR(BL_READOUT_PROTECT) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(RDP, 2), _in, DELAY);
	if (erc)
		return erc;
	erc = waitAck(noData, _in, DELAY);
	return erc;
}

Erc STM32::readoutUnprotect() {
	uint8_t RDUP[] = { BL_READOUT_UNPROTECT, XOR(BL_READOUT_UNPROTECT) };
	Bytes out(0);
	Bytes noData(0);
	flush();
	Erc erc = E_OK;
	erc = waitAck(out.map(RDUP, 2), _in, DELAY);
	if (erc)
		return erc;
	erc = waitAck(noData, _in, DELAY);
	return erc;
}

bool STM32::timeout() {
	return Sys::millis() > _timeout;
}

void STM32::timeout(uint32_t delta) {
	_timeout = Sys::millis() + delta;
}

Bytes uartBuffer(256);
uint32_t lastDataReceived = 0;
uint32_t firstDataReceived = 0;
bool sendNow = false;

void STM32::loop() {
	/*	if (_uart.hasData() && (uartBuffer.length() == 0)) {
	 firstDataReceived = Sys::millis();
	 }
	 while (_uart.hasData()) {
	 uint8_t b = _uart.read();
	 _usartRxd++;
	 if (b == '\n')
	 sendNow = true;
	 uartBuffer.write(b);
	 lastDataReceived = Sys::millis();
	 };
	 if (uartBuffer.length() > 200)
	 sendNow = true;
	 if (((lastDataReceived - firstDataReceived) > 100))	// more then 100 msec delay
	 sendNow = true;
	 if (sendNow && uartBuffer.length()) {
	 std::string msg;
	 uartBuffer.offset(0);
	 while (uartBuffer.hasData())
	 msg.append((char) uartBuffer.read());
	 mqttPublish("usart/rxd", msg);
	 uartBuffer.clear();
	 sendNow = false;
	 }*/
}
