#ifndef COMPONENT_H
#define COMPONENT_H
#include <Akka.h>
#include <Erc.h>

class Component
{
public :
    typedef enum {
        ST_ON_HOLD,ST_RUNNING
    } State;
    State _state;
public:
    static MsgClass SelfTest;
    static MsgClass Initialize;
    static MsgClass Hold;
    static MsgClass Run;
    virtual Erc selfTest(uint32_t level);
    virtual Erc initialize();
    virtual Erc hold();
    virtual Erc run();

    void state(State st)
    {
        _state=st;
    };
    State state()
    {
        return _state;
    };
    bool isOnHold()
    {
        return _state==ST_ON_HOLD;
    };


};

#endif // COMPONENT_H
