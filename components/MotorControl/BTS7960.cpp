#include "BTS7960.h"
#include <Log.h>

#define MAX_PWM 50



BTS7960::BTS7960(uint32_t pinLeftIS, uint32_t pinRightIS,
                 uint32_t pinLeftEnable, uint32_t pinRightEnable,
                 uint32_t pinLeftPwm, uint32_t pinRightPwm)
    : _adcLeftIS(ADC::create(pinLeftIS)), _adcRightIS(ADC::create(pinRightIS)),
      _pinLeftEnable(DigitalOut::create(pinLeftEnable)),
      _pinRightEnable(DigitalOut::create(pinRightEnable)),
      _pinPwmLeft(pinLeftPwm), _pinPwmRight(pinRightPwm) {}

BTS7960::BTS7960(Connector* uext)
    : BTS7960(uext->toPin(LP_RXD), uext->toPin(LP_MISO), uext->toPin(LP_MOSI),
              uext->toPin(LP_CS), uext->toPin(LP_TXD), uext->toPin(LP_SCK)
             )
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

void BTS7960::setDirection(float sign)
{
    if (sign < 0 && _directionTargetLast >= 0) {
        mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
        mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A,
                            MCPWM_DUTY_MODE_0);
        _directionTargetLast = -1;
    } else if (sign > 0 && _directionTargetLast <= 0) {
        mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
        mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B,
                            MCPWM_DUTY_MODE_0);
        _directionTargetLast = +1;
    }
}

void BTS7960::left(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
}

void BTS7960::right(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);
}

float weight=0.1;

void BTS7960::setPwm(float dutyCycle)
{
//  INFO(" dutyCycle %f",dutyCycle);
    static float lastDutyCycle = 0;
    if ( abs(lastDutyCycle-dutyCycle)< 1) return;
//   dutyCycle =  lastDutyCycle*(1-weight)+dutyCycle*weight;
    if ((dutyCycle < 0 && lastDutyCycle > 0) || (dutyCycle > 0 && lastDutyCycle < 0)) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    } else if (dutyCycle < 0) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(-dutyCycle);
    } else if (dutyCycle > 0) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        right(dutyCycle);
    } else {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    }
    lastDutyCycle = dutyCycle;
}

Erc BTS7960::initialize()
{
    _adcLeftIS.init();
    _adcRightIS.init();
    _pinLeftEnable.init();
    _pinLeftEnable.write(0);
    _pinRightEnable.init();
    _pinRightEnable.write(0);
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
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

    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
    return E_OK;
}

float absolute(float f)
{
    if (f > 0)
        return f;
    return -f;
}


BTS7960::~BTS7960() {}



void BTS7960::stop()
{
    //    INFO("%s",__func__);
    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
}

void BTS7960::round(float& f, float resolution)
{
    int i = f / resolution;
    f = i;
    f *= resolution;
}

void BTS7960::measureCurrent()
{
    _currentLeft = (_adcLeftIS.getValue() * 3.9 / 1024.0) * 0.85;
    _currentRight = (_adcRightIS.getValue() * 3.9 / 1024.0) * 0.85;

    round(_currentLeft, 0.1);
    round(_currentRight, 0.1);
}
