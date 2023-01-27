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
const double slope_ax = 0.0001; 
const double offset_ax = 1;

//Speed_Sensor
Speed_Sensor speed_sensor;
double rpm_value = 0;
int load_cycles = 0;

//Touch
bool button= false;
//bool motor_stop= false;
int flag = 0;
int flog = 0; 

//State Machine
int test = 0; 
int state = 0; 
bool motor_aus = false; 
bool motor_ein = false; 
double start_load = 0; 
String state_name; 
const char *  file_name;
int  file_name_2; 
const char *  file_name_3; 
int old_loadcycles = 0; 



Display_MCI Display;
elapsedMillis time_force_sensors;
elapsedMillis time_test_2;
TouchScreen ts = TouchScreen(YP, XP, YM, XM, 300);


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

switch (state)
{
  case 0:
  state_name = "0 - IDLE";
  load_cycles = 0;
  flag = 0; 
if (flog == 0){
   Serial.printf("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
Serial.println("card initialized.");



myfile = SD.open("Versuch 1", FILE_WRITE);

  flog = 1;
}

 
  if (digitalRead(MOTOR_EIN) == HIGH)
  {
    state = 1; 
  }
  break; 



  case 1:
  state_name = "1 - RUNNING";

  if (load_cycles != old_loadcycles){

  myfile.printf("%d",load_cycles); 
  myfile.printf(", %f",reading_bend);
  myfile.printf(", %f \n",reading_ax);
  Serial.printf("%d",load_cycles); 
  Serial.printf(", %f",reading_bend);
  Serial.printf(", %f \n",reading_ax);
  
  old_loadcycles = load_cycles; 
  }

  if (flag == 0)
  {
  start_load = reading_bend; 
  flag = 1; 
  }

  if (reading_bend < 0.5*start_load)
  {
    state = 2; 
  }
  else if (digitalRead(MOTOR_EIN) == LOW)
   {
    state = 3; 
  }
  
  break; 

  case 2:
  state_name = "2 - COMPLETE";
  digitalWrite(MOTOR_AUS,HIGH);
  if (button == true)
   {
    state = 0;
    myfile.close();
    flog = 0;
   }
  break; 

  case 3:
  state_name = "3 - HOLDING";
   if (digitalRead(MOTOR_EIN) == HIGH)
   {
    state = 1; 
   }
   else if (button == true) {
    state = 0;
    button = false; 
    myfile.close();
    flog = 0;
   }
  break; 
}


  if (time_force_sensors>100){
  reading_bend = bending_sensor.get_force_value(slope_bend, offset_bend);
  reading_ax = axial_sensor.get_force_value(slope_ax, offset_ax);
  time_force_sensors = 0; 
  }

  load_cycles = speed_sensor.get_load_cycles(); 
  rpm_value = speed_sensor.get_rpm_value();
 
 
  

  TSPoint p = ts.getPoint();
  
  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  
  if (p.z > ts.pressureThreshhold and p.x < 500 and p.y > 760) {
   button = true;
     //Serial.print("\tButton = "); Serial.println(button);
}
else
{
  button = false;
  //Serial.print("\tButton = "); 
  //Serial.println(state_name);
  }

  if (load_cycles != old_loadcycles){

  myfile.printf("%d",load_cycles); 
  myfile.printf(", %f",reading_bend);
  myfile.printf(", %f \n",reading_ax);
  Serial.printf("%d",load_cycles); 
  Serial.printf(", %f",reading_bend);
  Serial.printf(", %f \n",reading_ax);
  
  old_loadcycles = load_cycles; 
  }

 //Display.draw_tacho(rpm_value);

 Display.draw_display(reading_bend, reading_ax,rpm_value,load_cycles,state_name);

  


}