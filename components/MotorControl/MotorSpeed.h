#ifndef MOTORSPEED_H
#define MOTORSPEED_H

#include <Akka.h>
#include <Hardware.h>
#include <Bridge.h>
#include <Component.h>
#include <BTS7960.h>
#include <RotaryEncoder.h>

#include <MedianFilter.h>

#include "driver/mcpwm.h"
#include "driver/pcnt.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "soc/rtc.h"



typedef enum { SIG_CAPTURED = 2 } Signal;

class MotorSpeed : public Actor, Component
{

    BTS7960 _bts7960;
    RotaryEncoder _rotaryEncoder;
    ActorRef& _bridge;

//int _direction = 1;
//   int _directionPrev = 1;
    int _rpmTarget = 40;
    int _directionTargetLast;
    float _rpmFiltered;
    float _KP = 0.03;
    float _KI = 0.001;
    float _KD = 0;
    float _bias = 0;
    float _error = 0;
    float _errorPrior = 0;
    float _sample_time = 1;
    float _integral = 0.0;
    float _derivative = 0;
    float _output = 0.0;
//   float _angleFiltered;
    // AverageFilter<float>* _rpmMeasuredFilter;
    Uid _controlTimer;
    int _watchdogCounter;
    float _currentLeft ;
    float _currentRight;
//	    int _pwmSign = 1;




public:
    static MsgClass TargetSpeed;
    MotorSpeed(Connector* connector, ActorRef& bridge);
    MotorSpeed(uint32_t pinLeftIS, uint32_t pinrightIS, uint32_t pinLeftEnable,
               uint32_t pinRightEnable, uint32_t pinLeftPwm,
               uint32_t pinRightPwm, uint32_t pinTachoA, uint32_t pinTachoB,
               ActorRef& bridge);
    ~MotorSpeed();

    void calcTarget(float);
    float PID(float error, float interval);


    void round(float& f, float resol);
    void preStart();
    Receive& createReceive();

    int32_t deltaToRpm(uint32_t delta, int direction);

    Erc selfTest(uint32_t level,std::string& message);
    Erc initialize();
    Erc hold();
    Erc run();
};

#endif // MOTORSPEED_H
