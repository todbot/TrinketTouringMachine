/*
*/

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

const int KNOB1_PIN   = A3; // D3
const int KNOB2_PIN   = A4; // D4

void setup()
{
  Serial.begin(115200);
  startMozzi(CONTROL_RATE); // :)
  float f = (float)(afreq100/100.0);
  aSin.setFreq(f); // set the frequency
  aSaw.setFreq(f); // set the frequency
  aTri.setFreq(f); // set the frequency
  aSqu.setFreq(f); // set the frequency
  aSqu.setPhase( SQUARE_ANALOGUE512_NUM_CELLS/2 );

  pinMode(KNOB1_PIN, INPUT);
  pinMode(KNOB2_PIN, INPUT);

}


int r1,r2,r3;
void updateControl()
{
  // put changing controls in here
  int v1 = analogRead(KNOB1_PIN);
  int v2 = analogRead(KNOB2_PIN);

  afreq100 = map( v1, 0,1023, 100, 10*100);
  float f = (float)afreq100 / 100.0;
  
  //freq = mapfloat(v1,0,1023,0.1,10.0);
  aSin.setFreq(f);
  aSaw.setFreq(f);
  aTri.setFreq(f);
  aSqu.setFreq(f);

  if( v2 <= 255 ) { // A: no 3rd wave, 1st ramp down, 2nd ramp up
    r3 = 0;
    r1 = map(v2, 0,255, 1023,512);  
    r2 = map(v2, 0,255, 0,512);
  }
  else if( v2 > 255 && v2 <= 512 ) { // B: 
    r3 = 0;
    r1 = map(v2, 256,512, 512,0);
    r2 = map(v2, 256,512, 512,1023);
  }
  else if( v2 > 512 && v2 <= 768 ) { // C: 
   r1 = 0;
   r2 = map(v2, 512,768, 1023, 512);
   r3 = map(v2, 512,768, 0,    512); 
  }
  else if( v2 > 768 ) { // D: no 1st wave, 2nd ramp down, 3rd ramp up
    r1 = 0;
    r2 = map(v2, 768,1023, 512,0); 
    r3 = map(v2, 768,1023, 512,1023);
  }
    
  // check if data has been sent from the computer:
    if (Serial.available()) {
        Serial.read();
        Serial.printf("r1:%d r2:%d r3:%d\n", r1,r2,r3);
    }
}


int updateAudio()
{
    int v = (aSin.next() * r1) + (aSaw.next() * r2) + (aSqu.next() * r3);
    v = v / 1024; // divisor of scaling r's
    v = v*4; // scale to fill range
    return v;// return an int signal centred around 0
}


void loop()
{
  audioHook(); // required here
}
