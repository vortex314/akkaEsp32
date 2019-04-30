#ifndef NEO6M_H
#define NEO6M_H
#include <Hardware.h>
#include <Log.h>
#include <Akka.h>
#include <Mqtt.h>
class Neo6m
{
    Connector& _connector;
    UART& _uart;
    static void onRxd(void*);
    ActorRef& _mqtt;
public:
    Neo6m(Connector& connector,ActorRef& mqtt);
    ~Neo6m();
    int init();
    void handleRxd();
};

#endif // NEO6M_H
