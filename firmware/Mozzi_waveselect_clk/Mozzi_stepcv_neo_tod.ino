/*
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


// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

int afreq100 = 120;  // 1.2Hz
int aratio100 = 100; // 1.0

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
  aSin.setFreq(f); // set the frequency
  aSaw.setFreq(f); // set the frequency
  aTri.setFreq(f); // set the frequency
  aSqu.setFreq(f); // set the frequency
  resetPhase();

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

void resetPhase()
{
  aSaw.setPhase(0);
  aSin.setPhase(0);
  aTri.setPhase(0);
  aSqu.setPhase( SQUARE_ANALOGUE512_NUM_CELLS / 2 );
}

volatile int clockState;
uint32_t clockLastMillis;
void onClockChange()
{
  clockState = digitalRead(CLKIN_PIN);
  if( clockState ) { 
    uint32_t now = millis();
    clockLastMillis = millis();
    //setFreq(f);
    resetPhase();
  }
  Serial.printf("\n**Clock!** %d\n", clockState);
}


int clkIn;
int knob1, knob2;
int r1, r2, r3;

void updateLEDs()
{
  // check if data has been sent from the computer:
  if (Serial.available()) {
    Serial.read();
    Serial.printf("r1:%d r2:%d r3:%d clk:%d\n", r1, r2, r3, clockState);
  }

  uint32_t clkColor = (clockState) ? 0x00ffffff : 0x000000;
  pixels->setPixelColor(5, clkColor);
  pixels->setPixelColor(4, pixels->Color(clkIn/4,clkIn/4,clkIn/4));
  
  pixels->setPixelColor(0, pixels->Color(r1/4, r2/4, r3/4));
  pixels->setPixelColor(1, pixels->Color(r1/4, r1/4, r1/4));
  pixels->setPixelColor(2, pixels->Color(r2/4, r2/4, r2/4));
  pixels->setPixelColor(3, pixels->Color(r3/4, r3/4, r3/4));
  pixels->show();
 
}

void updateControl()
{
//  clockState = analog(CLKIN_PIN);
  clkIn = analogRead(CLKIN_PIN);
  clockState = (clkIn > 200);

  // put changing controls in here
  int knob1 = analogRead(KNOB1_PIN);
  int knob2 = analogRead(KNOB2_PIN);

  int k2old = knob2;  // save to compare
  bool button_press = detectButtonFixKnobVal(&knob2);

  afreq100 = map( knob1, 0, 1023, 100, 10 * 100);
  float f = (float)afreq100 / 100.0;

  //freq = mapfloat(v1,0,1023,0.1,10.0);
  aSin.setFreq(f);
  aSaw.setFreq(f);
  aTri.setFreq(f);
  aSqu.setFreq(f);

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

  updateLEDs();
}

// 10-bit DAC = 3.3V / 2**10  = 0.003222V per bit
// 1.0V = 310.3, 2.0V = 620.61, 3.0V = 930.91
//
int updateAudio()
{
  int v = 0;
  // knob = direct voltage output
  v = r2 - 512;
  //    v = r2/4;
  //    v = v*4; // scale to fill range

  v = (aSin.next() * r1) + (aSaw.next() * r2) + (aSqu.next() * r3);
  v = v / 1024; // divisor of scaling r's
  v = v * 4; // scale to fill range
  return v;// return an int signal centred around 0
}


void loop()
{
  audioHook(); // required here
}


/*
   When given a knob value 0-1023, where the lower range
   (decided by 'button_threshold') is where the button press
   value lives, do the following:
   - rescale the knob value to go from 0-1023
   - set knob value to 0 if button press
   - detect and return button press boolean
*/
const int button_threshold = 95; // 1k resistor on 50k
bool detectButtonFixKnobVal(int*knobval)
{
  int k = *knobval;
  bool button_press = false;
  int knew = map(k, button_threshold, 1023, 0, 1023);
  if ( k < (button_threshold - 10) ) {
    button_press = true;
    knew = 0;
  }
  *knobval = knew;
  return button_press;
}
