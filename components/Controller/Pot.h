#ifndef POT_H
#define POT_H
#include <Hardware.h>

class Pot
{
    ADC& _adc;
    float _value;
public:
    Pot(PhysicalPin p);
    ~Pot();
    void init();
    float getValue();

};

#endif // POT_H
