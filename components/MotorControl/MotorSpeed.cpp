#include "MotorSpeed.h"
#include "soc/rtc.h"

#define MAX_PWM 35
/*
 * CAPTURE :
 * Rotary sensor generates 400 pulses per rotation
 * Capture generates an interrupt per 10 pulses
 * The capture counter works at 80 M pulses per second
 *
 * 10 rpm => 6000 pulses per min  => 600 isr per min => 10 isr per sec
 *
 *  delta_time = (capture / 80M)*10 => time in sec for 1 rotation
 *
 * rpm = (1/delta_time)*60
 *
 * rpm = (80M/capture)/10*60 = ( 80M/capture )*6
 *
 *
 *
 * */

#define CAPTURE_FREQ 80000000
#define PULSE_PER_ROTATION 400
#define CAPTURE_DIVIDER 1
#define CONTROL_INTERVAL_MS 20

static mcpwm_dev_t* MCPWM[2] = {&MCPWM0, &MCPWM1};
MsgClass MotorSpeed::targetSpeed("targetSpeed");

MotorSpeed::MotorSpeed( uint32_t pinLeftIS,
                        uint32_t pinRightIS,
                        uint32_t pinLeftEnable,
                        uint32_t pinRightEnable,
                        uint32_t pinLeftPwm,
                        uint32_t pinRightPwm,
                        uint32_t pinTachoA,
                        uint32_t pinTachoB,
                        ActorRef& bridge)
    : _adcLeftIS(ADC::create(pinLeftIS)),
      _adcRightIS(ADC::create(pinRightIS)),
      _pinLeftEnable(DigitalOut::create(pinLeftEnable)),
      _pinRightEnable(DigitalOut::create(pinRightEnable)),
      _pinPwmLeft(pinLeftPwm), _pinPwmRight(pinRightPwm),
      _pinTachoA(pinTachoA),
      _dInTachoB(DigitalIn::create(pinTachoB)),_bridge(bridge)
{
    _isrCounter = 0;
    _rpmMeasuredFilter = new AverageFilter<float>();
    _rpmTarget = 0;
    _watchdogCounter=0;
}

MotorSpeed::MotorSpeed(Connector* uext,ActorRef& bridge)
    : MotorSpeed(uext->toPin(LP_RXD),
                 uext->toPin(LP_MISO),

                 uext->toPin(LP_MOSI),
                 uext->toPin(LP_CS),

                 uext->toPin(LP_TXD),
                 uext->toPin(LP_SCK),

                 uext->toPin(LP_SCL),
                 uext->toPin(LP_SDA),bridge)
{
    INFO(" Drive/Sensor = UEXT GPIO ");
    INFO("         L_IS = %s GPIO_%d ", Connector::uextPin(LP_RXD),
         uext->toPin(LP_SCL));
    INFO("         R_IS = %s GPIO_%d ", Connector::uextPin(LP_MISO),
         uext->toPin(LP_MISO));
    INFO("         L_EN = %s GPIO_%d ", Connector::uextPin(LP_MOSI),
         uext->toPin(LP_MOSI));
    INFO("         R_EN = %s GPIO_%d ", Connector::uextPin(LP_CS),
         uext->toPin(LP_CS));
    INFO("        L_PWM = %s GPIO_%d ", Connector::uextPin(LP_TXD),
         uext->toPin(LP_TXD));
    INFO("        R_PWM = %s GPIO_%d ", Connector::uextPin(LP_SCK),
         uext->toPin(LP_SCK));
    INFO(" Tacho Chan A = %s GPIO_%d ", Connector::uextPin(LP_SCL),
         uext->toPin(LP_RXD));
    INFO(" Tacho Chan B = %s GPIO_%d ", Connector::uextPin(LP_SDA),
         uext->toPin(LP_SDA));
}

MotorSpeed::~MotorSpeed() {}

void MotorSpeed::preStart()
{
    for (uint32_t i = 0; i < MAX_SAMPLES; i++)
        _samples[i] = 0;
    _adcLeftIS.init();
    _adcRightIS.init();
    _pinLeftEnable.init();
    _pinLeftEnable.write(0);
    _pinRightEnable.init();
    _pinRightEnable.write(0);
    _dInTachoB.init();
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTachoA);
    //    _dInTachoB.onChange(DigitalIn::DIN_RAISE,onRaise,this);
    INFO("Configuring Initial Parameters of mcpwm... on %d , %d ", _pinPwmLeft,
         _pinPwmRight);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 10000; // frequency = 500Hz,
    pwm_config.cmpr_a = 0;        // duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;        // duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0,
               &pwm_config); // Configure PWM0A & PWM0B with above settings
    //   _bts7960.init();
    esp_err_t rc = mcpwm_capture_enable(
                       MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_NEG_EDGE,
                       CAPTURE_DIVIDER); // capture signal on falling edge, prescale = 0 i.e. 80,000,000
    // counts is equal to one second
    MCPWM[MCPWM_UNIT_0]->int_ena.val =
        CAP0_INT_EN; // Enable interrupt on  CAP0, CAP1 and CAP2 signal
    rc = mcpwm_isr_register(MCPWM_UNIT_0, isrHandler, this, ESP_INTR_FLAG_IRAM,
                            NULL); // Set ISR Handler
    if (rc) {
        WARN("mcpwm_capture_enable() : %d", rc);
    }

    timers().startPeriodicTimer("controlTimer", Msg("controlTimer"), CONTROL_INTERVAL_MS);
    timers().startPeriodicTimer("reportTimer", Msg("reportTimer"), 500);
    timers().startPeriodicTimer("watchdogTimer", Msg("watchdogTimer"), 2000);

    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
}

int32_t MotorSpeed::deltaToRpm(uint32_t delta,int32_t direction)
{
    int rpm=0;
    if ( delta != _deltaPrev ) {
        float t = ( 60.0 * CAPTURE_FREQ * CAPTURE_DIVIDER)/(delta*PULSE_PER_ROTATION);
        _deltaPrev=delta;
        rpm =  ((int32_t)t)*_direction;
    } else {
        _deltaPrev=delta;
    }
    return rpm;
}

Receive& MotorSpeed::createReceive()
{
    return receiveBuilder()

           .match(MsgClass::Properties(),
    [this](Msg& msg) {
        sender().tell(replyBuilder(msg)("rpmTarget", _rpmTarget)(
                          "rpmMeasured", _rpmMeasured)(
                          "rpmFiltered", _rpmFiltered)(
                          "delta", _delta)("direction", _direction)(
                          "KP", _KP)("KI", _KI)("KD", _KD),
                      self());
    })

    .match(MsgClass("keepGoing"),  [this](Msg& msg) {
        _watchdogCounter++;
    })

    .match(MsgClass("watchdogTimer"),  [this](Msg& msg) {
        if ( _watchdogCounter == 0 ) {
            _rpmTarget=0;
        }
        _watchdogCounter=0;
    })

    .match(targetSpeed,  [this](Msg& msg) {
        double target;
        INFO(" targetSpeed message ");
        if ( msg.get("data",target)) {
            INFO(" targetSpeed : %f",target);
            _rpmTarget=target*40;
            sender().tell(replyBuilder(msg)("rc",0),self());
            _bridge.tell(msgBuilder(Bridge::Publish)("rpmTarget",_rpmTarget),self());
        }
    })

    .match(MsgClass("reportTimer"),
    [this](Msg& msg) {
        Msg& m = msgBuilder(Bridge::Publish)("leftCurrent",_currentLeft)("rightCurrent",_currentRight);
        if ( _direction != _directionPrev) {
            m("direction",_direction);
            _directionPrev = _direction;
        }
        /*       _sample_time = _delta;
               _sample_time /= APB_CLK_FREQ;*/
        m("rpmMeasured",_rpmMeasured);
        m("delta",_delta);
        _bridge.tell(m,self());
    })

    .match(MsgClass("controlTimer"),
    [this](Msg& msg) {
        static uint32_t loopCount=0;
        static float newOutput;
        static float w =0.5;
        if ( _delta ) _rpmMeasured =  deltaToRpm(_delta,_direction);
        measureCurrent();
        _error = _rpmTarget - _rpmMeasured;
        newOutput = PID(_error, CONTROL_INTERVAL_MS/1000.0);
        if ( _rpmTarget ==0 ) newOutput=0;
        newOutput = newOutput*_pwmSign;
        _output =  ( 1-w) * _output + w * newOutput;
        setOutput(_output);

        if ( loopCount++ % 10 ==0 )
            INFO("PID %3d/%3d rpm err:%3.1f pwm: %5f == P:%5f + I:%5f + D:%5f  %2.2f/%2.2f A, ",
                 _rpmMeasured, _rpmTarget,
                 _error, _output, _error * _KP,
                 _integral * _KI, _derivative * _KD,_currentLeft, _currentRight);
    })

    .build();
}
const uint32_t weight=10;
void IRAM_ATTR MotorSpeed::isrHandler(void* pv) // ATTENTION no float calculations in ISR
{
    MotorSpeed* ms = (MotorSpeed*)pv;
    uint32_t mcpwm_intr_status;
    int b = ms->_dInTachoB.read();          // check encoder B when encoder A has isr, indicates phase or rotation direction
    ms->_direction = (b == 1) ? (0- ms->_directionSign) : ms->_directionSign;

    mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; // Read interrupt
    // status
    ms->_isrCounter++;
    if (mcpwm_intr_status & CAP0_INT_EN) { // Check for interrupt on rising edge on CAP0 signal
        ms->_prevCapture = ms->_capture;
        uint32_t capt = mcpwm_capture_signal_get_value(
                            MCPWM_UNIT_0,
                            MCPWM_SELECT_CAP0); // get capture signal counter value
        ms->_capture = capt;
        ms->_delta = (ms->_delta*(100-weight)+weight*(ms->_capture - ms->_prevCapture))/100;
    }

    MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}


void MotorSpeed::measureCurrent()
{
    _currentLeft = (_adcLeftIS.getValue() * 3.9 / 1024.0) * 0.85;
    _currentRight =
        (_adcRightIS.getValue() * 3.9 / 1024.0) * 0.85;

    round(_currentLeft, 0.1);
    round(_currentRight, 0.1);
}
/*
void MotorSpeed::onRaise(void* pv)
{
    MotorSpeed* ms = (MotorSpeed*)pv;
    if (gpio_get_level((gpio_num_t)ms->_pinTachoA)) {
        ms->_direction = 1;
    } else {
        ms->_direction = -1;
    }
}
*/

void MotorSpeed::left(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A,
                        MCPWM_DUTY_MODE_0);
    // call this each time, if operator
    // was previously in low/high state
}


void MotorSpeed::right(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B,
                        MCPWM_DUTY_MODE_0);
    // call this each time, if operator
    // was previously in low/high state
}

float MotorSpeed::PID(float err, float interval)
{
    _integral = _integral + (err * interval);
    _derivative = (err - _errorPrior) / interval;
    float output = _KP * err + _KI * _integral + _KD * _derivative + _bias;
    _errorPrior = err;
    return output;
}

void MotorSpeed::setOutput(float output)
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

float MotorSpeed::filter(float f)
{
    float result;
    _samples[(_indexSample++) % MAX_SAMPLES] = f;
    result = 0;
    for (uint32_t i = 0; i < MAX_SAMPLES; i++) {
        result += _samples[i];
    }
    result /= MAX_SAMPLES;
    return result;
}

void MotorSpeed::round(float& f, float resolution)
{
    int i = f / resolution;
    f = i;
    f *= resolution;
}
