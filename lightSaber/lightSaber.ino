//include libraries
#include "neopixel/neopixel.h"
#include "application.h"
#include "SparkFunLSM9DS1/SparkFunLSM9DS1.h"
#include "math.h"
//default behavior
SYSTEM_MODE(AUTOMATIC);

/*****************************************************************
LSM9DS1_Basic_I2C.ino
SFE_LSM9DS1 Library Simple Example Code - I2C Interface
Jim Lindblom @ SparkFun Electronics
Original Creation Date: April 30, 2015
https://github.com/sparkfun/SparkFun_LSM9DS1_Particle_Library
This code is released under the MIT license.
Distributed as-is; no warranty is given.
*****************************************************************/
//////////////////////////
// LSM9DS1 Library Init //
//////////////////////////
// Use the LSM9DS1 class to create an object. [imu] can be
// named anything, we'll refer to that throught the sketch.
LSM9DS1 imu;
///////////////////////
// Example I2C Setup //
///////////////////////
// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M   0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW
////////////////////////////
// Sketch Output Settings //
////////////////////////////
#define PRINT_CALCULATED
//#define PRINT_RAW
#define PRINT_SPEED 250 // 250 ms between prints

// Earth's magnetic field varies by location. Add or subtract 
// a declination to get a more accurate heading. Calculate 
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Denver, CO.

int boardLed = D7; // This is the LED that is already on your device.
//three photoresistor input pins 
int photoresistor1 = A0;
int photoresistor2 = A1;
int photoresistor3 = A2;

// This is the other end of your photoresistor. The other side is plugged into the "photoresistor" pin (above).
// The reason we have plugged one side into an analog pin instead of to "power" is because we want a very steady voltage to be sent to the photoresistor.
// That way, when we read the value from the other side of the photoresistor, we can accurately calculate a voltage drop.
int power1 = A3; 
int power2 = A4;
int power3 = A5;

int intactValue; // This is the average value that the photoresistor reads when the beam is intact.
int brokenValue; // This is the average value that the photoresistor reads when the beam is broken.
int beamThreshold; // This is a value halfway between ledOnValue and ledOffValue, above which we will assume the led is on and below which we will assume it is off.

int analogvalue; // Here we are declaring the integer variable analogvalue, which we will use later to store the combined value of the photoresistors
int aVal1,aVal2,aVal3 = 0;//initiate values for each photoresistor as well

bool contactMade = false;//ensure that after crossing the threshold the saber must go back below the threshold before again publishing an event
bool one,two,three =false;//keep track of which sensor crosses the threshold for the feedback system

float roll;//z
float pitch;//x
float heading;//y

//establish neo pixel info 
#define PIXEL_PIN D2
#define PIXEL_COUNT 43
#define PIXEL_TYPE WS2812B
//create neo pixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() 
{
//test imu settings in the particle cli with the photon connected by usb to your computer 
  //IMU
  Serial.begin(115200);

  // Before initializing the IMU, there are a few settings
  // we may need to adjust. Use the settings struct to set
  // the device's communication mode and addresses:
  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = LSM9DS1_M;
  imu.settings.device.agAddress = LSM9DS1_AG;
  // The above lines will only take effect AFTER calling
  // imu.begin(), which verifies communication with the IMU
  // and turns it on.
  if (!imu.begin())
  {
    Serial.println("Failed to communicate with LSM9DS1.");
    Serial.println("Double-check wiring.");
    Serial.println("Default settings in this sketch will " \
                  "work for an out of the box LSM9DS1 " \
                  "Breakout, but may need to be modified " \
                  "if the board jumpers are.");
    while (1)
      ;
  }
  
    //Light
    // First, declare all of our pins. This lets our device know which ones will be used for outputting voltage, and which ones will read incoming voltage.
    pinMode(photoresistor1,INPUT); 
    pinMode(photoresistor2,INPUT);  
    pinMode(photoresistor3,INPUT); 
    // The pin powering the photoresistor is output (sending out consistent power)
    pinMode(power1,OUTPUT); 
    pinMode(power2,OUTPUT);
    pinMode(power3,OUTPUT);
    //initiate and clear the neo pixels
    strip.begin();
    strip.clear();
    strip.show();
    
    // Write the power of the photoresistor to be the maximum possible, so that we can use this for power.
    digitalWrite(power1,HIGH);
    digitalWrite(power2,HIGH);
    digitalWrite(power3,HIGH);
    
    // We are going to declare a Particle.variable() here so that we can access the value of the photoresistor and the beam threshold from the cloud.
    Particle.variable("analogvalue", &analogvalue, INT);
    Particle.variable("beamThresh", &beamThreshold, INT);


  delay(500);
  //turn the strip on with a brightness of 100 to establish the broken value  
  colorWipe(strip.Color(0, 0, 255), 5); // Blue
  // Now we'll take some readings...
  int off_1 = (analogRead(photoresistor1)); // read photoresistors
  delay(200); // wait 200 milliseconds
  int off_2 = (analogRead(photoresistor1)); // read photoresistors
  delay(300); // wait 300 milliseconds
  delay(500);//delay half second
  
  //turn the strip to full brightness for the intact value
  brightColor(strip.Color(0, 0, 255), 5,255);
  delay(500);//delay half second
    // ...And we will take two more readings.
  int on_1 = (analogRead(photoresistor1)); // read photoresistors
  delay(500); // wait 500 milliseconds
  int on_2 = (analogRead(photoresistor1)); // read photoresistors

  // Now we average the "on" and "off" values to get an idea of what the resistance will be when the laser strikes
  intactValue = (on_1+on_2)/2;
  brokenValue = (off_1+off_2)/2;

  // Let's also calculate the value between laser and no laser to find the crossing point that triggers an event. 
  beamThreshold = (intactValue+brokenValue)/2;
  //turn the saber back to the low brightness (off) setting to prepare for sensing laser contact
  brightColor(strip.Color(0, 0, 255), 5,100);
}

void loop()
{
  
   //store the values of each light sensor
   aVal1 = analogRead(photoresistor1);
   aVal2 = analogRead(photoresistor2);
   aVal3 = analogRead(photoresistor3);
    // check to see what the average value of the photoresistors are and store it in the int variable analogvalue
   analogvalue = ((aVal1 + aVal2 + aVal3)/3);
   //if any of the sensors have crossed the threshold set the associated boolean to true
    if(aVal1 > beamThreshold){
        one = true;
    }
    if(aVal2 > beamThreshold){
        two = true;
        }
    if(aVal3 > beamThreshold){
        three = true;
    }
    //if any of the sensors have crossed the threshhold and contact hasn't been made 
    if(one||two||three)
    {
        if(!contactMade){
            imu.readAccel();
            imu.readMag();
            imu.readGyro();
            //update the pitch, roll and heading
            // Call print attitude. The LSM9DS1's magnetometer x and y
            // axes are opposite to the accelerometer, so my and mx are
            // substituted for each other.
            printAttitude(imu.ax, imu.ay, imu.az, -imu.my, -imu.mx, imu.mz);
            String allData = getData();//put the imu data in a string
            Particle.publish("contact", allData);//publish a contact event with the sensor data
            
            //flash the section of the saber that made contact
            if(one){redFlash(1);}
            if(two){redFlash(2);}
            if(three){redFlash(3);}
            delay(500);//wait half a second
            brightColor(strip.Color(0, 0, 255), 5,100);//turn the saber back to the low brightness (off) setting
            contactMade = true;//contact!
            one,two,three = false;//set sensor threshold booleans back to false after successful post
        }
    }
    else
    {
        contactMade = false;//the saber exited or didn't intersect the beam
    }
}

/********************
 * IMU Functions
 * ******************/
 
String getData()
{
  // To read from the accelerometer, you must first call the
  // readAccel() function. When this exits, it'll update the
  // ax, ay, and az variables with the most current data.
  imu.readAccel();
  imu.readMag();
  imu.readGyro();
  //package all the data in a string with spaces to make seperating values on the event listener end easy 
  String data =  String(imu.calcAccel(imu.ax))  + " " + String(imu.calcAccel(imu.ay)) + " " + String(imu.calcAccel(imu.az)) + " " 
  + String(imu.calcGyro(imu.gx)) + " " + String(imu.calcGyro(imu.gy)) + " " + String(imu.calcGyro(imu.gz))  + " "
  + String(imu.calcMag(imu.mx)) + " " + String(imu.calcMag(imu.my)) + " " + String(imu.calcMag(imu.mz)) + " "
  + String(roll) + " " + String(pitch) + " " + String(heading) ;
  return data;
}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf?fpsp=1
// Heading calculations taken from this app note:
// http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void printAttitude(
float ax, float ay, float az, float mx, float my, float mz)
{
  roll = atan2(ay, az);
  pitch = atan2(-ax, sqrt(ay * ay + az * az));

  heading;
  if (my == 0)
    heading = (mx < 0) ? 180.0 : 0;
  else
    heading = atan2(mx, my);

  heading -= DECLINATION * M_PI / 180;

  if (heading > M_PI) heading -= (2 * M_PI);
  else if (heading < -M_PI) heading += (2 * M_PI);
  else if (heading < 0) heading += 2 * M_PI;

  // Convert everything from radians to degrees:
  heading *= 180.0 / M_PI;
  pitch *= 180.0 / M_PI;
  roll  *= 180.0 / M_PI;

}

/***********************************
 * Neo Pixel Functions
 * *********************************/
 
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
    strip.setBrightness(100);
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    //Serial.print("in call)");
    delay(wait);
  }
  
}
//Set the whole strip to a color and brightness
void brightColor(uint32_t c, uint8_t wait, uint8_t b) {
    strip.setBrightness(b);
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    delay(wait);
  }
  strip.show();
}
//quick feedback response to contact made
void redFlash(int section) {
    uint16_t i = 0;
    uint16_t j = 0;
    if (section == 1){
        j = 13;
    }
    else if(section == 2){
        i = 14;
        j = 27;
    }
     else if(section == 3){
        i = 28;
        j = 43;
    }
  for(i; i<j; i++) {
    strip.setPixelColor(i,strip.Color(255,0,0));
  }
  strip.show();
  delay(500);
}

