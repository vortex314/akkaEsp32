#include "Led.h"

Led::Led(PhysicalPin pin) : _dout(DigitalOut::create(pin)) {

}

Led::~Led() {
}

void Led::on() {
	_isOn=true;
	_dout.write(0);
}

void Led::off() {
	_isOn=false;
	_dout.write(1);
}

void Led::init() {
	_dout.init();
	off();
}

bool Led::isOn() {
	return _isOn;
}
