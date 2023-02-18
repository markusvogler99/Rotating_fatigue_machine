#ifndef Display_MCI_h
#define Display_MCI_h

#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>

class Display_MCI

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10




{
private: 


public:

    //Speed_Sensor();
    //virtual ~Speed_Sensor();

    void init_display();
    void draw_display(double reading_bend, double reading_ax,int test,String state,String status,double Temp_Motor, double Temp_Axial,double sigma_zd, double sigma_mittel);
    void draw_tacho(int rpm); 

};




#endif