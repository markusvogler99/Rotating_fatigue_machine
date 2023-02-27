//Rotating Fatigue Machine 
//Copyright (c) 2023 Markus Vogler & Matthias Strasser - All Rights Reserved

//Libraries
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

//------------------------------------Define PINS-------------------------------------
// Define Pins for Display
// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
// Pins for Touchscreen
#define YP 24  // must be an analog pin, use "An" notation!
#define XM 27  // must be an analog pin, use "An" notation!
#define YM 26   // can be a digital pin
#define XP 25   // can be a digital pin
TouchScreen ts = TouchScreen(YP, XP, YM, XM, 300);

// Define Pins for PT100
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 Thermo_Motor = Adafruit_MAX31865(17, 18, 19, 20); //PT100 left
Adafruit_MAX31865 Thermo_Axial = Adafruit_MAX31865(34, 35, 36, 37); //PT100 right

// Define Pins for HX711 with bending force sensor
const int BEND_DOUT_PIN = 2;  //PIN 2 - Data
const int BEND_SCK_PIN = 3;   //PIN 3 - Clock 

// Define Pins for HX711 with axial force sensor
const int AX_DOUT_PIN = 6;    //PIN 6 - Data
const int AX_SCK_PIN = 31;    //PIN 31 - Clock 

// Define HX711 input channel
const int GAIN = 128; //Input Channel A - Gain 128

// Define Pin for speed sensor (revolutions)
const int SPEED_SENSOR = 5;   //PIN 5 - Input

// Define Pin to turn off motor
const int MOTOR_AUS = 14;   //PIN 14 - Output

// Define Pin to get motor state
const int MOTOR_EIN = 8;    //PIN 8 - Input


//---------------------------Define variables, settings and formulas--------------------------
// Define the file to be saved
File myfile; 
const int chipSelect = BUILTIN_SDCARD;

// Define PT100 settings
// Define Resistors on the MAX31865 for PT100
#define RREF      430.0 //Reference Resistor (430 = PT100)
#define RNOMINAL  100.0 //Nominal Resistor
// Temperature limit at which the motor turns off
double Temp_thresh = 70; //°C 

// Define bending force sensor settings
Force_Sensor bending_sensor; 
double reading_bend = 0;     
double reading_bend_ave = 0; 
// Calibration data
const double slope_bend = 0.0001110613;   //Slope of the calibration curve (old value 0.000111688) 
const double offset_bend = 1.3300184442;  //Offset of the calibration curve (old value 1.05)

// Define axial force sensor settings
Force_Sensor axial_sensor;
double reading_ax = 0;
// Calibration data
const double slope_ax =  -0.0052338633;     //Slope of the calibration curve       old values //1; //-0.0052470496 ;           //-0.0050729537; //Linear ermittelter Wert mit 0.9994 Linearität 
const double offset_ax =  2682.8411330541;  //Offset of the calibration curve      old values //508616.1325909440; //2625.4388645074;      //2610.8711302451;           //- 26496.3043898747; //Offset ermittelt

// Define speed (revolution) sensor settings
Speed_Sensor speed_sensor; 
double rpm_value = 0; 
int load_cycles = 0;  //define revolution counter

// Define the formulas and settings for stress calculation
double l1 = 110;              //mm  distance
double d = 6;                 //mm  Probe diameter
double pi = 3.141592;         //
double Wb = (pi*(d*d*d))/32;  //mm^3  Widerstandsmoment
double A = (d*d)*pi/4;        //mm^2  Area
double sigma_zd = 0;          //MPa  tension-pressure stress (Zug-Druck Spannung)
double sigma_mittel = 0;      //MPa  mean stress (Mittelspannung)

// Define variables
int flag = 0;               //not used
int file_flag = 0;          //for data save in use
int flag_load_cycles = 0;   //not used
int state = 0;              //control variable for state machine
bool button= false;         //Touchscreen button
bool motor_aus = false;     //Turn off motor
bool motor_ein = false;     //motor state (on or off)
double start_load = 0;      //auxiliary variables: force set at the start 
String state_name;        
String status; 
const char *  file_name;    //file name
const char * Version_count; //current number of attempts in string
int  file_name_2;           //not used
const char *  file_name_3;  //not used
int old_loadcycles = 0;     //auxiliary variables for file writing 
int file_counter = 1;       //number of attempts
double complete = 0;        //auxiliary variables for force breakdown  
int load_cycles_file = 0;   //auxiliary variables for file writing 
int counter = 0;            //auxiliary variables for force average  
double Temp_Motor;          //Temperature left (PT100)
double Temp_Axial;          //Temperature right (PT100)

//------------------------------Define Classes and Millis-------------------------------------
Display_MCI Display;
elapsedMillis time_force_sensors;
elapsedMillis time_SD_CARD;
elapsedMillis time_average;
elapsedMillis time_temp;
elapsedMillis time_write_data; 
elapsedMillis reset_display; 

//------------------------------------File writing------------------------------------------
// Define file name
String Versuch_init  = "Versuch "; //file name
String Versuch;
String txt = ".txt";  //file type
TSPoint p;
String file_counter_str;
const char* Versuch_count;  //file number

// Write Data to file
void count_write_load_cycles()
{
 load_cycles = speed_sensor.get_load_cycles(); //read revolutions
  
          if(time_write_data >= 1000)         //writes every second
        {
          myfile.printf("%d",load_cycles);    //write load cycles to file 
          myfile.printf(", %f",reading_bend); //write bending force to file
          myfile.printf(", %f \n",reading_ax);//write axial force to file and then new line
          old_loadcycles = load_cycles;       //not necessary 
          time_write_data = 0;                //reset time
        }    
        
}

// File name
void reset_file_name()
{
          Versuch = "Versuch ";              //file name
          myfile.close();                    //close the file
          file_counter = file_counter + 1;   //increase file number 
          file_flag = 0;                     //reset flag
}


//------------------------------------Void Setup-----------------------------------------
void setup() {
  Serial.begin(115200);

   pinMode(SPEED_SENSOR, INPUT_PULLDOWN); //Pinmode Revolution Sensor
   pinMode(MOTOR_AUS, OUTPUT);            //Pinmode Motor turn off
   pinMode(MOTOR_EIN, INPUT);             //Pinmode Motor state

   Thermo_Axial.begin(MAX31865_4WIRE);    //PT100 right - 4 wire
   Thermo_Motor.begin(MAX31865_4WIRE);    //PT100 left - 4 wire
    
   bending_sensor.init(BEND_DOUT_PIN,BEND_SCK_PIN,GAIN); //init bending force sensor
   axial_sensor.init(AX_DOUT_PIN,AX_SCK_PIN,GAIN);       //init axial force sensor
   Display.init_display();                               //init display
   speed_sensor.attach_interrupt(SPEED_SENSOR);          //attach interrupt revolutions sensor
}

//------------------------------------Void Loop-----------------------------------------
void loop(void) {

 // Reload display to counteract possible failure
  if (reset_display > 20000) //reload every 20 seconds
  {
    digitalWrite(9,LOW);
    digitalWrite(10,LOW);
    digitalWrite(11,LOW);
    digitalWrite(12,LOW);
    delay(10);

    Display.init_display(); 
    reset_display = 0; //reset time
  }

// Read Temperature of PT100
if (time_temp>5000)  //read temperature every 5 seconds
  {
 Temp_Motor = Thermo_Motor.temperature(RNOMINAL, RREF); //read Temp left
 Temp_Axial = Thermo_Axial.temperature(RNOMINAL, RREF); //read Temp right
 time_temp = 0;   //reset time
  }


//---------------------------------STATE MACHINE----------------------------------
switch (state)
{
  //------------------------------------IDLE-------------------------------------
    case 0:
        //Initial state (motor not running)
        state_name = "0 - IDLE";  //for display output
        if (SD.begin(chipSelect)) //check SD card
        {
            status = "Card initialized"; //successful 
        }
        else
        {
          status = "Card failed"; //fail
        }

               
       // Create new file name
       file_counter_str = String(file_counter); 
       Versuch = Versuch_init + file_counter_str + txt;
       Versuch_count = Versuch.c_str(); 
    
   
  //-------------------------------Transition---------------------------------------
        //IDLE to RUNNING
        if (digitalRead(MOTOR_EIN) == HIGH) //Motor turn on
          {
          state = 1; //to RUNNING
          myfile = SD.open(Versuch_count, FILE_WRITE); //open file
          myfile.printf("Lastzyklen, Biegekraft, Axialkraft \n"); //write header in file
          start_load = reading_bend;  //save starting force 
          }
    break; 


//------------------------------------RUNNING-------------------------------------
    case 1:
        state_name = "1 - RUNNING";
        count_write_load_cycles();  //Function for writing the file
        status = "Writing Data..."; //Status line
        

//------------------------------------Transition------------------------------------  
       if ((complete < 0.5*start_load)  || (Temp_Motor >= Temp_thresh) || (Temp_Axial >= Temp_thresh))
        {
          state = 2; //RUNNING to COMPETE if force drop or temperature to high
       }
         if ((digitalRead(MOTOR_EIN) == LOW))
        {
          state = 3; //RUNNING to HOLDING if motor state = off
       }
  
    break; 


//------------------------------------COMPLETE-------------------------------------
    case 2:
        state_name = "2 - COMPLETE"; 
        status = "SAVE & RESET"; //text in status line

        digitalWrite(MOTOR_AUS,HIGH); //turn off motor


//-----------------------------------Transition---------------------------------------
        if (button == true) //touchscreen button pressed
        {
          state = 0;                          //COMPETE to IDLE
          reset_file_name();                  //reset file name
          speed_sensor.reset_load_cycles();   //reset counter revolutions
          digitalWrite(MOTOR_AUS,LOW);        //reset motor turn off pin 
          load_cycles_file = 0;               //reset counter file
        }    
    break; 


//------------------------------------HOLDING------------------------------------------
    case 3:
        state_name = "3 - HOLDING";
        status = "Restart or SAVE";
        if (digitalRead(MOTOR_EIN) == HIGH)
        {
          state = 1;  //HOLDING to RUNNING if motor turn on 
        }
        else if (button == true) {
          state = 0;  //HOLDING to IDLE if touchscreen button pressed
          speed_sensor.reset_load_cycles(); //reset counter revolutions
          reset_file_name();                //reset file name
          button = false;                   //reset button
          load_cycles_file = 0;             //reset counter file
        }
    break; 
}
//---------------------------------END STATE MACHINE--------------------------------------------

//------------------------Reading Force Values Consistently-------------------------------------
  if (time_force_sensors>120)  //read every 0.12 seconds (>10 Hz)
  {
  reading_bend = bending_sensor.get_force_value(slope_bend, offset_bend); //read bending force
  sigma_zd = (reading_bend*l1)/(2*Wb);                //calculate tension-press stress
  reading_bend_ave = reading_bend_ave + reading_bend; //calculate average bending force
  counter = counter +1;                               //auxiliary variable counter

  if (counter == 20) //Bridges short failures of hx711, where force is briefly written to 0
  {
  complete = (reading_bend_ave/20); //calculate average over 20 bending force values - needed for abort criterion
  reading_bend_ave = 0;             //reset average
  counter = 0;                      //reset counter
  }

  reading_ax = axial_sensor.get_force_value(slope_ax, offset_ax); //read axial force
  sigma_mittel = (reading_ax)/(A);                                //calculate mean stress
  time_force_sensors = 0;                                         //reset time
  }
  
//------------------------------------Count Load cycles------------------------------------------

  load_cycles = speed_sensor.get_load_cycles();  //count revolutions (load cycles)

 
//------------------------------------Check Touch Screen Input-----------------------------------
  p = ts.getPoint();

  if (p.z > ts.pressureThreshhold and p.x < 500 and p.y > 760) //Pressure and coordinates of the touchbutton
    {
     button = true; 
    }
  else
    {
     button = false;
    }


//------------------------------------Refresh Display -------------------------------------------

 Display.draw_display(reading_bend, reading_ax,load_cycles,state_name,status,Temp_Motor,Temp_Axial,sigma_zd,sigma_mittel);

}


