#include "BTS7960.h"
#include <Log.h>
#include <Register.h>

static Register reg_prescaler("PWM_CLK_CFG_REG",
                              "- - - - - - - - - - - - - - - - - - - - - - - - + + + + + + + PWM_CLK_PRESCALE");
static Register reg_fault_detect("PWM_FAULT_DETECT_REG",
                                 "- - - - - - - - - - - - - - - - - - - - - - - PWM_EVENT_F2 PWM_EVENT_F1 PWM_EVENT_F0 PWM_F2_POLE PWM_F1_POLE PWM_F0_POLE PWM_F2_EN PWM_F1_EN PWM_F0_EN");
static Register timer0_status_reg("PWM_TIMER0_STATUS_REG",
                                  "- - - - - - - - - - - - - - - PWM_TIMER0_DIRECTION + + + + + + + + + + + + + + + PWM_TIMER0_VALUE");
static Register pwm_int_raw_pwm_reg("INT_RAW_PWM_REG","- - INT_CAP2_INT_RAW INT_CAP1_INT_RAW INT_CAP0_INT_RAW INT_FH2_OST_INT_RAW INT_FH1_OST_INT_RAW INT_FH0_OST_INT_RAW"
                                    " INT_FH2_CBC_INT_RAW INT_FH1_CBC_INT_RAW INT_FH0_CBC_INT_RAW INT_OP2_TEB_INT_RAW INT_OP1_TEB_INT_RAW INT_OP0_TEB_INT_RAW INT_OP2_TEA_INT_RAW"
                                    " INT_OP1_TEA_INT_RAW INT_OP0_TEA_INT_RAW INT_FAULT2_CLR_INT_RAW INT_FAULT1_CLR_INT_RAW INT_FAULT0_CLR_INT_RAW INT_FAULT2_INT_RAW INT_FAULT1_INT_RAW"
                                    " INT_FAULT0_INT_RAW INT_TIMER2_TEP_INT_RAW INT_TIMER1_TEP_INT_RAW INT_TIMER0_TEP_INT_RAW INT_TIMER2_TEZ_INT_RAW"
                                    " INT_TIMER1_TEZ_INT_RAW INT_TIMER0_TEZ_INT_RAW INT_TIMER2_STOP_INT_RAW INT_TIMER1_STOP_INT_RAW INT_TIMER0_STOP_INT_RAW");
static Register pwm_int_ena_pwm_reg("INT_ENA_PWM_REG","- - INT_CAP2_INT_ENA INT_CAP1_INT_ENA INT_CAP0_INT_ENA INT_FH2_OST_INT_ENA INT_FH1_OST_INT_ENA INT_FH0_OST_INT_ENA"
                                    " INT_FH2_CBC_INT_ENA INT_FH1_CBC_INT_ENA INT_FH0_CBC_INT_ENA INT_OP2_TEB_INT_ENA INT_OP1_TEB_INT_ENA INT_OP0_TEB_INT_ENA INT_OP2_TEA_INT_ENA"
                                    " INT_OP1_TEA_INT_ENA INT_OP0_TEA_INT_ENA INT_FAULT2_CLR_INT_ENA INT_FAULT1_CLR_INT_ENA INT_FAULT0_CLR_INT_ENA INT_FAULT2_INT_ENA INT_FAULT1_INT_ENA"
                                    " INT_FAULT0_INT_ENA INT_TIMER2_TEP_INT_ENA INT_TIMER1_TEP_INT_ENA INT_TIMER0_TEP_INT_ENA INT_TIMER2_TEZ_INT_ENA"
                                    " INT_TIMER1_TEZ_INT_ENA INT_TIMER0_TEZ_INT_ENA INT_TIMER2_STOP_INT_ENA INT_TIMER1_STOP_INT_ENA INT_TIMER0_STOP_INT_ENA");
void BTS7960::showReg()
{
    uint32_t idx=_mcpwm_num;
    INFO(" MCPWM[%d]",idx);
    reg_prescaler.value(*(uint32_t*)MCPWM_CLK_CFG_REG(idx));
    reg_prescaler.show();
    reg_fault_detect.value(*(uint32_t*)MCPWM_FAULT_DETECT_REG(idx));
    reg_fault_detect.show();
    timer0_status_reg.value(*(uint32_t*)MCPWM_TIMER0_STATUS_REG(idx));
    timer0_status_reg.show();
    /*   pwm_int_raw_pwm_reg.value(*(uint32_t*)MCMCPWM_INT_RAW_MCPWM_REG(idx));
       pwm_int_raw_pwm_reg.show();*/
    pwm_int_ena_pwm_reg.value(*(uint32_t*)MCMCPWM_INT_ENA_MCPWM_REG(idx));
    pwm_int_ena_pwm_reg.show();
}


BTS7960::BTS7960(uint32_t pinLeftIS, uint32_t pinRightIS,
                 uint32_t pinLeftEnable, uint32_t pinRightEnable,
                 uint32_t pinLeftPwm, uint32_t pinRightPwm)
    : _adcLeftIS(ADC::create(pinLeftIS)), _adcRightIS(ADC::create(pinRightIS)),
      _pinLeftEnable(DigitalOut::create(pinLeftEnable)),
      _pinRightEnable(DigitalOut::create(pinRightEnable)),
      _pinPwmLeft(pinLeftPwm), _pinPwmRight(pinRightPwm)
{
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
}

BTS7960::BTS7960(Connector* uext)
    : BTS7960(uext->toPin(LP_RXD), uext->toPin(LP_MISO), uext->toPin(LP_MOSI),
              uext->toPin(LP_CS), uext->toPin(LP_TXD), uext->toPin(LP_SCK)
             )
{
    /*   INFO(" Drive/Sensor = UEXT GPIO ");
       INFO("         L_IS = %s GPIO_%d ", Connector::uextPin(LP_RXD),
            uext->toPin(LP_RXD));
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
            uext->toPin(LP_SCL));
       INFO(" Tacho Chan B = %s GPIO_%d ", Connector::uextPin(LP_SDA),
            uext->toPin(LP_SDA));*/
}

void BTS7960::setDirection(float sign)
{
    if (sign < 0 && _directionTargetLast >= 0) {
        _rc = mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
        if ( _rc != E_OK ) {
            WARN("mcpwm_set_signal_low()=%d",_rc);
        }
        _rc = mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A,
                                  MCPWM_DUTY_MODE_0);
        if ( _rc != E_OK ) {
            WARN("mcpwm_set_duty_type()=%d",_rc);
        }
        _directionTargetLast = -1;
    } else if (sign > 0 && _directionTargetLast <= 0) {
        _rc = mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
        if ( _rc != E_OK ) {
            WARN("mcpwm_set_signal_low()=%d",_rc);
        }
        _rc = mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B,
                                  MCPWM_DUTY_MODE_0);
        if ( _rc != E_OK ) {
            WARN("mcpwm_set_duty_type()=%d",_rc);
        }
        _directionTargetLast = +1;
    }
}

void BTS7960::left(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    if ( duty_cycle < 0 ) duty_cycle=0;
    _rc = mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
    if ( _rc != E_OK ) {
        WARN("mcpwm_set_duty_type()=%d",_rc);
    }
}

void BTS7960::right(float duty_cycle)
{
    if (duty_cycle > MAX_PWM)
        duty_cycle = MAX_PWM;
    if ( duty_cycle < 0 ) duty_cycle=0;

    _rc= mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);
    if ( _rc != E_OK ) {
        WARN("mcpwm_set_duty_type()=%d",_rc);
    }
    /*    float dc = mcpwm_get_duty(_mcpwm_num, _timer_num,MCPWM_OPR_B);
        INFO(" set/get duty cycle %5.1f/%5.1f",duty_cycle,dc);*/
}

float weight=0.1;

void BTS7960::setOutput(float dutyCycle)
{
    INFO("MCPWM[%d] PWM=%f",_mcpwm_num,dutyCycle);

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

void BTS7960::setPwmUnit(uint32_t unit)
{
    if ( unit==0 ) {
        _mcpwm_num = MCPWM_UNIT_0;
    } else if ( unit ==1 ) {
        _mcpwm_num = MCPWM_UNIT_1;
    }
}

Erc BTS7960::initialize()
{
    _adcLeftIS.init();
    _adcRightIS.init();
    _pinLeftEnable.init();
    _pinLeftEnable.write(0);
    _pinRightEnable.init();
    _pinRightEnable.write(0);

    INFO(" BTS7960 PWM[%d] PWM : GPIO_%d,%d enable : GPIO_%d,%d ",_mcpwm_num,
         _pinPwmLeft,_pinPwmRight,
         _pinLeftEnable.getPin(),_pinRightEnable.getPin());


    _rc = mcpwm_gpio_init(_mcpwm_num, MCPWM0A, _pinPwmLeft);
    if ( _rc != ESP_OK ) {
        WARN("mcpwm_gpio_init()=%d",_rc);
        return EIO;
    }
    _rc=mcpwm_gpio_init(_mcpwm_num, MCPWM0B, _pinPwmRight);
    if ( _rc != ESP_OK ) {
        WARN("mcpwm_gpio_init()=%d",_rc);
        return EIO;
    };
    mcpwm_config_t pwm_config;
    BZERO(pwm_config);
    pwm_config.frequency = 1000; // frequency = 1000Hz,
    pwm_config.cmpr_a = 0;        // duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;        // duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    _rc= mcpwm_init(_mcpwm_num, _timer_num,&pwm_config);
    if(_rc != ESP_OK) {
        WARN("mcpwm_init()=%d",_rc);
        return EIO;
    }
    // Configure PWM0A & PWM0B with above settings
    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
//    showReg();
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

float BTS7960::measureCurrentLeft()
{
    _currentLeft = (_adcLeftIS.getValue() * 3.9 / 1024.0) * 0.85;

    round(_currentLeft, 0.1);
    return _currentLeft;
}

float BTS7960::measureCurrentRight()
{
    _currentRight = (_adcRightIS.getValue() * 3.9 / 1024.0) * 0.85;
    round(_currentRight, 0.1);
    return _currentRight;
}

void BTS7960::setMaxPwm(uint32_t max)
{
    MAX_PWM=max;
}
