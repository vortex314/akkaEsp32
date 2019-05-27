#ifndef LED_H
#define LED_H
#include <Hardware.h>
class Led {
		DigitalOut& _dout;
		bool _isOn;
	public:
		Led(PhysicalPin pin);
		~Led();
		void init();
		void on();
		void off();
		bool isOn();

};

#endif // LED_H
