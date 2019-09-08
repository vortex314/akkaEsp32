#include "MotorServo.h"

#define MAX_PWM 50
#define CONTROL_INTERVAL_MS 100
#define ANGLE_MIN -45.0
#define ANGLE_MAX 45.0
#define ADC_MIN 330
#define ADC_MAX 620
#define ADC_ZERO 460

MsgClass MotorServo::targetAngle("targetAngle");


MotorServo::MotorServo(uint32_t pinPot, uint32_t pinIS,
                       uint32_t pinLeftEnable, uint32_t pinRightEnable,
                       uint32_t pinLeftPwm, uint32_t pinRightPwm,
                       ActorRef& bridge) : _bts7960(pinIS, pinIS, pinLeftEnable, pinRightEnable, pinLeftPwm,
                                   pinRightPwm),
    _adcPot(ADC::create(pinPot)),
    _bridge(bridge)
{
    _bts7960.setPwmUnit(1);
}

MotorServo::MotorServo(Connector* uext,ActorRef& bridge) : MotorServo(
        uext->toPin(LP_RXD), //only ADC capable pins
        uext->toPin(LP_MISO), // "

        uext->toPin(LP_MOSI),
        uext->toPin(LP_CS),

        uext->toPin(LP_TXD),
        uext->toPin(LP_SCK),bridge)
{

}

MotorServo::~MotorServo()
{
}


void MotorServo::preStart()
{
    for(uint32_t i=0; i<SERVO_MAX_SAMPLES; i++)
        _angleSamples[i]=0;
    if (initialize() == 0) {
        run();
        _bts7960.setMaxPwm(90);
    } else {
        hold();
    }
    timers().startPeriodicTimer("controlTimer", Msg("controlTimer"), CONTROL_INTERVAL_MS);
    timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 100);
    timers().startPeriodicTimer("pulseTimer", Msg("pulseTimer"), 10000);
    timers().startPeriodicTimer("watchdogTimer", Msg("watchdogTimer"), 2000);
}

float MotorServo::scale(float x,float x1,float x2,float y1,float y2)
{
    if ( x < x1 ) x=x1;
    if ( x > x2 ) x=x2;
    float y= y1+(( x-x1)/(x2-x1))*(y2-y1);
    return y;
}

bool MotorServo::measureAngle()
{
    int adc = _adcPot.getValue();
    _potFilter.addSample(adc);
    if ( _potFilter.isReady()) {
        _angleMeasured = scale(_potFilter.getMedian(),ADC_MIN,ADC_MAX,ANGLE_MIN,ANGLE_MAX);
        return true;
    }
    return false;
}

Receive& MotorServo::createReceive()
{
    return receiveBuilder()

           .match(MsgClass::Properties(),
    [this](Msg& msg) {
        sender().tell(replyBuilder(msg) //
                      ("angleTarget", _angleTarget)
                      ("angleMeasured", _angleMeasured) //
                      ("current", _current) //
                      ("output", _output) //
                      ("KP", _KP)("KI", _KI)("KD", _KD),
                      self());
    })

    .match(MsgClass("keepGoing"), [this](Msg& msg) {
        bool keepGoing=false;
        if ( msg.get("value",keepGoing) && keepGoing ) {
            _watchdogCounter++;
            INFO(" >> RXD keepGoing ");
        }
    })

    .match(MsgClass("watchdogTimer"),
    [this](Msg& msg) {
        if (_watchdogCounter == 0) {
//           hold();
        } else {
            run();
        }
        _watchdogCounter = 0;
    })

    .match(MsgClass("pulseTimer"),  [this](Msg& msg) {
        static uint32_t pulse=0;
        static int outputTargets[]= {-30,0,30,0};
        _angleTarget=outputTargets[pulse];
        pulse++;
        pulse %= (sizeof(outputTargets)/sizeof(int));

        _bridge.tell(msgBuilder(Bridge::Publish)("angleMeasured",_angleMeasured)("angleTarget",_angleTarget),self());
    })

    .match(MsgClass("reportTimer"),
    [this](Msg& msg) {
        measureAngle();
        Msg& m = msgBuilder(Bridge::Publish);
        m("adcPot",_adcPot.getValue());
        m("adcFilter",_potFilter.getMedian());
        m("angleMeasured",_angleMeasured);
        m("angleTarget",_angleTarget)        ;
        m("running", isRunning());
        _bridge.tell(m,self());
    })

    .match(MsgClass("controlTimer"),
    [this](Msg& msg) {
        if ( measureAngle()) {
            _error = _angleTarget - _angleMeasured;
            _output = PID(_error, CONTROL_INTERVAL_MS);
            _bts7960.setPwm(_output);
/*            INFO("PID  %3.1f/%3.1f angle err:%3.1f pwm:%5f == P:%5f + I:%5f + D:%5f  ",
                 _angleMeasured, _angleTarget,
                 _error, _output, _error * _KP,
                 _integral * _KI, _derivative * _KD);*/
        }
    })

    .match(MotorServo::targetAngle,  [this](Msg& msg) {
        double target;
        if ( msg.get("data",target)) {
            INFO(" targetAngle : %f",target);
            _angleTarget=target;
            if ( _angleTarget< ANGLE_MIN) _angleTarget=ANGLE_MIN;
            if ( _angleTarget> ANGLE_MAX) _angleTarget=ANGLE_MAX;
            sender().tell(replyBuilder(msg)("rc",0),self());
            _bridge.tell(msgBuilder(Bridge::Publish)("angleTarget",_angleTarget),self());
        }
    })

    .build();
}

float MotorServo::PID(float err, float interval)
{
    _integral = _integral + (err * interval);
    _derivative = (err - _errorPrior) / interval;
    float output = _KP * err + _KI * _integral + _KD * _derivative + _bias;
    _errorPrior = err;
    return output;
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

Erc MotorServo::initialize()
{
    Erc rc = _adcPot.init();
    if ( rc != E_OK ) return rc;
    return _bts7960.initialize();
}

Erc MotorServo::hold()
{
    state(ST_ON_HOLD);
    return 0;
}

Erc MotorServo::run()
{
    state(ST_RUNNING);
    return 0;
}

Erc MotorServo::selfTest(uint32_t level, std::string& message)
{
    return 0;
}
