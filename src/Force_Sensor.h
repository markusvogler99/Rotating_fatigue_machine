

#ifndef Force_Sensor_h
#include "HX711.h"
#include "string.h"
#define Force_Sensor_h


class Force_Sensor


{
private: 
   HX711 Load_Cell; 
public:

    void init(byte dout, byte pd_sck, byte gain = 128);

    double get_force_value(double slope, double offset);

};

#endif