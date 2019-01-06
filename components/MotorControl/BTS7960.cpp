#include "BTS7960.h"
#include <Log.h>

BTS7960::BTS7960(DigitalOut& left,DigitalOut& right,DigitalOut& enable,ADC& current,ADC& position) :
    _left(left),_right(right),_enable(enable),_current(current),_position(position)
{

}
BTS7960::BTS7960(Connector& connector) :
    _left(connector.getDigitalOut(LP_SCL)),
    _right(connector.getDigitalOut(LP_SDA)),
    _enable(connector.getDigitalOut(LP_TXD)),
    _current(connector.getADC(LP_SCK)),
    _position(connector.getADC(LP_RXD))
{

}

void BTS7960::init()
{
    _left.init();
    _right.init();
    _enable.init();
    _position.init();
    _current.init();
    
    _left.write(0);
    _right.write(0);
    _enable.write(0);
    
    _max=75;
    _min=-75;
    _angleTarget=0;

}

float absolute(float f)
{
    if (f > 0) return f;
    return -f;
}

int32_t BTS7960::getAngle()
{
//    INFO(" ADC %d ",_position.getValue());
    return ((_position.getValue() *180 )/1024)-90;
}

void BTS7960::setAngleTarget(int32_t target){
    _angleTarget = target;
}

int32_t BTS7960::getAngleTarget(){
    return _angleTarget;
}

int32_t BTS7960::getAngleCurrent(){
    return _angleCurrent;
}


BTS7960::~BTS7960()
{
}

void BTS7960::motorLeft()
{
//    INFO("%s",__func__);
    _left.write(1);
    _right.write(0);
    _enable.write(1);
}

void BTS7960::motorRight()
{
//    INFO("%s",__func__);
    _left.write(0);
    _right.write(1);
    _enable.write(1);
}

void BTS7960::motorStop()
{
//    INFO("%s",__func__);
    _left.write(1);
    _right.write(1);
    _enable.write(1);
}

void BTS7960::loop()
{
    _angleCurrent=getAngle();
//    INFO("angle %d target %d",_angleCurrent,_angleTarget);
    int delta = abs(_angleCurrent - _angleTarget);
    if (delta < 2) {
        motorStop();
    } else if (_angleCurrent < _angleTarget) {
        motorRight();
    } else if (_angleCurrent > _angleTarget) {
        motorLeft();
    }
}
