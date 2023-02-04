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

File myfile; 
const int chipSelect = BUILTIN_SDCARD;

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define YP 24  // must be an analog pin, use "An" notation!
#define XM 27  // must be an analog pin, use "An" notation!
#define YM 26   // can be a digital pin
#define XP 25   // can be a digital pin

//Pins definieren für die Hx711, an denen die Kraftsensoren hängen 
const int BEND_DOUT_PIN = 2;
const int BEND_SCK_PIN = 3;

const int AX_DOUT_PIN = 6;
const int AX_SCK_PIN = 20;

const int GAIN = 128; 

//Pin für Speed Sensor 

const int SPEED_SENSOR = 5;

// Pin für MOTOR_AUS
const int MOTOR_AUS = 14;
//const int TEST = 14;
const int MOTOR_EIN = 8;


//Force_Sensor bending
Force_Sensor bending_sensor;
double reading_bend = 0;
double reading_bend_ave = 0; 
const double slope_bend = 0.000111688; 
const double offset_bend = 1.05;

//Force_Sensor axial
Force_Sensor axial_sensor;
double reading_ax = 0;
const double slope_ax =   -0.0054034732; //-0.0052470496 ;           //-0.0050729537; //Linear ermittelter Wert mit 0.9994 Linearität 
const double offset_ax =  2625.2; //2625.4388645074;      //2610.8711302451;           //- 26496.3043898747; //Offset ermittelt

//Speed_Sensor
Speed_Sensor speed_sensor;
double rpm_value = 0;
int load_cycles = 0;

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

int counter = 0; 

//------------------------------------Define Classes-------------------------------------
Display_MCI Display;
elapsedMillis time_force_sensors;
elapsedMillis time_SD_CARD;
elapsedMillis time_average;
TouchScreen ts = TouchScreen(YP, XP, YM, XM, 300);
String Versuch_init  = "Versuch ";
String Versuch;
String txt = ".txt";
TSPoint p;
String file_counter_str;
const char* Versuch_count;

void count_write_load_cycles()
{
 load_cycles = speed_sensor.get_load_cycles();
 
        if(load_cycles != old_loadcycles)
        {
          myfile.printf("%d",load_cycles); 
          myfile.printf(", %f",reading_bend);
          myfile.printf(", %f \n",reading_ax);
          
          old_loadcycles = load_cycles; 
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
  Serial.begin(9600);


   pinMode(SPEED_SENSOR, INPUT_PULLDOWN); 
   pinMode(MOTOR_AUS, OUTPUT);
  
   //pinMode(TEST, OUTPUT);
   // delay(1000);
    //digitalWrite(TEST,HIGH);
    pinMode(MOTOR_EIN, INPUT);
   

   bending_sensor.init(BEND_DOUT_PIN,BEND_SCK_PIN,GAIN);
   axial_sensor.init(AX_DOUT_PIN,AX_SCK_PIN,GAIN);
   Display.init_display();
   speed_sensor.attach_interrupt(SPEED_SENSOR);
    
}


void loop(void) {



//----------------------------------------------------------STATE MACHINE---------------
switch (state)
{
    case 0:
        state_name = "0 - IDLE";
        speed_sensor.reset_load_cycles();
        load_cycles = 0; 
      
        if (SD.begin(chipSelect))
        {
            //Serial.println("Card initialized.");
            status = "Card initialized";
        }
        else
        {
          status = "Card failed";
        }

        flag_load_cycles = 1; 
               
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
        //if (reading_bend < 0.5*start_load)
       // {
        //  state = 2; 
       // }
         if (digitalRead(MOTOR_EIN) == LOW)
        {
          state = 3; 
        }
  
    break; 
//------------------------------------COMPLETE-------------------------------------
    case 2:
        state_name = "2 - COMPLETE";
        count_write_load_cycles();
        status = "SAVE & RESET";

        digitalWrite(MOTOR_AUS,HIGH);
  //--------------Transition------------------------
        if (button == true)
        {
          state = 0; 
          reset_file_name();
          speed_sensor.reset_load_cycles();
          digitalWrite(MOTOR_AUS,LOW);
        }    
    break; 
//------------------------------------HOLDING------------------------------------
    case 3:
        state_name = "3 - HOLDING";
        count_write_load_cycles();
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
        }
    break; 
}

//------------------------------------Reading Force Values Consistently-------------------------------------
  if (time_force_sensors>100)  
  {
  reading_bend = bending_sensor.get_force_value(slope_bend, offset_bend);
  reading_bend_ave = reading_bend_ave + reading_bend;
  Serial.printf("Load cycles: %f \n  ",load_cycles);
  counter = counter +1;
  if (counter > 10)
  {
reading_bend_ave = (reading_bend_ave/10);
//Serial.printf("Average over 10: %f \n  ",reading_bend_ave);
reading_bend_ave = 0; 

counter = 0; 
  }
  reading_ax = axial_sensor.get_force_value(slope_ax, offset_ax);
  
  
  time_force_sensors = 0; 
  }

 // if (time_average > 1000)
  //{
   // Serial.printf("Threshold: %f ",start_load/2); 
  //time_average = 0; 
  //}

  


//------------------------------------Count Load cycles-------------------------------------

  //load_cycles = speed_sensor.get_load_cycles(); 
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

 Display.draw_display(reading_bend, reading_ax,rpm_value,load_cycles,state_name,status);

}


