#ifndef MOTORSERVO_H
#define MOTORSERVO_H

#include <Akka.h>
#include <Bridge.h>
#include <Hardware.h>
#include <MedianFilter.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define SERVO_MAX_SAMPLES 16

class MotorServo : public Actor
{

// D34 : L_IS
// D35 : R_IS
// D25 : ENABLE
// D26 : L_PWM
// D27 : R_PWM
// D32 : ADC POT
    ADC& _adcPot;
    ADC& _adcIS;
    DigitalOut& _pinLeftEnable;
    DigitalOut& _pinRightEnable;
    uint32_t _pinPwmLeft;
    uint32_t _pinPwmRight;
    ActorRef& _bridge;
    MedianFilter<int,11> _potFilter;

    mcpwm_unit_t _mcpwm_num;
    mcpwm_timer_t _timer_num;
    float _angleMeasured=0.0;
    float _angleTarget=20;
    float _KP=15;
    float _KI=0.005;
    float _KD=0;
    float _bias=0;
    float _error=0;
    float _errorPrior=0;
    float _iteration_time=1;
    float _integral=0;
    float _derivative=0;
    float _output=0;
    float _angleSamples[SERVO_MAX_SAMPLES];
    uint32_t _indexSample=0;
    float _current;
    int _watchdogCounter;
    int _directionTargetLast;

public:
    static MsgClass targetAngle;
    MotorServo(Connector* connector,ActorRef& bridge);
    MotorServo(uint32_t pinPot, uint32_t pinIS,
               uint32_t pinLeftEnable, uint32_t pinRightEnable,
               uint32_t pinLeftPwm, uint32_t pinRightPwm,
               ActorRef& bridge);
    ~MotorServo();
    void preStart();
    Receive& createReceive();
    void calcTarget(float);
    float PID(float error, float interval);
    void setDirection(float output);
    void left(float);
    void right(float);
    void setOutput(float output);
    float filterAngle(float inp);
    void round(float& f,float resol);
};

#endif // MOTORSERVO_H
