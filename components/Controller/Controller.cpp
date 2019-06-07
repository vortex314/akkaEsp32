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

	timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 1000);
	timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 50);
}
#include <cmath>
bool delta(float a,float b,float max) {
	if ( abs(a-b)>max) return true;
	return false;
}

void Controller::savePrev() {
	_potLeftPrev = _potLeft;
	_potRightPrev= _potRight;
	_buttonLeftPrev=_buttonLeft;
	_buttonRightPrev=_buttonRight;
}

void Controller::measure() {
	const int weight=0.2;
	_potLeft = _pot_left.getValue()*(1-weight)+_potLeftPrev*weight;
	_potRight= _pot_right.getValue()*(1-weight)+_potRightPrev*weight;

	_buttonLeft = _leftSwitch.read()==1?false:true;
	_buttonRight = _rightSwitch.read()==1?false:true;
}

Receive& Controller::createReceive() {
	return receiveBuilder()
	.match(MsgClass("measureTimer"),	[this](Msg& timer) {
		Msg msg(Bridge::Publish);
		measure();
		if ( delta(_potLeft, _potLeftPrev,2 ))	msg("potLeft",round(_potLeft));
		if ( delta(_potRight, _potRightPrev,2) ) msg("potRight",round(_potRight));
		if ( _buttonLeftPrev != _buttonLeft) msg("buttonLeft",_buttonLeft);
		if ( _buttonRightPrev != _buttonRight ) msg("buttonRight",_buttonRight);
		_bridge.tell(msg,self());
		savePrev();
	})

	.match(MsgClass("reportTimer"),	[this](Msg& timer) {
		measure();
		Msg msg(Bridge::Publish);
		msg("potLeft",round(_potLeft));
		msg("potRight",round(_potRight));
		msg("buttonLeft",_buttonLeft);
		msg("buttonRight",_buttonRight);
		_bridge.tell(msg,self());
		savePrev();
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
