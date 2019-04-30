#ifndef GPS_H
#define GPS_H

#include <Akka.h>
#include <Hardware.h>
#include <Neo6m.h>

class Gps : public Actor
{
    int32_t _distance;
    int32_t _delay;
    Label _measureTimer;
    ActorRef& _mqtt;
    Connector _uext;
    Neo6m* _neo6m;

public:
    Gps(Connector& uext,ActorRef& publisher);
    virtual ~Gps() {};
    void preStart();
    Receive& createReceive();
};

#endif // GPS_H
