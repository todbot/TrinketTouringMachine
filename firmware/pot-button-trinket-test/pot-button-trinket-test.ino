/**
 * pot-button-trinket-test --  
 *  -- have a pot and button share an input, and still both work
 *  -- see 'pot-button-test-schematic.png' for hookup details
 *  
 *  2019 todbot / Tod Kurt  
 */
 
#include <Adafruit_NeoPixel.h>


#define NUMPIXELS 8

// Pins in use
const int NEO_PIN     = 2;  // D2 A1
const int KNOB1_PIN   = A3; // D3
const int KNOB2_PIN   = A4; // D4
const int CVOUT_PIN   = A0; // D1
const int DOTSTAR_DAT = 7;
const int DOTSTAR_CLK = 8;
const int LED_PIN     = 13; // onboard LED

Adafruit_NeoPixel* pixels;
uint8_t brightness = 30;

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(KNOB1_PIN, INPUT_PULLUP);
  pinMode(KNOB2_PIN, INPUT_PULLUP);
  // don't set A0/D1 as OUTPUT or analog output value is cut off at 2.2V
  // pinMode(CVOUT_PIN, OUTPUT);

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

/*
 * When given a knob value 0-1023, where the lower range
 * (decided by 'button_threshold') is where the button press
 * value lives, do the following:
 * - rescale the knob value to go from 0-1023
 * - set knob value to 0 if button press
 * - detect and return button press boolean
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

void loop()
{
  digitalWrite(LED_PIN, HIGH);
  int k1 = analogRead(KNOB1_PIN); // regular knob
  int k2 = analogRead(KNOB2_PIN); // knob with button 
  
  int k2old = k2;  // save to compare
  bool button_press = detectButtonFixKnobVal(&k2);

  pixels->setPixelColor(1, pixels->Color(0,k1/4,0) );
  pixels->setPixelColor(2, pixels->Color(0,k2/4,0) );
  pixels->setPixelColor(3, pixels->Color(0,k2old/4,0) );
  pixels->setPixelColor(0, pixels->Color(0,0,(button_press?255:0)) );
  pixels->show();
  
  Serial.printf("knob1:%d\t knob2:%d\t k2old:%d\t button:%d\n", 
                   k1, k2, k2old, button_press);

  delay(100);
}
