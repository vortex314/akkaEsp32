#include "Controller.h"

MsgClass Controller::LedCommand("led");

Controller::Controller(ActorRef& publisher)  : _publisher(publisher),
	_led_right(23),
	_led_left(32),
	_pot_left(36),
	_pot_right(39),
	_leftSwitch(DigitalIn::create(13)),
	_rightSwitch(DigitalIn::create(16))

{
	_potLeftFilter=new AverageFilter<uint32_t>();
	_potRightFilter=new AverageFilter<uint32_t>();
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

	timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 300);
	timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 50);
}

Receive& Controller::createReceive() {
	return receiveBuilder()
	.match(MsgClass("reportTimer"),	[this](Msg& timer) {
		Msg msg(Publisher::Publish);
		msg("potLeft",_potLeft)("potRight",_potRight);
		msg("buttonLeft",_leftSwitch.read()==1?false:true);
		msg("buttonRight",_rightSwitch.read()==1?false:true);
		_publisher.tell(msg,self());
	})

	.match(MsgClass("measureTimer"),	[this](Msg& timer) {
		float weight = 0.2;
		_potLeft = _potLeft*(1-weight) + weight*_pot_left.getValue();
		_potRight = _potRight*(1-weight) + weight*_pot_right.getValue();
	})

	.match(Controller::LedCommand,[this](Msg& msg) {
		INFO(" led command ");
		std::string ledName,mode;
		if ( msg.get("led",ledName)==0 && msg.get("mode",mode)==0) {
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
