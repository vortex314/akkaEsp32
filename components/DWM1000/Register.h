/*
 * Register.h
 *
 *  Created on: 25 Apr 2019
 *      Author: lieven
 */

#ifndef COMPONENTS_DWM1000_REGISTER_H_
#define COMPONENTS_DWM1000_REGISTER_H_
#include <stdint.h>
#include <string>
#include <Log.h>
class Register {
	const char* _name;
	uint32_t _reg;
	const char* _format;
public:
	Register(const char* name,const char* format);
	virtual ~Register();
	void value(uint32_t);
	void show();
};


#endif /* COMPONENTS_DWM1000_REGISTER_H_ */
