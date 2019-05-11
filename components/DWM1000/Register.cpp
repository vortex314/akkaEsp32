/*
 * Register.cpp
 *
 *  Created on: 25 Apr 2019
 *      Author: lieven
 */

#include "Register.h"

Register::Register(const char* name, const char* format)
		:  _name(name),_format(format) {
	_reg = 0;
}

Register::~Register() {
}

void Register::value(uint32_t reg) {
	_reg = reg;
}

void Register::show() {
	std::string str;
	uint32_t reg = _reg;
	uint32_t bits=0;
	uint32_t bitCount=1;
	const char* fmt = _format;
	while (fmt != 0) {

		const char* next = strchr(fmt, ' ');
		if (reg & 0x80000000) {
			bits += 1;
		}
		if (*fmt == '+') {
			bitCount++;
		} else {
			if (bitCount > 1 || bits>0 ) {
				if (next == 0)
					str.append(fmt);
				else
					str.append(fmt, next - fmt);
			}
			if (bitCount > 1) {
				str.append("=");
				char numstr[21]; // enough to hold all numbers up to 64-bits
				sprintf(numstr, "0x%X", bits);
				str +=  numstr;			}
			bits = 0;
			bitCount = 1;
			str.append(",");
		}

		bits <<= 1;
		reg <<= 1;
		fmt = next;
		if (fmt) fmt++;
	}
	INFO(" %10s = %08X |%s|", _name, _reg, str.c_str());
}

