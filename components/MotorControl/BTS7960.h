#ifndef BTS7960_H
#define BTS7960_H
#include <Hardware.h>

class BTS7960
{
    DigitalOut& _left;
    DigitalOut& _right;
    DigitalOut& _enable;

    ADC& _current;
    ADC& _position;
    int _min,_max,_zero;
    int _angleTarget;
    int _angleCurrent;
public:
    BTS7960(Connector& conn);
    BTS7960(DigitalOut& left,DigitalOut& right,DigitalOut& enable,ADC& current,ADC& position);
    ~BTS7960();
    void init();
    void loop();
    void calcTarget(float v);
    int32_t getAngle();
    int32_t getAngleCurrent();
    void setAngleTarget(int32_t target);
    int32_t getAngleTarget();
    void motorStop();
    void motorLeft();
    void motorRight();
};

#endif // BTS7960_H
