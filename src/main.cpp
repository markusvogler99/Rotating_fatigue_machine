#include <SD.h>
#include <Arduino.h>
#include <SPI.h>
#include "HX711.h"
#include "Force_Sensor.h"
#include "Speed_Sensor.h"
#include "string.h"
#include "Display_MCI.h"
#include "elapsedMillis.h"
#include "Tachometer.h"
#include "TouchScreen.h"
#include <Adafruit_MAX31865.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 Thermo_Motor = Adafruit_MAX31865(17, 18, 19, 20); 
Adafruit_MAX31865 Thermo_Axial = Adafruit_MAX31865(34, 35, 36, 37);

File myfile; 
const int chipSelect = BUILTIN_SDCARD;

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define YP 24  // must be an analog pin, use "An" notation!
#define XM 27  // must be an analog pin, use "An" notation!
#define YM 26   // can be a digital pin
#define XP 25   // can be a digital pin

#define RREF      430.0
#define RNOMINAL  100.0

//Pins definieren für die Hx711, an denen die Kraftsensoren hängen 
const int BEND_DOUT_PIN = 2;
const int BEND_SCK_PIN = 3;

const int AX_DOUT_PIN = 6;
const int AX_SCK_PIN = 31;

const int GAIN = 128; 

//Pin für Speed Sensor 

const int SPEED_SENSOR = 5;

// Pin für MOTOR_AUS
const int MOTOR_AUS = 14;
//const int TEST = 14;
const int MOTOR_EIN = 8;

// Temperaturgrenze 

double Temp_thresh = 70; 


//Force_Sensor bending
Force_Sensor bending_sensor;
double reading_bend = 0;
double reading_bend_ave = 0; 
const double slope_bend = 0.0001110613; //0.000111688; 
const double offset_bend = 1.3300184442;//1.05;

//Force_Sensor axial
Force_Sensor axial_sensor;
double reading_ax = 0;
const double slope_ax =  -0.0052338633   ;//1; //-0.0052470496 ;           //-0.0050729537; //Linear ermittelter Wert mit 0.9994 Linearität 
const double offset_ax =  2682.8411330541;//508616.1325909440; //2625.4388645074;      //2610.8711302451;           //- 26496.3043898747; //Offset ermittelt

//Speed_Sensor
Speed_Sensor speed_sensor;
double rpm_value = 0;
int load_cycles = 0;

//Calculate Stresses
double l1 = 110; //mm
double d = 6; //mm 
double pi = 3.141592; 
double Wb = (pi*(d*d*d))/32; //mm^3
double A = (d*d)*pi/4; //mm^2
double sigma_zd = 0; 
double sigma_mittel = 0; 



//Touch
bool button= false;
//bool motor_stop= false;
int flag = 0;
int file_flag = 0;
int flag_load_cycles = 0; 

//State Machine
//int test = 0; 
int state = 0; 
bool motor_aus = false; 
bool motor_ein = false; 
double start_load = 0; 
String state_name; 
String status; 
const char *  file_name;
const char * Version_count;
int  file_name_2; 
const char *  file_name_3; 
int old_loadcycles = 0; 
int file_counter = 1; 
double complete = 0; 
int load_cycles_file = 0; 

int counter = 0; 

//------------------------------------Define Classes-------------------------------------
Display_MCI Display;
elapsedMillis time_force_sensors;
elapsedMillis time_SD_CARD;
elapsedMillis time_average;
elapsedMillis time_temp;
elapsedMillis time_write_data; 
elapsedMillis reset_display; 
TouchScreen ts = TouchScreen(YP, XP, YM, XM, 300);
String Versuch_init  = "Versuch ";
String Versuch;
String txt = ".txt";
double Temp_Motor; 
double Temp_Axial; 
TSPoint p;
String file_counter_str;
const char* Versuch_count;

void count_write_load_cycles()
{
 load_cycles = speed_sensor.get_load_cycles();
 
        if(time_write_data >= 1000)
        {
          //load_cycles_file = load_cycles_file +1;
          myfile.printf("%d",load_cycles); 
          myfile.printf(", %f",reading_bend);
          myfile.printf(", %f \n",reading_ax);
          old_loadcycles = load_cycles;   
          time_write_data = 0; 
        }    
        
}

void reset_file_name()
{
          Versuch = "Versuch ";
          myfile.close();
          file_counter = file_counter + 1; 
          file_flag = 0;
}

void setup() {
  Serial.begin(115200);


   pinMode(SPEED_SENSOR, INPUT_PULLDOWN); 
   pinMode(MOTOR_AUS, OUTPUT);

   Thermo_Axial.begin(MAX31865_4WIRE);
   Thermo_Motor.begin(MAX31865_4WIRE);
  
   
    pinMode(MOTOR_EIN, INPUT);
   

   bending_sensor.init(BEND_DOUT_PIN,BEND_SCK_PIN,GAIN);
   axial_sensor.init(AX_DOUT_PIN,AX_SCK_PIN,GAIN);
   Display.init_display();
   speed_sensor.attach_interrupt(SPEED_SENSOR);
    
}


void loop(void) {
  

 if (time_temp>5000)  
  {
 Temp_Motor = Thermo_Motor.temperature(RNOMINAL, RREF);
 Temp_Axial = Thermo_Axial.temperature(RNOMINAL, RREF);
 time_temp = 0; 
  }


//Reinitialize Display and setting all outputs to low  (Helps with the reset)
  if (reset_display > 20000)
  {
    digitalWrite(9,LOW);
    digitalWrite(10,LOW);
    digitalWrite(11,LOW);
    digitalWrite(12,LOW);
    delay(10);

    Display.init_display(); 
    reset_display = 0; 
  }



//----------------------------------------------------------STATE MACHINE---------------
switch (state)
{
    case 0:
        state_name = "0 - IDLE";
        
      
        if (SD.begin(chipSelect))
        {
            status = "Card initialized";
        }
        else
        {
          status = "Card failed";
        }

               
//Create File Name

    file_counter_str = String(file_counter);
    Versuch = Versuch_init + file_counter_str + txt;
    Versuch_count = Versuch.c_str(); 
    
   

  //--------------Transition------------------------

        if (digitalRead(MOTOR_EIN) == HIGH)
          {
          state = 1; 
          myfile = SD.open(Versuch_count, FILE_WRITE);
          myfile.printf("Lastzyklen, Biegekraft, Axialkraft \n");
          start_load = reading_bend; 
          }
    break; 


//------------------------------------RUNNING-------------------------------------
    case 1:
        state_name = "1 - RUNNING";
       
        count_write_load_cycles();
        status = "Writing Data...";
        

//--------------Transition------------------------
       if ((complete < 0.5*start_load)  || (Temp_Motor >= Temp_thresh) || (Temp_Axial >= Temp_thresh))
        {
          state = 2; 
       }
         if ((digitalRead(MOTOR_EIN) == LOW))
        {
          state = 3; 
       }
  
    break; 
//------------------------------------COMPLETE-------------------------------------
    case 2:
        state_name = "2 - COMPLETE";
        //count_write_load_cycles();
        status = "SAVE & RESET";

        digitalWrite(MOTOR_AUS,HIGH);
  //--------------Transition------------------------
        if (button == true)
        {
          state = 0; 
          reset_file_name();
          speed_sensor.reset_load_cycles();
          digitalWrite(MOTOR_AUS,LOW);
          load_cycles_file = 0; 
        }    
    break; 
//------------------------------------HOLDING------------------------------------
    case 3:
        state_name = "3 - HOLDING";
        //count_write_load_cycles();
        status = "Restart or SAVE";
        if (digitalRead(MOTOR_EIN) == HIGH)
        {
          state = 1; 
        }
        else if (button == true) {
          state = 0;
          speed_sensor.reset_load_cycles();
          reset_file_name();
          button = false; 
          load_cycles_file = 0; 
        }
    break; 
}

//------------------------------------Reading Force Values Consistently-------------------------------------
  if (time_force_sensors>120)  
  {
  reading_bend = bending_sensor.get_force_value(slope_bend, offset_bend);
  sigma_zd = (reading_bend*l1)/(2*Wb);
  reading_bend_ave = reading_bend_ave + reading_bend;
  counter = counter +1;

  if (counter == 20)
  {
     
  complete = (reading_bend_ave/20);
  Serial.printf("Average over 20: %f \n  ",complete); 
  reading_bend_ave = 0;

counter = 0; 
  }

  reading_ax = axial_sensor.get_force_value(slope_ax, offset_ax);
  sigma_mittel = (reading_ax)/(A);
  
  
  time_force_sensors = 0; 
  }

 // if (time_average > 1000)
  //{
   // Serial.printf("Threshold: %f ",start_load/2); 
  //time_average = 0; 
  //}

  


//------------------------------------Count Load cycles-------------------------------------

  load_cycles = speed_sensor.get_load_cycles(); 
  //rpm_value = speed_sensor.get_rpm_value();
 
//------------------------------------Check Touch Screen Input-------------------------------------
  p = ts.getPoint();
  
  if (p.z > ts.pressureThreshhold and p.x < 500 and p.y > 760) 
    {
     button = true;
    }
  else
    {
     button = false;
    }


//------------------------------------Refresh Display -------------------------------------




 Display.draw_display(reading_bend, reading_ax,load_cycles,state_name,status,Temp_Motor,Temp_Axial,sigma_zd,sigma_mittel);
  // Serial.printf("Load cycles: %d \n  ",rpm_value);
}


