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
    std::string _reason;
public:
    static MsgClass SelfTest;
    static MsgClass Initialize;
    static MsgClass Status;
    static MsgClass Run;
    virtual Erc selfTest(uint32_t level,std::string& message);
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
    bool isRunning()
    {
        return _state==ST_RUNNING;
    };
    void setReason(const char* reason)
    {
        _reason=reason;
    }
    const char* getReason()
    {
        return _reason.c_str();
    }

};

#endif // COMPONENT_H
