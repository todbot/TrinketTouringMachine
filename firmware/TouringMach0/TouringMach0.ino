/* NeoPixelTester 
 * ------------------- 
 * Simple Hello world for serial ports.
 * Print out "Hello world!" and blink pin 13 LED every second.
 *
 * Created 13 Aug 2019
 * 2019 Tod E. Kurt <tod@todbot.com>`http://todbot.com/
 */

#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>

#include "waveforms.h"

const bool debug = true;

#define NUMPIXELS 8

// Pins in use
const int NEO_PIN     = 2;  // D2 A1
const int KNOB1_PIN   = A3; // D3
const int KNOB2_PIN   = A4; // D4
const int CVOUT_PIN   = A0; // D1
const int DOTSTAR_DAT = 7; 
const int DOTSTAR_CLK = 8; 
const int LED_PIN     = 13; // onboard LED

Adafruit_DotStar dotstrip = Adafruit_DotStar( 1, DOTSTAR_DAT, DOTSTAR_CLK, DOTSTAR_BGR);

Adafruit_NeoPixel* pixels;


int mode = 0; // mode==0 RGB; mode==1 HSV; mode==2 strip select

uint8_t brightness = 30;
uint32_t mycolor;

uint32_t debugNextMillis;
uint32_t outputNextMillis;
uint32_t pixelsNextMillis;
uint8_t outputMillis = 5;

uint16_t cvout = 0;

void setup() 
{
    dotstrip.begin(); // Initialize pins for output
    dotstrip.setPixelColor(0,0x003300);
    dotstrip.show();
    
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT); 
    pinMode(KNOB1_PIN, INPUT);
    pinMode(KNOB2_PIN, INPUT);
    // don't set A0/D1 as OUTPUT or analog output value is cut off at 2.2V
    // pinMode(CVOUT_PIN, OUTPUT);
  
    pixels = new Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
    pixels->begin();
    pixels->setBrightness(brightness);
    
    // say hello
    for(int i=0; i<NUMPIXELS; i++) { 
        pixels->setPixelColor(i, pixels->Color(0, 150, 0));
        pixels->show();   // Send the updated pixel colors to the hardware.
        delay(30);
    }  
}

void updateCVout0()
{
    cvout = cvout + 1;
    if( cvout >= 1024 ) { cvout = 0; }  // 10-bit DAC on Trinket M0
}

int wave0 = 0;
// see https://www.arduino.cc/en/Tutorial/DueSimpleWaveformGenerator
void updateCVout()
{
    static int cvi;
    cvout = waveformsTable[wave0][cvi] / 4;

    cvi++;
    if(cvi == maxSamplesNum)  // Reset the counter to repeat the wave
        cvi = 0;
}

uint8_t lastNvals[NUMPIXELS];
uint8_t nvi;

void loop () 
{
    
    digitalWrite(LED_PIN, HIGH);
    int v1 = analogRead(KNOB1_PIN);
    int v2 = analogRead(KNOB2_PIN);

    //mycolor = v1*16 + v2;
    
    // debug once every 100 millis
    if( debug && debugNextMillis - millis() > 200 ) { 
        debugNextMillis = millis() + 200;
        Serial.printf("mode:%d cv:%d inputs: %d, %d, bri:%d\n",
            mode,cvout, v1,v2, brightness);
//        Serial.printf("%d %d %d %d %d %d\n", pixels->getColor(i)
    }

    // update cvout every outputMillis milliseconds
    if( outputNextMillis - millis() > outputMillis ) {
        outputNextMillis = millis() + outputMillis;
        updateCVout();
        analogWrite(CVOUT_PIN, cvout);
    }

    if( pixelsNextMillis - millis() > 100 ) {
        pixelsNextMillis = millis() + 100;
        for( int i=1; i<NUMPIXELS; i++) { 
            lastNvals[i] = lastNvals[i-1]; //shift everyone over
        }
        lastNvals[0] = cvout/4;
    }
    
    for( int i=0; i< NUMPIXELS; i++ ) { 
       mycolor = pixels->Color( i, lastNvals[i], 0);
       pixels->setPixelColor(i, mycolor);
    }
    pixels->show();
  
    digitalWrite(LED_PIN, LOW);
}
