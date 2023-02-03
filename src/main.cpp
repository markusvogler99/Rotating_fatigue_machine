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
const int MOTOR_EIN = 8;


//Force_Sensor bending
Force_Sensor bending_sensor;
double reading_bend = 0;
const double slope_bend = 0.000111688; 
const double offset_bend = 1.05;

//Force_Sensor axial
Force_Sensor axial_sensor;
double reading_ax = 0;
const double slope_ax =  -0.0050729537; //Linear ermittelter Wert mit 0.9994 Linearität 
const double offset_ax = - 26496.3043898747; //Offset ermittelt

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
int test = 0; 
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

int test_variable = 0; 

//------------------------------------Define Classes-------------------------------------
Display_MCI Display;
elapsedMillis time_force_sensors;
elapsedMillis time_SD_CARD;
TouchScreen ts = TouchScreen(YP, XP, YM, XM, 300);
String Versuch_init  = "Versuch ";
String Versuch;
String txt = ".txt";
TSPoint p;
String file_counter_str;
const char* Versuch_count;


void setup() {
  Serial.begin(9600);


   pinMode(SPEED_SENSOR, INPUT_PULLDOWN); 
   pinMode(MOTOR_AUS, OUTPUT);
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
        flag = 0; 

      
          //Serial.printf("Initializing SD card...");

        
      
        if (SD.begin(chipSelect))
        {
            Serial.println("Card initialized.");
            status = "Card initialized";
            //test_variable = 1;
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
    myfile = SD.open(Versuch_count, FILE_WRITE);
   

  //--------------Transition------------------------

        if (digitalRead(MOTOR_EIN) == HIGH)
          {
          state = 1; 
          //load_cycles = 0; 
          myfile.printf("Lastzyklen, Biegekraft, Axialkraft \n");
          }
    break; 


//------------------------------------RUNNING-------------------------------------
    case 1:
        state_name = "1 - RUNNING";
       
         load_cycles = speed_sensor.get_load_cycles();
        if(load_cycles != old_loadcycles)
        {
          myfile.printf("%d",load_cycles); 
          myfile.printf(", %f",reading_bend);
          myfile.printf(", %f \n",reading_ax);  
          old_loadcycles = load_cycles; 
        }

        if (flag == 0)
        {
          start_load = reading_bend; 
          flag = 1; 
        }
//--------------Transition------------------------
        if (reading_bend < 0.5*start_load)
        {
          state = 2; 
        }
        else if (digitalRead(MOTOR_EIN) == LOW)
        {
          state = 3; 
        }
  
    break; 
//------------------------------------COMPLETE-------------------------------------
    case 2:
        state_name = "2 - COMPLETE";
        load_cycles = speed_sensor.get_load_cycles();
        if(load_cycles != old_loadcycles)
        {
          myfile.printf("%d",load_cycles); 
          myfile.printf(", %f",reading_bend);
          myfile.printf(", %f \n",reading_ax);  
          old_loadcycles = load_cycles; 
        }
        digitalWrite(MOTOR_AUS,HIGH);
  //--------------Transition------------------------
        if (button == true)
        {
          state = 0;
          //load_cycles = 0; 
          Versuch = "Versuch ";
          myfile.close();
          file_counter = file_counter + 1; 
          file_flag = 0;
          digitalWrite(MOTOR_AUS,LOW);
        }    
    break; 
//------------------------------------HOLDING------------------------------------
    case 3:
        state_name = "3 - HOLDING";
        load_cycles = speed_sensor.get_load_cycles();
        if(load_cycles != old_loadcycles)
        {
          myfile.printf("%d",load_cycles); 
          myfile.printf(", %f",reading_bend);
          myfile.printf(", %f \n",reading_ax);  
          old_loadcycles = load_cycles; 
        }
    //--------------Transition------------------------
        if (digitalRead(MOTOR_EIN) == HIGH)
        {
          state = 1; 
          test_variable = 0; 
        }
        else if (button == true) {
          state = 0;
          //load_cycles = 0; 
          Versuch = "Versuch ";
          button = false; 
          myfile.close();
          file_counter = file_counter + 1; 
          file_flag = 0;
        }
    break; 
}

//------------------------------------Reading Force Values Consistently-------------------------------------
  if (time_force_sensors>100){
  reading_bend = bending_sensor.get_force_value(slope_bend, offset_bend);
  reading_ax = 0; //axial_sensor.get_force_value(slope_ax, offset_ax);
  time_force_sensors = 0; 
  }


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

  /*if (load_cycles != old_loadcycles){

  myfile.printf("%d",load_cycles); 
  myfile.printf(", %f",reading_bend);
  myfile.printf(", %f \n",reading_ax);
  Serial.printf("%d",load_cycles); 
  Serial.printf(", %f",reading_bend);
  Serial.printf(", %f \n",reading_ax);
  
  old_loadcycles = load_cycles; 
  }
  */
//------------------------------------Refresh Display -------------------------------------

 Display.draw_display(reading_bend, reading_ax,rpm_value,load_cycles,state_name,status);

}