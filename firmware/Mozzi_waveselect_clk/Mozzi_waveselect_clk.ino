/**

*/
#include <Adafruit_NeoPixel.h>
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template

#include <tables/saw256_int8.h> // saw table for oscillator
#include <tables/sin256_int8.h> // sin table for oscillator
#include <tables/cos256_int8.h> // sin table for oscillator
#include <tables/triangle_analogue512_int8.h> // tri table for oscillator
#include <tables/square_analogue512_int8.h> // square table for oscillator
#include <tables/square_no_alias512_int8.h> // square table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SAW256_NUM_CELLS, AUDIO_RATE> aSaw(SAW256_DATA);
Oscil <COS256_NUM_CELLS, AUDIO_RATE> aSin(COS256_DATA);
Oscil <TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTri(TRIANGLE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSqu(SQUARE_ANALOGUE512_DATA);

const bool debug = true;

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable


int afreq100 = 120;  // 1.2Hz
int aratio100 = 100; // 1.0

float theFreq = 0;

// Pins in use
const int NEO_PIN     = 2;  // D2 A1
const int KNOB1_PIN   = A3; // D3
const int KNOB2_PIN   = A4; // D4
const int CVOUT_PIN   = A0; // D1
const int CLKIN_PIN   = A2; // D0
const int DOTSTAR_DAT = 7;
const int DOTSTAR_CLK = 8;
const int LED_PIN     = 13; // onboard LED

#define NUMPIXELS 8
Adafruit_NeoPixel* pixels;

const int brightness = 50;

void setup()
{
  Serial.begin(115200);
  startMozzi(CONTROL_RATE); // :)
  float f = (float)(afreq100 / 100.0);
  setFreq(f);
  setPhase(0);

  // don't set A0/D1 as OUTPUT or analog output value is cut off at 2.2V
  // pinMode(CVOUT_PIN, OUTPUT);

  pinMode(KNOB1_PIN, INPUT);
  pinMode(KNOB2_PIN, INPUT);

  pinMode(CLKIN_PIN, INPUT);
  // looks like we can't use interrupts with both Mozzi & Neopixel
  // because combined it means we get pixel glitches
  //attachInterrupt(CLKIN_PIN, onClockChange, CHANGE);

  pixels = new Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  pixels->setBrightness(brightness);
  // say hello
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels->setPixelColor(i, pixels->Color(0, 150, 0));
    pixels->show();   // Send the updated pixel colors to the hardware.
    delay(30);
  }

}

void setFreq(float f)
{
  aSin.setFreq(f); // set the freqency, in float hertz
  aSaw.setFreq(f);
  aTri.setFreq(f);
  aSqu.setFreq(f);
}

void setPhase(int p)
{
  aSaw.setPhase(p);
  aSin.setPhase(p);
  aTri.setPhase(p);
  aSqu.setPhase( p + SQUARE_ANALOGUE512_NUM_CELLS / 2 );
}


int clkIn;
bool clockState;
bool isClocked = false;
uint32_t clockLastMillis;
uint32_t clockLastSeenMillis = 0;
//int clkFudgeMicros = 600;
int clkFudgeMillis = 1;

int knob1, knob2;
bool button_press = false;
int r1, r2, r3;
volatile int lastV;
int lastLastV;
bool mode_wavesel = true; // if false, then amplitude mode
int amplitude = 1024; // max

void debugShowSerial()
{
  if( debug ) { 
    if( lastLastV <= 0 && lastV > 0 ) { 
      Serial.println('!');
    }
  }
  
  // check if data has been sent from the computer:
  if (Serial.available()) {
    Serial.read();
    Serial.printf("mode:%d clkd:%d clk:%d k1:%3d k2:%3d but:%d \tr1:%3d r2:%3d r3:%3d lastV:%d \n",
                  mode_wavesel, isClocked, clockState,
                  knob1,knob2, button_press,
                  r1, r2, r3, lastV);
  }
}

void updateLEDs()
{
  pixels->clear();

  uint32_t clkColor = (isClocked) ? 0x00ffff00 : 0x00000000;
  if( lastV > 0 ) { 
    clkColor = clkColor + 0xff;
  }
  
  uint32_t clkInColor = 0x000000;
  if( clockState ) { 
    clkInColor = 0x00ffffff;
  }
  int lastVc = (lastV+512)/4;
  pixels->setPixelColor(7, clkColor);
  pixels->setPixelColor(6, clkInColor);
  pixels->setPixelColor(5, pixels->Color(clkIn/4, clkIn/4, clkIn/4));
  pixels->setPixelColor(4, pixels->Color(lastVc,lastVc,lastVc) );

  pixels->setPixelColor(1, pixels->Color(mode_wavesel*88,mode_wavesel*88, 0));
  
  //pixels->setPixelColor(3, pixels->Color(r3 / 4, r3 / 4, r3 / 4));
  //pixels->setPixelColor(2, pixels->Color(r2 / 4, r2 / 4, r2 / 4));
  //pixels->setPixelColor(1, pixels->Color(r1 / 4, r1 / 4, r1 / 4));

  pixels->setPixelColor(0, pixels->Color(r1/4, r2/4, r3/4));
  pixels->show();

}

// do input CV clock detection
void updateClock()
{ 
  uint32_t now = millis();
  uint16_t t = now - clockLastMillis;
  
  clkIn = analogRead(CLKIN_PIN);
  int newClockState = (clkIn > 200); // threshold

  if ( !newClockState && clockState) { // on falling edge
    clockLastSeenMillis = now; // save new clock
    clockLastMillis = now;
    t -= clkFudgeMillis;
    if( t > 20 && t < 5000 ) { // only update if within good range
      theFreq = 1000.0 / t;
    }
    //setFreq(f);  // setFreq() done in updateControl() now
    //setPhase(0); // FIXME: hmm this screws up sub-freq
    Serial.printf("\n!%d\n", t);
  }
  clockState = newClockState;
  
  if( now - clockLastSeenMillis > 5000 || now < 5000 ) {
    isClocked = false;
  } else { 
    isClocked = true;
  }
}

//
void waveSelect(int knob2)
{
  if ( knob2 <= 255 ) { // A: no 3rd wave, 1st ramp down, 2nd ramp up
    r3 = 0;
    r1 = map(knob2, 0, 255, 1023, 512);
    r2 = map(knob2, 0, 255, 0,   512);
  }
  else if ( knob2 > 255 && knob2 <= 512 ) { // B:
    r3 = 0;
    r1 = map(knob2, 256, 512, 512, 0);
    r2 = map(knob2, 256, 512, 512, 1023);
  }
  else if ( knob2 > 512 && knob2 <= 768 ) { // C:
    r1 = 0;
    r2 = map(knob2, 512, 768, 1023, 512);
    r3 = map(knob2, 512, 768, 0,    512);
  }
  else if ( knob2 > 768 ) { // D: no 1st wave, 2nd ramp down, 3rd ramp up
    r1 = 0;
    r2 = map(knob2, 768, 1023, 512, 0);
    r3 = map(knob2, 768, 1023, 512, 1023);
  }  
}

// called by Mozzi
void updateControl()
{
  int k2old = knob2;  // save to compare
  
  // put changing controls in here
  knob1 = analogRead(KNOB1_PIN);
  knob2 = analogRead(KNOB2_PIN);
  
  button_press = detectButtonFixKnobVal(&knob2,k2old); // modifies knob2
  if( button_press ) { 
    knob2 = k2old; // put back old value FIXME: hmmm. seems wrong
    mode_wavesel = !mode_wavesel; // toggle    
  }
  
  if ( !isClocked ) { // if no clock, knob selects freq
    // 1 Hz to 10 Hz
    afreq100 = map( knob1, 0, 1023, 0.1 *100, 60 * 100); // FIXME
    theFreq = (float)afreq100 / 100.0;
    setFreq(theFreq);
  }
  else {              // pot selects freq multiples/divisors
    // FIXME: this is a dumb way to do this
    float b = 4 - map(knob1, 0,1023, 0,8);
    float f = theFreq;
    if( b > 0 ) {        f = f * b;  }
    else if (b < 0 ) {   f = f / -b;  }
    setFreq(f);
  }

  if( mode_wavesel ) {
    waveSelect(knob2);
  }
  else { 
    amplitude = knob2;
  }
  
  updateLEDs();

  debugShowSerial();

  lastLastV = lastV;  // save for next go-around

}

// called by Mozzi
// 10-bit DAC = 3.3V / 2**10  = 0.003222V per bit
// 1.0V = 310.3, 2.0V = 620.61, 3.0V = 930.91
int updateAudio()
{
  int v = 0;
  
  // mix three different waves proportionally
  v = (aSin.next() * r1) + (aSaw.next() * r2) + (aSqu.next() * r3);
  v = v / 1024; // divisor of scaling r's
  v = (v * amplitude) / 1024; 
  v = v * 4; // scale to fill range
  
  lastV = v;
  return v;// return an int signal centred around 0
}


void loop()
{
  updateClock();

  audioHook(); // required here
}


/*
   Detect button on knob
    When given a knob value 0-1023, where the lower range
    (decided by 'button_threshold') is where the button press
    value lives, do the following:
    - rescale the knob value to go from 0-1023
    - set knob value to  if button press
    - detect and return button press boolean
*/
const int buttonThreshold = 90; // 1k resistor on 50k, 90 out of 0-1023 range
uint32_t detectButtonMillis;
uint32_t lastPressMillis;
const int pressDurMillis = 100;
//
bool detectButtonFixKnobVal(int*knobval, int knobvallast)
{
  int k = *knobval; // get knobval int from pointer
  bool button_press = false;
  // remap knobval to ignore button value range
  int knewval = map(k, buttonThreshold, 1023, 0, 1023);

  if( (millis() - detectButtonMillis) > 50 ) { // time to update debouncer FIXME
    detectButtonMillis = millis(); 
    
    bool isPress = (k < (buttonThreshold - 10)); // detect button
    
    if( isPress ) {
      lastPressMillis = millis();
      knewval = knobvallast;  // knobval is bad, use last good value
      // handle debounce: if was pressed and still pressed, is really pressed!
      if( lastPressMillis && (millis() - lastPressMillis > pressDurMillis) ) {
        button_press = true;
      }
    }
    else { 
      lastPressMillis = 0; // zero out our debounce timer because no press
    }
    
    return button_press;
  }
  
  *knobval = knewval; // save the value back to pointer
  return button_press;
}
