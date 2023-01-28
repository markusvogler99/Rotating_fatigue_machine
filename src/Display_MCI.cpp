#include "Display_MCI.h"



Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

double temp_bend =0;
double temp_ax = 0;
int temp_rpm_value = 0; 
int temp_loadcycles = 0;
int temp_indicator = 0; 
String temp_status = 0; 
String temp_state_name; 

elapsedMillis time_display;
elapsedMillis time_display_tacho;

const int TEXT_SIZE_SMALL = 1;
  const int TEXT_SIZE_LARGE = 2;
  const int ONE_K = 1000;

  const uint16_t INDICATOR_WIDTH = 5;

  const int PHOTODIODE_PIN_2 = 2;

 const uint16_t DIAL_CENTER_X = 180;
  const uint16_t DIAL_RADIUS = 100;
  const uint16_t DIAL_CENTER_Y = 235;

  const uint16_t INDICATOR_LENGTH = DIAL_RADIUS - 5;

  const long MAJOR_TICKS[] = { 0, 1000, 2000, 3000,4000, 5000 };
  const int MAJOR_TICK_COUNT = sizeof(MAJOR_TICKS) / sizeof(MAJOR_TICKS[0]);
  const int  MAJOR_TICK_LENGTH = 7;
  const long MINOR_TICKS[] = {500, 1500, 2500, 3500, 4500};
  const int MINOR_TICK_COUNT = sizeof(MINOR_TICKS) / sizeof(MINOR_TICKS[0]);
  const int MINOR_TICK_LENGTH = 3;
  const uint16_t LABEL_RADIUS = DIAL_RADIUS - 18;
  
  const int HALF_CIRCLE_DEGREES = 180;
  const float PI_RADIANS = PI/HALF_CIRCLE_DEGREES;

  const uint16_t DIAL_MAX_RPM = MAJOR_TICKS[MAJOR_TICK_COUNT-1];

  const int DIAL_LABEL_Y_OFFSET = 6;
  const int DIAL_LABEL_X_OFFSET = 4;

   const int POS_BIEGEKRAFT_X= 5;
   const int POS_BIEGEKRAFT_Y= 5;

   const int POS_AXIALKRAFT_X= 5;
   const int POS_AXIALKRAFT_Y= 25;

   const int POS_LASTZYKLEN_X = 5;
   const int POS_LASTZYKLEN_Y = 45;

   const int POS_STATE_X = 5;
   const int POS_STATE_Y = 65;

   const int POS_STATUS_X = 5;
   const int POS_STATUS_Y = 230;

   const int POS_UPDATE_VALUES_X = 140;


  float getPercentMaxRpm(long value) {
	float ret_value = (value * 1.0)/(DIAL_MAX_RPM * 1.0);
	return ret_value;
  };
  float getCircleXWithLengthAndAngle(uint16_t radius, float angle) {
	return DIAL_CENTER_X + radius * cos(angle*PI_RADIANS);
};

float getCircleYWithLengthAndAngle(uint16_t radius, float angle) {
	return DIAL_CENTER_Y + radius * sin(angle*PI_RADIANS);
};



  void drawTicks(const long ticks[], int tick_count, int tick_length) {
  for (int tick_index = 0; tick_index < tick_count; tick_index++) {
		long rpm_tick_value = ticks[tick_index];
		float tick_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t tick_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		uint16_t tick_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		tft.drawLine(dial_x, dial_y, tick_x, tick_y, ILI9341_WHITE);
	}
}
 
 void drawTickMarks() {
  drawTicks(MAJOR_TICKS, MAJOR_TICK_COUNT, MAJOR_TICK_LENGTH);
  drawTicks(MINOR_TICKS, MINOR_TICK_COUNT, MINOR_TICK_LENGTH);
}

void drawMajorTickLabels() {
	tft.setTextSize(TEXT_SIZE_SMALL);
	for (int label_index = 0; label_index < MAJOR_TICK_COUNT; label_index++) {
    tft.setTextColor(ILI9341_ORANGE);
		long rpm_tick_value = MAJOR_TICKS[label_index];
		float tick_angle = (HALF_CIRCLE_DEGREES	* getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		tft.setCursor(dial_x - DIAL_LABEL_X_OFFSET, dial_y - DIAL_LABEL_Y_OFFSET);
		int label_value = rpm_tick_value / ONE_K;
		tft.print(label_value);
	}
}

void drawIndicatorHand(long rpm_value) {
    float indicator_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_value)) + HALF_CIRCLE_DEGREES;
    uint16_t indicator_top_x = getCircleXWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);
    uint16_t indicator_top_y = getCircleYWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);

	tft.drawTriangle(DIAL_CENTER_X - INDICATOR_WIDTH / PHOTODIODE_PIN_2,
	                     DIAL_CENTER_Y,DIAL_CENTER_X + INDICATOR_WIDTH / PHOTODIODE_PIN_2,
	                     DIAL_CENTER_Y,
	                     indicator_top_x, 
	                     indicator_top_y, 
	                     ILI9341_WHITE);
}

void clearIndicatorHand(long rpm_value) {
    float indicator_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_value)) + HALF_CIRCLE_DEGREES;
    uint16_t indicator_top_x = getCircleXWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);
    uint16_t indicator_top_y = getCircleYWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);

	tft.drawTriangle(DIAL_CENTER_X - INDICATOR_WIDTH / PHOTODIODE_PIN_2,
	                     DIAL_CENTER_Y,DIAL_CENTER_X + INDICATOR_WIDTH / PHOTODIODE_PIN_2,
	                     DIAL_CENTER_Y,
	                     indicator_top_x, 
	                     indicator_top_y, 
	                     ILI9341_BLACK);
}

void Display_MCI::init_display()
{
tft.begin();
tft.fillScreen(ILI9341_BLACK);
 tft.setRotation(3);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(POS_BIEGEKRAFT_X,POS_BIEGEKRAFT_Y);
  tft.print("Biegekraft: ");
  
  tft.setCursor(POS_AXIALKRAFT_X,POS_AXIALKRAFT_Y);
  tft.print("Axialkraft: ");

  //tft.setCursor(5, 45);
 // tft.print("Drehzahl: ");
  
  tft.setCursor(POS_LASTZYKLEN_X, POS_LASTZYKLEN_Y);
  tft.print("Lastzyklen: ");

  tft.setCursor(POS_STATE_X, POS_STATE_Y);
  tft.print("State: ");
  
  tft.setTextSize(1);
  tft.setCursor(POS_STATUS_X, POS_STATUS_Y);
  tft.print("Status: ");
  /*
  tft.drawCircle(DIAL_CENTER_X, DIAL_CENTER_Y, DIAL_RADIUS, ILI9341_WHITE);
  drawTickMarks();
  drawMajorTickLabels();

  tft.setTextColor(ILI9341_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(110, 210);
  tft.print(" x1000");
*/
 
  tft.fillRect(160,200,200,200, ILI9341_GREEN);
 // tft.fillRect(0,200,95 ,100, ILI9341_RED);

  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(169, 214);
  tft.print("SAVE & RESET");
  //tft.setCursor(20, 214);
  //tft.print("STOPP");
  
  
}

void Display_MCI::draw_display(double reading_bend, double reading_ax, int rpm_value, int loadcycles,String state_name, String status)
 
{

 

if (time_display > 1000)
  {
     tft.setTextSize(2);
   tft.setTextColor(ILI9341_BLACK);
   tft.setCursor(POS_UPDATE_VALUES_X, POS_BIEGEKRAFT_Y);
   tft.printf("%3.2f N",temp_bend);

   tft.setCursor(POS_UPDATE_VALUES_X, POS_AXIALKRAFT_Y);
   tft.printf("%3.2f N",temp_ax);

   tft.setCursor(POS_UPDATE_VALUES_X, 45);
   tft.printf("%d rpm", temp_rpm_value);

   tft.setCursor(POS_UPDATE_VALUES_X, POS_LASTZYKLEN_Y);
   tft.print(temp_loadcycles);

   tft.setCursor(POS_UPDATE_VALUES_X, POS_STATE_Y);
   tft.print(temp_state_name);

    tft.setTextSize(1);
    tft.setCursor(50, POS_STATUS_Y);
    tft.print(temp_status);
    tft.setTextSize(2);

   tft.setTextColor(ILI9341_WHITE);
   tft.setCursor(POS_UPDATE_VALUES_X, POS_BIEGEKRAFT_Y);
   tft.printf("%3.2f N",reading_bend);
  
   tft.setCursor(POS_UPDATE_VALUES_X, POS_AXIALKRAFT_Y);
   tft.printf("%3.2f N",reading_ax);

  // tft.setCursor(POS_UPDATE_VALUES_X, 45);
   //tft.printf("%d rpm",rpm_value);

  tft.setCursor(POS_UPDATE_VALUES_X, POS_LASTZYKLEN_Y);
   tft.print(loadcycles);

    tft.setCursor(POS_UPDATE_VALUES_X, POS_STATE_Y);
    tft.print(state_name);
  tft.setTextSize(1);

if (status=="Card failed"){
  tft.setTextColor(ILI9341_RED);
}
else 
{
  tft.setTextColor(ILI9341_GREEN);
}
    tft.setCursor(50, POS_STATUS_Y);
    tft.print(status);
    tft.setTextSize(2);

   temp_bend =reading_bend;
   temp_ax = reading_ax;
   temp_rpm_value = rpm_value;
   temp_loadcycles = loadcycles; 
   time_display = 0;
   temp_state_name = state_name;
   temp_status = status; 
  
 }

}
 
     
void Display_MCI::draw_tacho(int rpm)
{
  if (time_display_tacho > 1000)
  {
  clearIndicatorHand(temp_indicator);
  tft.fillCircle(DIAL_CENTER_X, DIAL_CENTER_Y, 5, ILI9341_ORANGE);
  tft.setTextColor(ILI9341_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(110, 210);
  tft.print(" x1000");
  drawMajorTickLabels();
  drawIndicatorHand(rpm);
  tft.fillCircle(DIAL_CENTER_X, DIAL_CENTER_Y, 5, ILI9341_ORANGE);
  tft.setTextColor(ILI9341_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(110, 210);
  tft.print(" x1000");
  drawMajorTickLabels();
  temp_indicator = rpm;
  time_display_tacho = 0;  
  }
}

