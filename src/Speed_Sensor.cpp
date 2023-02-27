#include "Speed_Sensor.h"

//define variables
 volatile int count_value = 0;
    int rpms = 0;
    Tachometer tacho;
    int oldtime = 0; 
    int rpm = 0; 
    unsigned long tmr= 0;
    int time = 0;
    int temp = 0; 
    int old_count_value = 0;

   unsigned long RPM_T1 = 0;                                       // Setzen der Zeitvariable T1

    unsigned long RPM_T2 = 0;                                       // Setzen der Zeitvariable T2

    unsigned long RPM_Count = 0;    
    unsigned long RPM = 0;

    int end=0;  
    int start=0;  
    int duration=0;  
    
    


    elapsedMillis timer; 
 
//Interrupt service routine 
void pin_ISR() {
      RPM_Count++;  //count load cycle
}

//interrupt on rising edge
void Speed_Sensor::attach_interrupt(byte DIN) {
  attachInterrupt(DIN, pin_ISR,RISING);
}

//reset load cycles
void Speed_Sensor::reset_load_cycles() {
  RPM_Count = 0;
}

//not used
 unsigned long  Speed_Sensor::get_rpm_value()
{
  rpm= (duration/1000)*60;
  return rpm; 

    }

 //return load cycles
int Speed_Sensor::get_load_cycles()
{
  return RPM_Count;            
}




