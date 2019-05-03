#include "Neo6m.h"

Neo6m::Neo6m(Connector* connector,ActorRef& mqtt)
    :_connector(connector),_uart(connector->getUART()),_mqtt(mqtt)
{
}

Neo6m::~Neo6m()
{
}


void Neo6m::preStart()
{
    INFO(" GPS NEO6M module started...");
    init();
    _measureTimer = timers().startPeriodicTimer("measureTimer", Msg("measureTimer"), 300);
}

Receive& Neo6m::createReceive()
{
    return receiveBuilder()
    .match(MsgClass("measureTimer"),	[this](Msg& msg) {

    })
    .match(MsgClass::Properties(),[this](Msg& msg) {
        sender().tell(replyBuilder(msg)("connector",_connector->index()),self());
    })
    .build();
}

void Neo6m::onRxd(void* me)
{
    ((Neo6m*) me)->handleRxd();
}

void Neo6m::handleRxd()
{

    while ( _uart.hasData() ) {
        char ch = _uart.read();
        if ( ch=='\n' || ch=='\r') {
            if ( _line.size()>8 ) {
//               INFO("%s",line.c_str());
                std::string topic="src/";
                string_format(topic,"src/%s/%s",self().path(),_line.substr(1,5).c_str());
                Msg msg(Mqtt::Publish);
                msg("topic",topic)("message",_line.substr(6))(AKKA_SRC,self().id())(AKKA_DST,_mqtt.id());
                _mqtt.tell(msg);
            }
            _line.clear();
        } else {
            _line +=ch;
        }
    }
}

int Neo6m::init()
{
    _uart.setClock(9600);
    _uart.onRxd(onRxd,this);
    _uart.init();
    return E_OK;
}
