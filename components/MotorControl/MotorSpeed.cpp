#include "MotorSpeed.h"

#define CONTROL_INTERVAL_MS 1000

MsgClass MotorSpeed::TargetSpeed("targetSpeed");

MotorSpeed::MotorSpeed(uint32_t pinLeftIS, uint32_t pinRightIS,
                       uint32_t pinLeftEnable, uint32_t pinRightEnable,
                       uint32_t pinLeftPwm, uint32_t pinRightPwm,
                       uint32_t pinTachoA, uint32_t pinTachoB, ActorRef& bridge)
    : _bts7960(pinLeftIS, pinRightIS, pinLeftEnable, pinRightEnable, pinLeftPwm,
               pinRightPwm),
      _rotaryEncoder(pinTachoA, pinTachoB), _bridge(bridge)

{
    //_rpmMeasuredFilter = new AverageFilter<float>();
    _rpmTarget = 0;
    _watchdogCounter = 0;
    _directionTargetLast = 0;
    _rotaryEncoder.setPwmUnit(0);
    _bts7960.setPwmUnit(0);
}

MotorSpeed::MotorSpeed(Connector* uext, ActorRef& bridge)
    : MotorSpeed(uext->toPin(LP_RXD), uext->toPin(LP_MISO),
                 uext->toPin(LP_MOSI), uext->toPin(LP_CS), uext->toPin(LP_TXD),
                 uext->toPin(LP_SCK), uext->toPin(LP_SCL), uext->toPin(LP_SDA),
                 bridge) {}

MotorSpeed::~MotorSpeed() {}

void MotorSpeed::preStart()
{
    if (initialize() == 0)
        run();
    else
        hold();

    timers().startPeriodicTimer("controlTimer", Msg("controlTimer"),
                                CONTROL_INTERVAL_MS);
    timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 100);
    timers().startPeriodicTimer("watchdogTimer", Msg("watchdogTimer"), 2000);
    timers().startPeriodicTimer("pulseTimer", Msg("pulseTimer"), 5000);
}

Receive& MotorSpeed::createReceive()
{
    return receiveBuilder()

           .match(MsgClass::Properties(),
    [this](Msg& msg) {
        sender().tell(replyBuilder(msg)//
                      ("rpmTarget", _rpmTarget) //
                      ("rpmMeasured", _rotaryEncoder.rpm())//
                      ("direction", _rotaryEncoder.direction()) //
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

    .match(MsgClass("pulseTimer"),
    [this](Msg& msg) {
 //       _bts7960.showReg();

        static uint32_t pulse = 0;
        static int rpmTargets[] = {0,  30, 50,  100, 150, 100, 80,
                                   40, 0,  -40, -80,-120,-80 -30
                                  };
        _rpmTarget = rpmTargets[pulse];
        pulse++;
        pulse %= (sizeof(rpmTargets) / sizeof(int));
        INFO("rpmTarget : %d rpmMeasured : %d;",  _rpmTarget,
             _rotaryEncoder.rpm());
        _bridge.tell(
            msgBuilder(Bridge::Publish)("rpmTarget", _rpmTarget)(
                "rpmMeasured", _rotaryEncoder.rpm()),
            self());
    })

    .match(TargetSpeed,
    [this](Msg& msg) {
        double target;
        INFO(" targetSpeed message ");
        if (msg.get("value", target)) {
            INFO(" targetSpeed : %f", target);
            _rpmTarget = target * 40;
            sender().tell(replyBuilder(msg)("rc", 0), self());
            _bridge.tell(
                msgBuilder(Bridge::Publish)("rpmTarget", _rpmTarget),
                self());
        }
    })

    .match(MsgClass("reportTimer"),
    [this](Msg& msg) {
        _currentLeft = _bts7960.measureCurrentLeft();
        _currentRight = _bts7960.measureCurrentRight();
        Msg& m = msgBuilder(Bridge::Publish);
        m("rpmMeasured", _rotaryEncoder.rpm());
        m("direction", _rotaryEncoder.direction());
        m("rpmTarget", _rpmTarget);
        m("PWM", _output);
        m("running", isRunning());
        m("currentLeft",_currentLeft);
        m("currentRight",_currentRight);
        _bridge.tell(m, self());
    })

    .match(
        MsgClass("controlTimer"),
    [this](Msg& msg) {
        if (isOnHold()) {
            _bts7960.setOutput(0.0);
            return;
        } else {
            static float newOutput;
            int rpmMeasured;

            rpmMeasured = _rotaryEncoder.rpm();
            _error = _rpmTarget - rpmMeasured;
            newOutput = PID(_error, CONTROL_INTERVAL_MS);
            if (_rpmTarget == 0) {
                newOutput = 0;
                _integral=0;
            }
            _output = newOutput;
            _bts7960.setOutput(20);
//            _bts7960.setPwm(_rpmTarget/2);

            /*         INFO("PID %3d/%3d rpm err:%5.2f pwm:%-5.2f == P:%-5.2f + I:%-5.2f + "
                             "D:%+5.2f  %2.2f/%2.2f A, ",
                             rpmMeasured, _rpmTarget, _error, _output, _error * _KP,
                             _integral * _KI, _derivative * _KD);*/
        }
    })

    .build();
}

float MotorSpeed::PID(float err, float interval)
{
    _integral = _integral + (err * interval);
    _derivative = (err - _errorPrior) / interval;
    float output = _KP * err + _KI * _integral + _KD * _derivative + _bias;
    _errorPrior = err;
    return output;
}



// Generic Component commands

Erc MotorSpeed::selfTest(uint32_t level, std::string& message)
{
    return 0;
}

Erc MotorSpeed::initialize()
{
    Erc rc = _rotaryEncoder.initialize();
    if ( rc != E_OK ) return rc;
    return _bts7960.initialize();
}

Erc MotorSpeed::hold()
{
    state(ST_ON_HOLD);
    return 0;
}

Erc MotorSpeed::run()
{
    state(ST_RUNNING);
    return 0;
}
