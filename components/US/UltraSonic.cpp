#include "UltraSonic.h"

UltraSonic::UltraSonic(va_list args ) : _uext(1) {
//	_hcsr = new HCSR04(_uext.getDigitalOut(LP_SCL),_uext.getDigitalIn(LP_SDA));
	_hcsr = new HCSR04(_uext);
	_publisher = va_arg(args,ActorRef) ;
	_distance=0;
	_delay=0;
}

void UltraSonic::preStart() {
	_hcsr->init();
	_measureTimer = timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 100);
}

Receive& UltraSonic::createReceive() {
	return receiveBuilder()
	.match(MsgClass("measureTimer"),	[this](Envelope& msg) {
//		INFO("%d",_hcsr->getCentimeters());
		int cm = _hcsr->getCentimeters();
		if(cm < 400 && cm > 0) {
			_distance = _distance + (cm - _distance) / 2;
			_delay = _delay + (_hcsr->getTime() - _delay) / 2;
			/*		INFO("distance : %d micro-sec = %d cm ", _delay,
					     _distance);*/
		}
		_hcsr->trigger();
		_publisher.tell(Msg(Publisher::PollMe)("e",1),self());
	})
	.match(Properties(),[this](Envelope& msg) {sender().tell(msg.reply()("distance",_distance),self());})
	.build();
}
