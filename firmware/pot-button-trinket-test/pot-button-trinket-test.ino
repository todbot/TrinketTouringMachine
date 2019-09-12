#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>


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
    for(int i=0; i<NUMPIXELS; i++) { 
        pixels->setPixelColor(i, pixels->Color(0, 150, 0));
        pixels->show();   // Send the updated pixel colors to the hardware.
        delay(30);
    }  
}
void loop() {
    digitalWrite(LED_PIN, HIGH);
    int k1 = analogRead(KNOB1_PIN);
    int k2 = analogRead(KNOB2_PIN);

    int button_threshold = 95;
    bool button_press = false;
    int k3 = map(k2, button_threshold,1023, 0,1023);
    if( k2 < (button_threshold-10) ) {
        button_press = true;
        k3 = 0;
    }
    
    Serial.printf("knob1:%d knob2:%d 'knob3':%d, button:%d\n", k1,k2,k3,button_press);

    delay(100);
}
