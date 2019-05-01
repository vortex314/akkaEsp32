#include "UltraSonic.h"

UltraSonic::UltraSonic(Connector* connector,ActorRef& publisher ) : _publisher(publisher) {
	_uext=connector;
	_hcsr = new HCSR04(*_uext);
	_distance=0;
	_delay=0;
}

UltraSonic::~UltraSonic(){
delete _uext;
}

void UltraSonic::preStart() {
	_hcsr->init();
	_measureTimer = timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 300);
}

Receive& UltraSonic::createReceive() {
	return receiveBuilder()
	.match(MsgClass("measureTimer"),	[this](Msg& msg) {
		int cm = _hcsr->getCentimeters();
		if(cm < 400 && cm > 0) {
			_distance = _distance + (cm - _distance) / 2;
			_delay = _delay + (_hcsr->getTime() - _delay) / 2;
		}
		_hcsr->trigger();
		_publisher.tell(msgBuilder(Publisher::PollMe)("e",1),self());
	})
	.match(MsgClass::Properties(),[this](Msg& msg) {sender().tell(replyBuilder(msg)("distance",_distance),self());})
	.build();
}
