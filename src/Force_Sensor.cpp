
#include <Arduino.h>
#include "Force_Sensor.h"
#include "SPI.h"
#include "string.h"
#include "HX711.h"

HX711 Load_Cell;

double slope = 0; 
double offset = 0; 
double reading = 0;



void Force_Sensor::init(byte dout, byte pd_sck, byte gain = 128)
{
  Load_Cell.begin(dout,pd_sck,gain);
}



double Force_Sensor::get_force_value(double slope, double offset){


  if (Load_Cell.is_ready()) 
    {
      reading = Load_Cell.read()*slope + offset;
        if (reading <= 0)
          {
            reading = reading * (1);
          }
      return reading;
    }

  else 
    {
      return -5;
    }
  


}




