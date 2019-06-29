#include "MotorServo.h"

#define MAX_PWM 20

MotorServo::MotorServo(uint32_t pinLeftIS, uint32_t pinRightIS,
                       uint32_t pinLeftEnable, uint32_t pinRightEnable,
                       uint32_t pinLeftPwm, uint32_t pinRightPwm,
                       uint32_t pinPot,ActorRef& bridge) :
    _adcLeftIS(ADC::create(pinLeftIS)),
    _adcRightIS(ADC::create(pinRightIS)),
    _pinLeftEnable(DigitalOut::create(pinLeftEnable)),
    _pinRightEnable(DigitalOut::create(pinRightEnable)),
    _pinPwmLeft(pinLeftPwm), _pinPwmRight(pinRightPwm),
    _adcPot(ADC::create(pinPot)),_bridge(bridge)
{
}

MotorServo::MotorServo(Connector* uext,ActorRef& bridge) : MotorServo(
        uext->toPin(LP_RXD),
        uext->toPin(LP_MISO),

        uext->toPin(LP_MOSI),
        uext->toPin(LP_CS),

        uext->toPin(LP_TXD),
        uext->toPin(LP_SCK),

        uext->toPin(LP_SCL),bridge)
{

}

MotorServo::~MotorServo()
{
}


void MotorServo::preStart()
{
    for(uint32_t i=0; i<SERVO_MAX_SAMPLES; i++)
        _angleSamples[i]=0;
    _adcLeftIS.init();
    _adcRightIS.init();
    _adcPot.init();
    _pinLeftEnable.init();
    _pinLeftEnable.write(0);
    _pinRightEnable.init();
    _pinRightEnable.write(0);
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
//    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTacho);
    INFO("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 2000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
//    pwm_config.mcpwm_cap0_in_num   = _pinTacho;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings

//   _bts7960.init();

}

Receive& MotorServo::createReceive()
{
    return receiveBuilder()

           .match(MsgClass::Properties(),
    [this](Msg& msg) {
        sender().tell(replyBuilder(msg)("angleTarget", _angleTarget)(
                          "angleMeasured", _angleCurrent)(
                          "angleFiltered", _angleFiltered)("currentLeft", _currentLeft)("currentRight", _currentRight)(
                          "KP", _KP)("KI", _KI)("KD", _KD),
                      self());
    })

    .match(MsgClass("keepGoing"),  [this](Msg& msg) {
        _watchdogCounter++;
    })

    .match(MsgClass("watchdogTimer"),  [this](Msg& msg) {
        if ( _watchdogCounter == 0 ) {
//            _rpmTarget=0;
        }
        _watchdogCounter=0;
    })

    .match(MsgClass("pulseTimer"),  [this](Msg& msg) {
    })



    .match(MsgClass("reportTimer"),
    [this](Msg& msg) {

    })

    .match(MsgClass("controlTimer"),
    [this](Msg& msg) {

    })

    .build();
}



void MotorServo::left(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);

    // call this each time, if operator
    // was previously in low/high state
}


void MotorServo::right(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);

    // call this each time, if operator
    // was previously in low/high state
}

float MotorServo::PID(float err, float interval)
{
    _integral = _integral + (err * interval);
    _derivative = (err - _errorPrior) / interval;
    float output = _KP * err + _KI * _integral + _KD * _derivative + _bias;
    _errorPrior = err;
    return output;
}


void MotorServo::setDirection(float output)
{
    if ( output < 0 && _directionTargetLast >= 0 ) {
        mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
        mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A,
                            MCPWM_DUTY_MODE_0);
        _directionTargetLast=-1;
    } else if ( output > 0 && _directionTargetLast <= 0 ) {
        mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
        mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B,
                            MCPWM_DUTY_MODE_0);
        _directionTargetLast=+1;
    }
}

void MotorServo::setOutput(float output)
{
    //    INFO(" output %f",output);
    static float lastOutput = 0;
    //    if ( abs(lastOutput-output)< 1) return;
    if ((output < 0 && lastOutput > 0) || (output > 0 && lastOutput < 0)) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    } else if (output < 0) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(-output);
    } else if (output > 0) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        right(output);
    } else {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    }
    lastOutput = output;
}



void MotorServo::round(float& f, float resolution)
{
    int i = f / resolution;
    f = i;
    f *= resolution;
}

float MotorServo::filterAngle(float f)
{
    float result;
    _angleSamples[(_indexSample++) % SERVO_MAX_SAMPLES]=f;
    result=0;
    for(uint32_t i=0; i<SERVO_MAX_SAMPLES; i++) {
        result += _angleSamples[i];
    }
    result /= SERVO_MAX_SAMPLES;
    return result;
}



/*

void MotorServo::run()
{
    static uint32_t loopCount=0;
    PT_BEGIN();
    _pinEnable.write(1);
    while(true) {
        PT_WAIT_SIGNAL(100);
                _pinEnable.write(0);
                PT_WAIT_SIGNAL(10);
        _pinEnable.write(1);
        _currentLeft = (_adcLeftIS.getValue()*3.9 / 1024.0)*0.85;
        _currentRight =  (_adcRightIS.getValue()*3.9 / 1024.0)*0.85;
        round(_currentLeft,0.1);
        round(_currentRight,0.1);
        _angleCurrent = (_adcPot.getValue()-1024/2);
        _angleCurrent *= 180.0/1024;
        _angleCurrent+=10;
        round(_angleCurrent,0.1);
        _angleFiltered = filterAngle(_angleCurrent);
        round(_angleFiltered,0.1);
        _iteration_time=0.1;
        _error = _angleTarget - _angleFiltered;
        if ( _error <1 && _error >-1) _error=0;

        if ( _currentLeft > 4 || _currentRight > 4 ) {
            if ( _output < 0 ) {
                setOutput(-10);
            } else  {
                setOutput(10);
            }
        } else {
            PID(_error);
            setOutput(_output);
        }


        INFO(" %f/%f/%f angle, %f/%f V,  %f sec, %f error, %f == P=%f + I=%f + D=%f",
             _angleCurrent,
             _angleFiltered,
             _angleTarget,
             _currentLeft,
             _currentRight,
             _iteration_time,
             _error,
             _output,
             _error*_KP,
             _integral*_KI,
             _derivative*_KD);

        loopCount++;
        if ( loopCount % 50 == 0 ) {
            _angleTarget = - _angleTarget;
        }


//        _bts7960.loop();
    }
    PT_END();
}*/
