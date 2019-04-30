#include "Gps.h"

Gps::Gps(Connector& uext,ActorRef& mqtt) : _mqtt(mqtt),_uext(uext)
{
    _neo6m = new Neo6m(_uext,mqtt);
}

void Gps::preStart()
{
    INFO(" GPS NEO6M module started...");
    _neo6m->init();
    _measureTimer = timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 300);
}

Receive& Gps::createReceive()
{
    return receiveBuilder()
    .match(MsgClass("measureTimer"),	[this](Msg& msg) {

    })
    .match(MsgClass::Properties(),[this](Msg& msg) {
        sender().tell(replyBuilder(msg)("gps",_distance),self());
    })
    .build();
}
