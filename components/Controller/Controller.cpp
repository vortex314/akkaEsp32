#include "Controller.h"

MsgClass Controller::LedCommand("led");
MsgClass Controller::LedLeft("ledLeft");
MsgClass Controller::LedRight("ledRight");

Controller::Controller(ActorRef& bridge)  : _bridge(bridge),
	_led_right(23),
	_led_left(32),
	_pot_left(36),
	_pot_right(39),
	_leftSwitch(DigitalIn::create(13)),
	_rightSwitch(DigitalIn::create(16))

{

}

Controller::~Controller() {
}

void Controller::preStart() {
	INFO(" Controller started ");
	_leftSwitch.setMode(DigitalIn::DIN_PULL_UP);
	_rightSwitch.setMode(DigitalIn::DIN_PULL_UP);

	_led_right.init();
	_led_left.init();
	_led_left.off();
	_pot_left.init();
	_pot_right.init();
	_leftSwitch.init();
	_rightSwitch.init();

	timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 100);
	timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 50);
}

Receive& Controller::createReceive() {
	return receiveBuilder()
	.match(MsgClass("reportTimer"),	[this](Msg& timer) {
		static float _potLeftOld,_potRightOld;
		static bool _buttonLeftOld,_buttonRightOld;
		Msg msg(Bridge::Publish);
		if ( _potLeft != _potLeftOld )	msg("potLeft",_potLeft);
		if ( _potRight!= _potRightOld )	msg("potRight",_potRight);
		if ( _buttonLeftOld != _leftSwitch.read())	msg("buttonLeft",_leftSwitch.read()==1?false:true);
		if ( _buttonRightOld != _rightSwitch.read() ) 	msg("buttonRight",_rightSwitch.read()==1?false:true);
		_potLeftOld = _potLeft;
		_potRightOld= _potRight;
		_buttonLeftOld=_leftSwitch.read();
		_buttonRightOld=_rightSwitch.read();
		_bridge.tell(msg,self());
	})

	.match(MsgClass("measureTimer"),	[this](Msg& timer) {
		float weight = 0.2;
		_potLeft = _potLeft*(1-weight) + weight*_pot_left.getValue();
		_potRight = _potRight*(1-weight) + weight*_pot_right.getValue();
	})

	.match(Controller::LedLeft,[this](Msg& msg) {
		bool b;
		INFO(" LedLeft %s",msg.toString().c_str());
		if ( msg.get("data",b)) {
			if ( b ) _led_left.on();
			else _led_left.off();
		} else {
			WARN(" no data ");
		}
	})

	.match(Controller::LedRight,[this](Msg& msg) {
		bool b;
		INFO(" LedRight %s",msg.toString().c_str());

		if ( msg.get("data",b)) {
			if ( b ) _led_right.on();
			else _led_right.off();
		} else {
			WARN(" no data ");
		}
	})

	.match(Controller::LedCommand,[this](Msg& msg) {
		INFO(" led command ");
		std::string ledName,mode;
		if ( msg.get("led",ledName) && msg.get("mode",mode)) {
			if ( ledName=="left" ) {
				if (  mode=="on" ) {
					_led_left.on();
				} else {
					_led_left.off();
				}
			} else if ( ledName=="right" ) {
				if (  mode=="on" ) {
					_led_right.on();
				} else {
					_led_right.off();
				}
			}
		}
	})

	.match(MsgClass::Properties(),[this](Msg& msg) {
	})
	.build();
}
