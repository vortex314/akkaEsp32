#include "Compass.h"

Compass::Compass(Connector* connector,ActorRef& publisher) :  _v( { 0, 0, 0 }),_publisher(publisher) {
	_uext=connector;
	_hmc = new 	HMC5883L(*_uext);
	_x=_y=_z=0;
};

Compass::~Compass(){
	delete _uext;
}
void Compass::preStart() {

	if(_hmc->init()) {
		INFO("HMC5883L initialized.");
	} else {
		ERROR("HMC5883L initialization failed.");
	}
	_hmc->setRange(HMC5883L_RANGE_1_3GA);

	// Set measurement mode
	_hmc->setMeasurementMode(HMC5883L_CONTINOUS);

	// Set data rate
	_hmc->setDataRate(HMC5883L_DATARATE_30HZ);

	// Set number of samples averaged
	_hmc->setSamples(HMC5883L_SAMPLES_8);

	// Set calibration offset. See HMC5883L_calibration.ino
	_hmc->setOffset(0, 0);
	_measureTimer = timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 100);
}

Receive& Compass::createReceive() {
	return receiveBuilder()
	.match(MsgClass("measureTimer"),	[this](Msg& msg) {
		_v = _hmc->readNormalize();
//		INFO("%f:%f:%f", _v.XAxis, _v.YAxis, _v.ZAxis);
		_x = _x + (_v.XAxis - _x) / 4;
		_y = _y + (_v.YAxis - _y) / 4;
		_z = _z + (_v.ZAxis - _z) / 4;
		_publisher.tell(msgBuilder(Publisher::Publish)("e",1),self());
	})
	.match(MsgClass::Properties(),[this](Msg& msg) {sender().tell(replyBuilder(msg)("x",_x)("y",_y)("z",_z),self());})
	.build();
}
