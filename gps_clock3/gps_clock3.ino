#include <SoftwareSerial.h>
#include "Adafruit_GPS.h"
#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            2

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16

// From adaFruit NEOPIXEL goggles example: Gamma correction improves appearance of midrange colors
const uint8_t gamma[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
      3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
      7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
     13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
     20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
     30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
     42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
     76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
     97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
    122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
    150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
    182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
    218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255
};

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// Offset the hours from UTC (universal time) to your local time by changing
// this value.  The GPS time will be in UTC so lookup the offset for your
// local time from a site like:
//   https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
// This value, -7, will set the time to UTC-7 or Pacific Standard Time during
// daylight savings time.
#define HOUR_OFFSET       +9

SoftwareSerial gpsSerial(4, 3);  // GPS breakout/shield will use a 
                                 // software serial connection with 
                                 // RX = pin 4 and TX = pin 3.
Adafruit_GPS gps(&gpsSerial);

uint32_t milli1_color  = pixels.Color ( 2,  3,  10);
uint32_t milli2_color  = pixels.Color ( 4,  6,  15);
uint32_t milli3_color  = pixels.Color ( 6,  9,  20);
uint32_t milli4_color  = pixels.Color ( 8, 12,  25);
uint32_t milli5_color  = pixels.Color ( 10, 15, 30);
uint32_t second_color  = pixels.Color ( 0, 0, 50);
uint32_t hour_color    = pixels.Color ( 0, 50, 20);
uint32_t minutes_color = pixels.Color ( 30, 0, 20);
uint32_t off_color     = pixels.Color ( 0, 0, 0);

void setup() {
  // Setup function runs once at startup to initialize the display and GPS.

  // Setup Serial port to print debug output.
  Serial.begin(115200);
  Serial.println("Clock starting!");
  
  // Setup the GPS using a 9600 baud connection (the default for most
  // GPS modules).
  gps.begin(9600);

  // Configure GPS to onlu output minimum data (location, time, fix).
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // Use a 1 hz, once a second, update rate.
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  
  // Enable the interrupt to parse GPS data.
  enableGPSInterrupt();
  
  //Initialise the NeoPixel library (watch this doesn't disable interrupts)
  pixels.begin();
}

uint32_t inhibitTimer =0;     //the time of the last transmission
uint32_t inhibitPeriod =800;  //900mS
bool dispInhibit = false;


void loop() {
  // Loop function runs over and over again to implement the clock logic.

  //GPS code
  // Check if GPS has new data and parse it.
  if (gps.newNMEAreceived()) {
    if (gps.parse(gps.lastNMEA())){
      Serial.println ("----GPS Parse OK");
    }
    debug();
    inhibitTimer = millis();
    dispInhibit = false;
  }

  //inhibit code
  if(millis() - inhibitTimer > inhibitPeriod){
    dispInhibit = true;
  }



  //Display code
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  const long interval = 10;       //change the speed of the cylon here
  if (currentMillis - previousMillis >= interval) {
    //previousMillis = currentMillis;
    previousMillis += interval;

    //if (gps.fixquality !=0){
    if (gps.fix){
        drawclock(); //shows the clock, as long as GPS is locked, else cylon
    }
    else {
      cylon();
    }
    gammacorrect(); //correct brightness
    gps.read();
    //clearstrand();

    //if we're not inhibited, refresh the display
    if(dispInhibit == false) pixels.show(); // This sends the updated pixel color to the hardware.

    gps.read();
    //delay (2); //helps to slow down pixels.show to give a more stable GPS parse
  }
}

SIGNAL(TIMER0_COMPA_vect) {
  // Use a timer interrupt once a millisecond to check for new GPS data.
  // This piggybacks on Arduino's internal clock timer for the millis()
  // function.
  gps.read();
}

void enableGPSInterrupt() {
  // Function to enable the timer interrupt that will parse GPS data.
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function above
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}

void clearstrand(){
  //Sets all neopixels blank
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, (0,0,0));
  }
}

void clearstrand2(){
  //sparkling random colours instead of blank pixels
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(random(30,50),0,random(50,80)));
  }
}

void cylon(){
  static int i=0;
  int j=0;

  clearstrand2();
  if (i>=16){
    j=31-i;
  }else{
    j=i;
  }
  //pixels.setPixelColor(j, pixels.Color(200,50,150));    //magenta
  pixels.setPixelColor(j, pixels.Color(random(0,255),random(0,255),random(0,255)));    //randomises colour every time it moves to the next pixel
  //pixels.setPixelColor(j, pixels.Color(random(100,200),0,random(200,255)));    //random shades of blue, pink, and purple
  i++;
  if (i==32){
    i=0;
  }
}

void drawclock(){
  // Grab the current hours, minutes, seconds from the GPS.
  // This will only be set once the GPS has a fix!  Make sure to add
  // a coin cell battery so the GPS will save the time between power-up/down.
  int hours = gps.hour + HOUR_OFFSET;  // Add hour offset to convert from UTC
                                       // to local time.
  // Handle when UTC + offset wraps around to a negative or > 23 value.
  if (hours < 0) {
    hours = 24+hours;
  }
  if (hours > 23) {
    hours = 24-hours;
  }
  if (TIME_24_HOUR == false){ //set 12 hour time
  hours %= 12;
  }
  
  int minutes = gps.minute;
  int seconds = gps.seconds;
  
  clearstrand();
  //hours goes form led0 to led11, 1 hour per led
  int hoursled = hours;
  add_color(hoursled, hour_color);
  
  //mins goes from led0 to led11, 5 minutes per led
  int minutesled = minutes/5;
  add_color(minutesled, minutes_color);
  
  //seconds goes from led0 to led11
  int secondsled = seconds/5;
  add_color(secondsled, second_color);
  
  //every second, a pulse crosses the whole strip end to end

  
}

//copied from NeoPixel ring clock face by Kevin ALford and modified by Becky Stern for Adafruit Industries
void add_color (uint8_t position, uint32_t color)
{
  uint32_t blended_color = blend (pixels.getPixelColor (position), color);
 
  /* Gamma mapping */
  uint8_t r,b,g;
 
  r = (uint8_t)(blended_color >> 16),
  g = (uint8_t)(blended_color >>  8),
  b = (uint8_t)(blended_color >>  0);
 
  pixels.setPixelColor (position, blended_color);
}
 
 
uint32_t blend (uint32_t color1, uint32_t color2)
{
  uint8_t r1,g1,b1;
  uint8_t r2,g2,b2;
  uint8_t r3,g3,b3;
 
  r1 = (uint8_t)(color1 >> 16),
  g1 = (uint8_t)(color1 >>  8),
  b1 = (uint8_t)(color1 >>  0);
 
  r2 = (uint8_t)(color2 >> 16),
  g2 = (uint8_t)(color2 >>  8),
  b2 = (uint8_t)(color2 >>  0);
 
 
  return pixels.Color (constrain (r1+r2, 0, 255), constrain (g1+g2, 0, 255), constrain (b1+b2, 0, 255));
}

void gammacorrect(){
  uint8_t r,b,g;
  for(int i=0; i<NUMPIXELS; i++){ //increment through all pixels
    uint32_t raw_color= pixels.getPixelColor(i); //fetch color
    r = (uint8_t)(raw_color >> 16); //extracting colour values from the uint32
    g = (uint8_t)(raw_color >>  8);
    b = (uint8_t)(raw_color >>  0);
    r = gamma[r]; //select pixel intensity from colour value using gamma chart
    g = gamma[g];
    b = gamma[b];
    pixels.setPixelColor (i, pixels.Color(r,g,b)); //jam it back in
  
  }
}

void debug(){
  Serial.print ("gps.hour=");
  Serial.print (gps.hour);
  Serial.println("");
  Serial.print ("gps.minute=");
  Serial.print (gps.minute);
  Serial.println("");
  Serial.print ("gps.second=");
  Serial.print (gps.seconds);
  Serial.println("");
  Serial.print ("gpsfix=");
  Serial.print (gps.fixquality);
  Serial.println("");
  gps.fix ? Serial.println ("gps has fix"): Serial.println ("gps has no fix");

  Serial.print ("gps data=");
  Serial.print (gps.lastNMEA());
  Serial.println("");
}

