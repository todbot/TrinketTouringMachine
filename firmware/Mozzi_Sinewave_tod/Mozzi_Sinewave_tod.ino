/*  Example playing a sinewave at a set frequency,
    using Mozzi sonification library.

    Demonstrates the use of Oscil to play a wavetable.

    Circuit: Audio output on digital pin 9 on a Uno or similar, or
    DAC/A14 on Teensy 3.1, or
    check the README or http://sensorium.github.com/Mozzi/

    Mozzi documentation/API
		https://sensorium.github.io/Mozzi/doc/html/index.html

		Mozzi help/discussion/announcements:
    https://groups.google.com/forum/#!forum/mozzi-users

    Tim Barrass 2012, CC by-nc-sa.
*/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
//#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/sin1024_int8.h> // sine table for oscillator
//#include <tables/cos4096_int16.h>

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
//Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <SIN1024_NUM_CELLS, AUDIO_RATE> aSin(SIN1024_DATA);
//Oscil <COS4096X16_NUM_CELLS, AUDIO_RATE> aSin(COS4096X16_DATA);

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

float freq = 1.2;

const int KNOB1_PIN   = A3; // D3

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{
  Serial.begin(115200);
  startMozzi(CONTROL_RATE); // :)
  //aSin.setFreq(440); // set the frequency
  aSin.setFreq(freq); // set the frequency

  pinMode(KNOB1_PIN, INPUT);

}


void updateControl(){
  // put changing controls in here
  int v1 = analogRead(KNOB1_PIN);
  freq = mapfloat(v1,0,1023,0.1,10.0);
  aSin.setFreq(freq);

//    freq = freq - 0.1;
//    aSin.setFreq(freq); // set the frequency
    
  // check if data has been sent from the computer:
    if (Serial.available()) {
        Serial.read();
        int v1 = analogRead(KNOB1_PIN);
        freq = mapfloat(v1,0,1023,0.1,10.0);
        Serial.printf("freq: %d\n", (int)(freq*100));
    }
//    // read the most recent byte (which will be from 0 to 255):
//    float f = Serial.parseFloat();
////    Serial.printf("parsed: %f\n", f);
//    freq = f;
//    aSin.setFreq(freq);
//  }

}


int updateAudio(){
  return aSin.next() * 4; // return an int signal centred around 0
}


void loop(){
  audioHook(); // required here
}
