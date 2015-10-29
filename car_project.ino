/*                        *** Car Project by Marvin Ibarra ***
 * This is an arduino project for my car. This is to measure internal/external temperature,
 * run the rear windshield wiper(not implemented yet), record the time the car has been on, and have the
 * instrument lights transition RGB colors. An OLED display is used as the display.
 * the Celsius/unit ratio was calculated with the TMP36 voltage-to-Celsius ratio of 20mV/C
 * and the arduino 10-bit A/D ratio of 3.22mV/unit.
 *
 *                              *** Car Fuse Info ***
 * Option A - uses fuse #36, (under hood of car) a 50A fuse - for the moon roof
 *          - HOT all the time. NOT USED
 * Option B - uses fuse #16, an empty fuse - currently using a 7.5A
 *          - HOT all the time. Powers Arduino. Arduino will be on all the time
 * Option C - uses fuse #15, an empty fuse - currently using a 7.5A
 *          - HOT only on ON(II). Software controlled; This will operate the temp, OLED, and TOT.
 * Option D - uses fuse #18, a 10A fuse - for the tail light, instrument light, and license light
 *          - HOT only on light switch. Hardware controlled; This will turn on the RGB LEDs.
 * Option E - uses fuse #22, a 15A fuse - for the radio and cigarette lighter
 *          - HOT only on ACC(I) and ON(II)
 */

#include <SPI.h>
#include <Wire.h>
#include <SFE_MicroOLED.h>
#include <Adafruit_NeoPixel.h>

#define PIN_DC 8
#define PIN_RESET 9
#define PIN_CS 10

//#define FLOATING_PIN A0
//#define TEMP_SENSOR_1 A1
//#define TEMP_SENSOR_2 A2
#define TEMP_SENSOR_3 A0

#define LED_PIN 6
#define LED_COUNT 7

#define OPTION_C 2
#define BUTTON 4
#define ROW_1 0
#define ROW_2 7
#define ROW_3 14
#define ROW_4 21
#define ROW_5 28
#define ROW_6 35

//the varaibles
int state = 0;  //used for the switch case for the presets
long start_time = 0;
String estimate_time = "00:00";  //estimate time car has been on
String exterior_result = "0.0F", interior_result = "0.0F";  //temp sensor variables
uint16_t j = 0;  //16-bit unsigned int
 
//create an Adafruit_NeoPixel object called "leds"
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT,LED_PIN,NEO_GRB + NEO_KHZ800);
  
//create a MicroOLED object called "oleds"
MicroOLED oleds(PIN_RESET, PIN_DC, PIN_CS);

void setup()
{
  oleds.begin();  //initialize the OLED display
  leds.begin();  //initialize all the LEDs
  oleds.setFontType(0);  //set text font to small 5x7-pixel characters
  
  pinMode(OPTION_C, INPUT);  //powered by a 5V regulator
  pinMode(BUTTON, INPUT_PULLUP);  //used to change color preset
}

//the "main" function
void loop()
{
  oleds.clear(PAGE);  //clear the display's internal memory
  
  //if key position is not on "ON(II)"
  //the code after the IF statement is ignored
  if(!digitalRead(OPTION_C))
  {
    start_time = millis();  //record time
    oleds.clear(ALL);  //clear the library's display buffer
    colorPreset(leds.Color(127, 127, 127), 50);  //white at half brightness
    return;
  }
  if(!digitalRead(BUTTON))  state++;  //increment the state when button is pressed
  else if(state > 5)  state = 0;  //return to state 0
  
  readSensors(exterior_result, interior_result);  //call the temperature sensors
  timeOfTravel(estimate_time);
  
  oleds.setCursor(0, ROW_1);  //set the text cursor to first row
  oleds.print("Int: ");
  oleds.print(interior_result);
  
  //oleds.setCursor(0, ROW_2);  //set the text cursor to second row
  //oleds.print("Ext: ");
  //oleds.print(exterior_result);
  
  oleds.setCursor(0, ROW_3);  //set the text cursor to the third row
  oleds.print("TOT: ");
  oleds.print(estimate_time);
  
  oleds.setCursor(0,ROW_5);  //set the text cursor to the fourth row
  oleds.print("Dash Color");
  
  oleds.setCursor(0,ROW_6);  //set the text cursor to the fifth row
  switch(state)  //preset colors for the dash light
  {
    case 0:
    rainbowCycle(100, j);  //creates the rainbow cycle through the color wheel
    oleds.print(" Rainbow");
    break;
    
    case 1:
    colorPreset(leds.Color(127, 127, 127), 50);  //white at half brightness
    oleds.print("  White");
    break;
    
    case 2:
    colorPreset(leds.Color(255, 0, 0), 50);  //red
    oleds.print("   Red");
    break;
    
    case 3:
    colorPreset(leds.Color(0, 255, 0), 50);  //green
    oleds.print("  Green");
    break;
    
    case 4:
    colorPreset(leds.Color(0, 0, 255), 50);  //blue
    oleds.print("   Blue");
    break;
    
    case 5:
    colorPreset(leds.Color(255, 255, 0), 50);  //yellow
    oleds.print("  Yellow");
    break;
  }
  oleds.display();  //draw to the screen
}

void rainbowCycle(uint8_t wait, uint16_t& j) 
{
  if(j > 256) j = 0;
  j++;
  // 5 cycles of all colors on wheel 
  for(uint16_t i = 0; i < leds.numPixels(); i++)
  {
    leds.setPixelColor(i, Wheel(((i * 256 / leds.numPixels()) + j) & 255));
  }
  leds.show();
  delay(wait);
}

void colorPreset(uint32_t c, uint8_t wait)
{
  for(uint16_t i=0; i<leds.numPixels(); i++) 
  {
      leds.setPixelColor(i, c);
      leds.show();
      delay(wait);
  }
}

void readSensors(String& exterior_result, String& interior_result)
{
  //int sense1 = analogRead(TEMP_SENSOR_1);  //sensor for the left side mirror
  //int sense2 = analogRead(TEMP_SENSOR_2);  //sensor for the right side mirror
  int sense3 = analogRead(TEMP_SENSOR_3);  //sensor for the interior
  //int average = (sense1+sense2)/2;  //getting the average of both left/right temp sensors
  
  //calculating the temperature in Celsius with the ratio
  //and converting it to fahrenheit
  //the extra .5 in 32 is for the double to int truncation
  //int ext_result = (average * 0.123 * 1.8) + 32.5;
  int int_result = (sense3 * 0.123 * 1.8) + 32.5;  // ratio 0.153 is a calibration ratio C/units
  
  //exterior_result = String(ext_result) + "F";
  interior_result = String(int_result) + "F";
  
  return;
}

void timeOfTravel(String& estimate_time)
{
  unsigned long time = millis();
  int seconds = 0, minutes = 0, hours = 0;
  
  time -= start_time;  //get the duration car was on
  time /= 1000;  //converting time from miliseconds to seconds
  //seconds = time % 60;  //getting seconds
  time /= 60;  //converting time from seconds to minutes
  minutes = time % 60;  //getting minutes
  hours = time / 60;
  if(minutes < 10) estimate_time = String(hours) + ":0" + String(minutes);
  else estimate_time = String(hours) + ":" + String(minutes);
  
  return;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return leds.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return leds.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return leds.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
