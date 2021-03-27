//Arduino is connected to 3 potentiometers (sliders for hue, val, sat control), a button, a microphone, and a HC06 bluetooth module.
//The bluetooth module communicates via the RS232 TX/RS pins and works with a SPP bluetooth profile.
//Android app is responsible for creating a connection and sending data to Arduino. 
//Data is sent as 2 byte chunks, the first byte is a char identifier and the second is a value.

#include <FastLED.h>

const int PotChangeTrig = 50;
const int PotNoiseTol = 5;
const int PotCutOff = 5;

CRGB crib;
int greenLED = 3;
int redLED = 9;
int blueLED = 10;
int button = 2;
int huePot, satPot, valPot,mic;
byte readBuffer[2];
int hue, sat, val, volume, lastVol, knobHue, knobSat, knobVal, huePlus, hueMinus, satPlus, satMinus, valPlus, valMinus, curHue, curSat, curVal;
boolean fadeLatch, knobLatch, micLatch, phoneLatch, buttonLatch;
unsigned long previousMillis;
long fadeInterval, buttonFadeInterval;

void setup(){

  //Set PWM Clocks
  TCCR0B = TCCR0B & B11111000 | B00000011;
  TCCR1B = TCCR1B & B11111000 | B00000011;
  TCCR2B = TCCR2B & B11111000 | B00000100;

  analogReference(DEFAULT);

  //Set pin modes
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  buttonFadeInterval = -10;
  lastVol = 0;

  fadeLatch = 1;
  knobLatch  = 1;
  micLatch = 0;
  phoneLatch =0;
  buttonLatch = 0;

  huePot = 0;
  satPot = 1;
  valPot = 2;
  mic = 3;

  //Read pot values with ADC
  knobHue = analogRead(huePot)/4;
  knobSat = analogRead(satPot)/4;
  knobVal = 255-(analogRead(valPot)/4);

  hue = knobHue;
  sat = knobSat;
  val = knobVal;

  //Update object with new CHSV object, pass parameters in constructor. CHSV has access to rgb values
  crib = CHSV(hue, sat, val);
  
  Serial.begin(9600);

  Serial.println("Startup");

  updateLights();
  knobLatch = 0;
}        

void loop(){
  //Set tolerances for noise
  curHue = analogRead(huePot)/4;
  curSat = analogRead(satPot)/4;
  curVal = 255-(analogRead(valPot)/4);
  huePlus = curHue + PotChangeTrig;
  hueMinus = curHue - PotChangeTrig;
  satPlus = curSat + PotChangeTrig;
  satMinus = curSat - PotChangeTrig;
  valPlus =  curVal + PotChangeTrig;
  valMinus = curVal - PotChangeTrig;
  
  //Buffer for any potential noise from pots
  if (((knobHue > huePlus) || (knobHue < hueMinus ) || (knobSat > satPlus) || (knobSat < satMinus) || (knobVal > valPlus) || (knobVal < valMinus)) ){
    knobLatch = 1;
    phoneLatch = 0;
    fadeLatch = 0;
    fadeInterval = 0;
    micLatch = 0;
  }
  
  if (Serial.available() > 1){
    phoneLatch = 1;
    knobLatch = 0;
    byte i;
    
    Serial.readBytes(readBuffer,2);
    switch(readBuffer[0]){
      case('H'):
      hue = readBuffer[1];
      fadeLatch = 0;
      break;
      
      case('S'):
      sat = readBuffer[1];
      break;
      
      case('V'):
      val = readBuffer[1];
      micLatch = 0;
      break;

      case('F'):
      i = readBuffer[1];
      if (i == 'F') {
        fadeLatch = 0;
      }else{
        fadeLatch = 1;
        fadeInterval = i;
      }
      break;
  
      case('M'):
      micLatch = 1;
      i = readBuffer[1];
      if ( i  == 'M'){
        micLatch = 0;
        knobLatch= 0;
      }
      break; 
    }
    updateLights();
  }

  //Since button is momentary, buttonLatch stays off while function runs.
  //Pressing the button will cycle through different fade speeds and back to knob control.
  if (digitalRead(button) == 0 && buttonLatch == 0){
    fadeLatch = 1;
    knobLatch = 0;
    phoneLatch = 0;
    if (buttonFadeInterval == 90){
      buttonFadeInterval = -10;
      fadeLatch = 0;
      knobLatch = 1;
    }else{
      buttonFadeInterval = buttonFadeInterval + 20;
      fadeInterval = buttonFadeInterval;
    }
    buttonLatch = 1;
  }

  if (digitalRead(button) == 1){
    buttonLatch = 0;
  }

  if (knobLatch == 1) {
     
        knobHue = analogRead(huePot)/4;
        knobSat = analogRead(satPot)/4;
        knobVal = 255-(analogRead(valPot)/4);

        hue = knobHue;
        sat = knobSat;
        val = knobVal;
        
        if (!fadeLatch){
          updateLights();
        }
  }
  
   if (fadeLatch == 1){
//     Serial.println(fadeInterval);
     unsigned long currentMillis = millis();
     if (currentMillis  - previousMillis >= fadeInterval) {
        previousMillis = currentMillis ;
        if(hue == 255)
        {
          hue = 0;
        }
        else{
          hue = hue + 1;
        }
      }
      if (micLatch == 0){
        updateLights();   
      }
    }
  
    if( micLatch == 1){
      volume = analogRead(mic);
      delay(100);
      
      if ((volume - lastVol) > 1) {
        val = ((volume - lastVol)*25);
      }else {
        val = 40;
      }
      Serial.println(val);
      if (val < 0) {
        val = 0;
      }
      if (val > 255){
        val = 255;
      }
      lastVol = volume;
     
      updateLights();
    }
}


void updateLights(){
    crib.setHSV(hue, sat, val);
    analogWrite(greenLED, crib.green);
    analogWrite(redLED, crib.red);
    analogWrite(blueLED, crib.blue);
}


