#ifndef SYSTEM_H
#define SYSTEM_H

#include <Akka.h>
#include <Hardware.h>

class System : public Actor {
    uid_type _ledTimer;
    uid_type _reportTimer;
    DigitalOut& _ledGpio;
    uint32_t _interval = 100;

  public:
    System(va_list args);
    ~System();
    void preStart();
    Receive& createReceive();
};

#endif // SYSTEM_H
