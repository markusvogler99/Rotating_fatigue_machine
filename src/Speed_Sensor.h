#ifndef Speed_Sensor_h
#define Speed_Sensor_h

#include "string.h"
#include <Arduino.h>
#include "Tachometer.h"



class Speed_Sensor
{
private: 

    

public:

    void attach_interrupt(byte DIN);

     void reset_load_cycles();

    unsigned long get_rpm_value();

    int get_load_cycles();

    //void pin_ISR();

};




#endif