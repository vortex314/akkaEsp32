#include "RotaryEncoder.h"
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

#define CAP0_INT_EN BIT(27) // Capture 0 interrupt bit
#define CAP1_INT_EN BIT(28) // Capture 1 interrupt bit
#define CAP2_INT_EN BIT(29) // Capture 2 interrupt bit
static mcpwm_dev_t* MCPWM[2] = {&MCPWM0, &MCPWM1};


RotaryEncoder::RotaryEncoder(uint32_t pinTachoA, uint32_t pinTachoB)
    : _pinTachoA(pinTachoA), _dInTachoB(DigitalIn::create(pinTachoB)) {
    _isrCounter = 0;
}

Erc RotaryEncoder::initialize() {
    _dInTachoB.init();
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTachoA);

    esp_err_t rc =
        mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_NEG_EDGE,
                             CAPTURE_DIVIDER); // capture signal on falling
                                               // edge, prescale = 0 i.e.
                                               // 80,000,000
    // counts is equal to one second
    // Enable interrupt on  CAP0, CAP1 and CAP2 signal
    MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN;
    // Set ISR Handler
    rc = mcpwm_isr_register(MCPWM_UNIT_0, isrHandler, this, ESP_INTR_FLAG_IRAM,
                            NULL);
    if (rc) {
        WARN("mcpwm_capture_enable() : %d", rc);
        return ENODEV;
    }
    return E_OK;
}

RotaryEncoder::~RotaryEncoder() {}

const uint32_t weight = 10;

void IRAM_ATTR
RotaryEncoder::isrHandler(void* pv) // ATTENTION no float calculations in ISR
{
    RotaryEncoder* ms = (RotaryEncoder*)pv;
    uint32_t mcpwm_intr_status;
    int b = ms->_dInTachoB.read(); // check encoder B when encoder A has isr,
                                   // indicates phase or rotation direction
    ms->_direction = (b == 1) ? (0 - ms->_directionSign) : ms->_directionSign;

    mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; // Read interrupt
    // status
    ms->_isrCounter++;
    if (mcpwm_intr_status &
        CAP0_INT_EN) { // Check for interrupt on rising edge on CAP0 signal
        ms->_prevCapture = ms->_capture;
        uint32_t capt = mcpwm_capture_signal_get_value(
            MCPWM_UNIT_0,
            MCPWM_SELECT_CAP0); // get capture signal counter value
        ms->_capture = capt;
        ms->_delta = (ms->_delta * (100 - weight) +
                      weight * (ms->_capture - ms->_prevCapture)) /
                     100;
    }

    MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

int32_t RotaryEncoder::rpm(){
	return deltaToRpm(_delta,_direction);
}

int32_t RotaryEncoder::direction(){
	return _direction;
}

int32_t RotaryEncoder::deltaToRpm(uint32_t delta, int32_t direction) {
    int rpm = 0;
    if (delta != _deltaPrev) {
        float t = (60.0 * CAPTURE_FREQ * CAPTURE_DIVIDER) /
                  (delta * PULSE_PER_ROTATION);
        _deltaPrev = delta;
        rpm = ((int32_t)t) * _direction;
    } else {
        _deltaPrev = delta;
    }
    return rpm;
}
