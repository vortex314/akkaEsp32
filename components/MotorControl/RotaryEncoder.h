#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H
#include <Hardware.h>
#include <Log.h>
#include "driver/mcpwm.h"
#include "driver/pcnt.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "soc/rtc.h"

class RotaryEncoder {
    uint32_t _pinTachoA;
    DigitalIn& _dInTachoB;
    uint32_t _pinTachoB;

    uint32_t _capture;
    uint32_t _prevCapture;
    uint32_t _delta;
    uint32_t _deltaPrev;
    uint32_t _captureInterval;
    uint32_t _isrCounter;
    int _direction = 1;
    int _directionPrev = 1;
    int _directionSign = -1;

  public:
    RotaryEncoder(uint32_t pinTachoA, uint32_t pinTachoB);
    ~RotaryEncoder();
    Erc initialize();
    static void onRaise(void*);
    static void isrHandler(void*);
    int32_t deltaToRpm(uint32_t delta, int32_t direction);

    int32_t rpm();
	int32_t direction();
};

#endif // ROTARYENCODER_H
