#include "Neo6m.h"

Neo6m::Neo6m(Connector& connector,ActorRef& mqtt)
    :_connector(connector),_uart(connector.getUART()),_mqtt(mqtt)
{
}

Neo6m::~Neo6m()
{
}

void Neo6m::onRxd(void* me)
{
    ((Neo6m*) me)->handleRxd();
}

void Neo6m::handleRxd()
{
    static std::string line;
    while ( _uart.hasData() ) {
        char c = _uart.read();
        if ( c=='\n' || c=='\r') {
            if ( line.size()>7 ) {
                INFO("%s",line.c_str());

                std::string topic="src/ESP32-12857/GPS/";
                topic += line.substr(1,5);
                Msg msg(Mqtt::Publish);
                msg("topic",topic)("message",line.substr(7))(AKKA_SRC,0)(AKKA_DST,_mqtt.id());
                _mqtt.tell(msg);
            }
            line.clear();
        } else {
            line +=c;
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
