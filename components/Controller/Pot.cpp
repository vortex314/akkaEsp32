#include "Pot.h"


Pot::Pot(PhysicalPin pin) : _adc(ADC::create(pin)) {
}

Pot::~Pot() {
}

void Pot::init() {
	_adc.init();
}

float Pot::getValue() {
	return _adc.getValue();
}
