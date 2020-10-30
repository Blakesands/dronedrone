/*
   Library code found at: https://github.com/Billwilliams1952/AD9833-Library-Arduino
*/

#include <AD9833.h>   // Include the library
#define FNC_PIN 4     // Can be any digital IO pin
#define FNC_PIN2 5    // Define another pin for gen2 ad9833
//#define FNC_PIN2 6    // Define another pin for gen3 ad9833

// global variables

// debugging serial output counter
//int count = 0;

// arrays for setting freq range, multiplier, trim, signal on/off, waveform and phase values. 
int sgnl[2] = {false, true};                  // Signal on/off
int wave[2] = {SINE_WAVE, TRIANGLE_WAVE};     // Fundamental frequency waveform not using the square waves  SQUARE_WAVE, HALF_SQUARE_WAVE
int hwave[2] = {SINE_WAVE, TRIANGLE_WAVE};    // Harmonic waveform not using the square waves  SQUARE_WAVE, HALF_SQUARE_WAVE
int freq[21] = {233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740}; // Fundamental frequency
int hmon[7] = {2, 3, 4, 5, 6, 7, 8};          // Harmonic channel hz multiplier
int trim[67] = {-212, -106, -86, -66, -62, -58, -56, -54, -52, -48, -44, -40, -38, -36, -34, -32, -30, -28, 
                -26, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6, 8, 
                10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 44, 48, 52, 56, 58, 62, 64, 66, 86, 106, 212}; //harmonic content Hz trim
int phas[23] = {0, 0.003, 0.006, 0.011, 0.022, 0.045, 0.09, 0.18, 0.36, .72, 1.44, 2.88, 5.72, 11.5, 22.5, 45, 90, 135, 180, 225, 270, 315, 0}; //Phase on harmonics

// Variables for button inputs
const int buttonPin1 = 3;     // the number of the pushbutton pin
int buttonState1 = 0;         // current state of the button
int lastButtonState1 = 0;     // previous state of the button

const int buttonPin2 = 2;     // the number of the pushbutton pin
int buttonState2 = 0;         // current state of the button
int lastButtonState2 = 0;     // previous state of the button

const int buttonPin3 = 6;     // the number of the pushbutton pin
int buttonState3 = 0;         // current state of the button
int lastButtonState3 = 0;     // previous state of the button

const int buttonPin4 = 8;     // the number of the pushbutton pin
int buttonState4 = 0;         // current state of the button
int lastButtonState4 = 0;     // previous state of the button

// Variables for pot inputs
const int pot2 = A5;          // input pin for the potentiometer
const int pot = A0;           // input pin for the potentiometer
const int pot3 = A3;          // input pin for the potentiometer

// Variables for array indexes
int waveindex = 0;            // 0-1 Sine or Triangle Wave
int lastwaveindex = 0;

int harmonicindex = 0;        // Harmonic multiplier for gen2
int lastharmonicindex = 0;

int signalindex = 0;          // 0-1 off/on
int lastsignalindex = 0;

int hwaveindex = 0;           // 0-1 Sine or Triangle Wave
int lasthwaveindex = 0;

int phaseindex = 0;           // phase for gen2
int lastphaseindex = 0;

int freqindex = 0;            // Frequency index
int lastfreqindex = 0;

int trimindex = 0;            // fine trim for harmonics
int lasttrimindex = 0;

//--------------- Create an AD9833 object ----------------
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency
AD9833 gen2(FNC_PIN2);       // Defaults to 25MHz internal reference frequency


void setup() {
  
// This MUST be the first command after declaring the AD9833 object
  gen.Begin();
  gen2.Begin();
  
// debugging waveform generator reset and disable
//  gen.Reset();  
//  gen2.Reset(); 
//  gen.DisableDAC (true);
//  gen.DisableInternalClock (true);

// debugging serial output     
//  Serial.begin(115200);

// define input pins, comment out if not using that pin to avoid floating inputs
  pinMode(buttonPin1, INPUT);
//  pinMode(buttonPin2, INPUT);
//  pinMode(buttonPin3, INPUT);
//  pinMode(buttonPin4, INPUT);
  pinMode(pot, INPUT);
//  pinMode(pot2, INPUT);
  pinMode(pot3, INPUT);

// setup waveform generators
  gen.ApplySignal(TRIANGLE_WAVE, REG0, 440, 0); 
  gen2.ApplySignal(SINE_WAVE, REG0, 440, 0); 
  gen.EnableOutput(true);   // Turn ON the output - it defaults to OFF
  gen2.EnableOutput(true);   // Turn ON the output of gen2 - it defaults to OFF
  //  gen.SetOutputSource(REG1);   // Switch between 2 registers on 1 output (mystery solved)
}

void loop() {
// define local variables
  buttonState1 = digitalRead(buttonPin1); // read the pushbutton input pin: 
  buttonState2 = digitalRead(buttonPin2); // read the pushbutton input pin:
  buttonState3 = digitalRead(buttonPin3); // read the pushbutton input pin:
  buttonState4 = digitalRead(buttonPin4); // read the pushbutton input pin:
    
  freqindex = analogRead(pot); // read the potentiometer analog input pin:
  freqindex = map(freqindex, 0, 1000, 0, 20); // map pot input to array size - doesn't go up to 1023 because of jitter from the pot
  freqindex = constrain(freqindex, 0, 20);    // constrain index number to avoid freak inputs
  
  trimindex = analogRead(pot2); // read the potentiometer analog input pin:  
  trimindex = map(trimindex, 0, 1000, 0, 66); // map pot input to array size
  trimindex = constrain(trimindex, 0, 66);    // constrain index number to avoid freak inputs  
 
  phaseindex = analogRead(pot3); // read the potentiometer analog input pin:  
  phaseindex = map(phaseindex, 0, 1000, 0, 22); // map pot input to array size
  phaseindex = constrain(phaseindex, 0, 22);    // constrain index number to avoid freak inputs
   
// use if statements to see if a button has been pressed or pot value changed and increment the index associated, reset index to 0 if it goes over the array size 
  if (buttonState1 != lastButtonState1) { // compare the buttonState to its previous state

    if (buttonState1 == HIGH) {
      ++waveindex; // increment wave index

      if (waveindex >= 2) {
        waveindex = 0; // can be 0 or 1 (change this to 4 and add square wave values into wave array to use square waves - these output at 4x the voltage so beware)
      }
    }
  }

  if (buttonState2 != lastButtonState2) { // compare the buttonState to its previous state

    if (buttonState2 == HIGH) {
      ++signalindex;  // increment signal index

      if (signalindex >= 2) {
        signalindex = 0; // can be 0 or 1 off/on
      }
    }
  }

  if (buttonState3 != lastButtonState3) { // compare the buttonState to its previous state

    if (buttonState3 == HIGH) {
      ++harmonicindex;   // increment harmonic multiplier index

      if (harmonicindex >= 7) {
        harmonicindex = 0; // 0 = x1, 1 = x2, 3 = x4 etc
      }
    }
  }

  if (buttonState4 != lastButtonState4) { // compare the buttonState to its previous state

    if (buttonState4 == HIGH) {
      ++hwaveindex; // increment harmonic wave index

      if (hwaveindex >= 2) {
        hwaveindex = 0;
      }
    }
  }  

// use defined indexes to look up array values 
  int WS = wave[waveindex]; // sine or triangle
  int FS = freq[freqindex]; // base frequency
  int PS = phas[phaseindex]; // harmonic phase 0-360 degrees
  int SG = sgnl[signalindex]; // on/off
  int HM = hmon[harmonicindex]; // harmonic multiplier
  int TM = trim[trimindex]; // harmonic frequency trim
  int HW = hwave[hwaveindex]; // harmonic waveform

// send array values to waveform generators if they have changed 
  if   (signalindex != lastsignalindex) {
    gen.EnableOutput(SG);   // Turn ON/OFF the output - it defaults to OFF
    gen2.EnableOutput(SG);        
  }
  
  if ((waveindex != lastwaveindex) || (freqindex != lastfreqindex) 
      || (harmonicindex != lastharmonicindex) || (trimindex != lasttrimindex) 
      || (phaseindex != lastphaseindex) || (hwaveindex != lasthwaveindex) ) {
    gen.ApplySignal(WS, REG0, FS-TM); // remove the -TM in this line (added for testing purposes)
    gen2.ApplySignal(HW, REG0, HM*FS-TM,PS);    
  }      
  
// debugging serial output
//  if (count > 10) { // 10 x 20ms delay = update serial 5/s
//  Serial.println(count);    
//  Serial.println(sgnl[signalindex]);
//  Serial.println(wave[waveindex]);
//  Serial.println(freq[freqindex]);
//  Serial.println(hmon[harmonicindex]);
//  Serial.println(trim[trimindex]);
//  Serial.println(phas[phaseindex]);
//  count = 0;
//  } 

// save the current button state as the last state, for next time through the loop 
  lastButtonState1 = buttonState1; 
  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;  
  lastButtonState4 = buttonState4; 

// save the current index state as the last state, for next time through the loop  
  lastfreqindex = freqindex;
  lastwaveindex = waveindex;
  lastharmonicindex = harmonicindex; 
  lastsignalindex = signalindex;
  lasttrimindex = trimindex;
  lastphaseindex = phaseindex;

// delay a bit and debugging increment counter
  delay(20);
//  ++count; 

} // end of loop

// Cheers!
