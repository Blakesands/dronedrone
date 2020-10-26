/*
   ApplySignal.ino
   2018 WLWilliams
   Library code found at: https://github.com/Billwilliams1952/AD9833-Library-Arduino

*/

#include <AD9833.h>     // Include the library
#define FNC_PIN 4       // Can be any digital IO pin

// My code

#define FNC_PIN2 5       // Define another pin for separate ad9833
#define DATA  11  ///< SPI Data pin number
#define CLK   13  ///< SPI Clock pin number

// arrays
int wave[2] = {SINE_WAVE, TRIANGLE_WAVE}; // not using the square waves  SQUARE_WAVE, HALF_SQUARE_WAVE
int freq[21] = {233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740};
int phas[23] = {0, 0.003, 0.006, 0.011, 0.022, 0.045, 0.09, 0.18, 0.36, .72, 1.44, 2.88, 5.72, 11.5, 22.5, 45, 90, 135, 180, 225, 270, 315, 0};
int sgnl[2] = {false, true};
int hmon[7] = {2, 3, 4, 5, 6, 7, 8}; // harmonic content hz multiplier on second ad9833
int trim[43] = {-256, -128, -64, -32, -30, -28, -26, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 64, 128, 256}; //harmonic content Hz trim

// Variables for inputs and indexes
const int buttonPin1 = 3;     // the number of the pushbutton pin
int buttonState1 = 0;         // current state of the button
int lastButtonState1 = 0;     // previous state of the button
int waveindex = 0;  // 0-1 Sine or Triangle Wave
int lastwaveindex = 0;

const int buttonPin2 = 2;     // the number of the pushbutton pin
int buttonState2 = 0;         // current state of the button
int lastButtonState2 = 0;     // previous state of the button
int signalindex = 0; // 0-1 off/on
int lastsignalindex = 0;

const int buttonPin3 = 6;     // the number of the pushbutton pin
int buttonState3 = 0;         // current state of the button
int lastButtonState3 = 0;     // previous state of the button
int harmonicindex = 0; // Harmonic multiplier for gen2
int lastharmonicindex = 0;

const int pot3 = A3;     // the number of the pushbutton pin
int phaseindex = 0; // Harmonic multiplier for gen2
int lastphaseindex = 0;

const int pot = A0;    // select the input pin for the potentiometer
int freqindex = 0; // 0-20 Frequencies
int lastfreqindex = 0;

const int pot2 = A7;    // select the input pin for the potentiometer
int trimindex = 0; // -50 to +50hz trim for harmonics
int lasttrimindex = 0;

//--------------- Create an AD9833 object ----------------
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency
AD9833 gen2(FNC_PIN2);       // Defaults to 25MHz internal reference frequency


void setup() {
  
  // This MUST be the first command after declaring the AD9833 object
  gen.Begin();
//  gen2.Begin();

  Serial.begin(115200);

  // button code
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
//  pinMode(buttonPin4, INPUT);
  pinMode(pot, INPUT);
  pinMode(pot2, INPUT);
  pinMode(pot3, INPUT);
    
  // Apply a 1000 Hz sine wave using REG0 (register set 0). There are two register sets,
  // REG0 and REG1.
  // Each one can be programmed for:
  //   Signal type - SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, and HALF_SQUARE_WAVE
  //   Frequency - 0 to 12.5 MHz
  //   Phase - 0 to 360 degress (this is only useful if it is 'relative' to some other signal
  //           such as the phase difference between REG0 and REG1).
  // In ApplySignal, if Phase is not given, it defaults to 0.


  gen.ApplySignal(SINE_WAVE, REG0, 262, 0); // defaults to 1000hz Sine on R0
  gen2.ApplySignal(TRIANGLE_WAVE, REG0, 2620, 0); // defaults to 1000hz Sine on R0
  gen.EnableOutput(true);   // Turn ON the output - it defaults to OFF
  gen2.EnableOutput(true);   // Turn ON the output of gen2 - it defaults to OFF
  //  gen.SetOutputSource(REG1);   // Switch between 2 registers on 1 output (mystery solved)
}

void loop() {

  buttonState1 = digitalRead(buttonPin1); // read the pushbutton input pin: 
  buttonState2 = digitalRead(buttonPin2); // read the pushbutton input pin:
  buttonState3 = digitalRead(buttonPin3); // read the pushbutton input pin:
//  buttonState4 = digitalRead(buttonPin4); // read the pushbutton input pin:
    
  freqindex = analogRead(pot); // read the potentiometer analog input pin:
  freqindex = map(freqindex, 0, 1000, 0, 20); // map pot input to array size
  freqindex = constrain(freqindex, 0, 20); // belt and braces
  
  trimindex = analogRead(pot2); // read the potentiometer analog input pin:  
  trimindex = map(trimindex, 0, 1000, 0, 42); // map pot input to array size
  trimindex = constrain(trimindex, 0, 42);  
 
  phaseindex = analogRead(pot3); // read the potentiometer analog input pin:  
  phaseindex = map(phaseindex, 0, 1000, 0, 22); // map pot input to array size
  phaseindex = constrain(phaseindex, 0, 22);  
   
  
  if (buttonState1 != lastButtonState1) { // compare the buttonState to its previous state

    if (buttonState1 == HIGH) {
      ++waveindex;

      if (waveindex >= 2) {
        waveindex = 0;
      }
    }
  }

  if (buttonState2 != lastButtonState2) { // compare the buttonState to its previous state

    if (buttonState2 == HIGH) {
      ++signalindex;

      if (signalindex >= 2) {
        signalindex = 0;
      }
    }
  }

  if (buttonState3 != lastButtonState3) { // compare the buttonState to its previous state

    if (buttonState3 == HIGH) {
      ++harmonicindex;

      if (harmonicindex >= 7) {
        harmonicindex = 0;
      }
    }
  }

//  if (buttonState4 != lastButtonState4) { // compare the buttonState to its previous state
//
//    if (buttonState4 == HIGH) {
//      ++phaseindex;
//
//      if (phaseindex >= 23) {
//        phaseindex = 0;
//      }
//    }
//  }  


  int WS = wave[waveindex]; // sine or triangle
  int FS = freq[freqindex]; // 370hz - 1174hz
  int PS = phas[phaseindex]; // 0-360 degrees
  int SG = sgnl[signalindex]; // on/off
  int HM = hmon[harmonicindex]; // harmonic multiplier
  int TM = trim[trimindex]; // harmonic frequency trim

  if (waveindex != lastwaveindex) {
    gen.ApplySignal(WS, REG0, FS);
    gen2.ApplySignal(WS, REG0, HM*FS-TM, PS);    
  }

  if (freqindex != lastfreqindex) {
    gen.ApplySignal(WS, REG0, FS);
    gen2.ApplySignal(WS, REG0, HM*FS-TM, PS);       
  }

  if (signalindex != lastsignalindex) {
    gen.EnableOutput(SG);   // Turn ON the output - it defaults to OFF
    gen2.EnableOutput(SG);   // Turn ON the output - it defaults to OFF      
  }

  if (harmonicindex != lastharmonicindex) {
    gen.ApplySignal(WS, REG0, FS);    
    gen2.ApplySignal(WS, REG0, HM*FS-TM, PS);       
  }

  if (trimindex != lasttrimindex) {
    gen.ApplySignal(WS, REG0, FS);
    gen2.ApplySignal(WS, REG0, HM*FS-TM, PS);       
  }  

    if (phaseindex != lastphaseindex) {
    gen.ApplySignal(WS, REG0, FS);
    gen2.ApplySignal(WS, REG0, HM*FS-TM, PS);       
  }  

  delay(20);

//Serial.print (sgnl[signalindex], wave[waveindex], freq[freqindex], hmon[harmonicindex], trim[trimindex], phas[phaseindex]);

  lastButtonState1 = buttonState1; // save the current state as the last state, for next time through the loop
  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;  
//  lastButtonState4 = buttonState4; 
  lastfreqindex = freqindex;
  lastwaveindex = waveindex;
  lastharmonicindex = harmonicindex; 
  lastsignalindex = signalindex;
  lasttrimindex = trimindex;
  lastphaseindex = phaseindex;

} // loop
