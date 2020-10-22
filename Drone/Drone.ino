/*
 * ApplySignal.ino
 * 2018 WLWilliams
 * Library code found at: https://github.com/Billwilliams1952/AD9833-Library-Arduino
 * 
 */

#include <AD9833.h>     // Include the library
#define FNC_PIN 4       // Can be any digital IO pin

// My code

#define DATA  11  ///< SPI Data pin number
#define CLK   13  ///< SPI Clock pin number

int X = 0;  // 0-3
int Y = 0; // 0-11
int Z = 0;  // 0-2
int A = 0; // 0-1

int wave[4] = {SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, HALF_SQUARE_WAVE}; // not using the square waves
int freq[18] = {160, 330, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 927, 1024, 2000, 3000};
int phas[4] = {0, 90, 180, 270};
int sigon[2] = {false, true};


// button code
const int buttonPin1 = 3;     // the number of the pushbutton pin
int buttonState1 = 0;         // current state of the button
int lastButtonState1 = 0;     // previous state of the button

const int buttonPin2 = 2;     // the number of the pushbutton pin
int buttonState2 = 0;         // current state of the button
int lastButtonState2 = 0;     // previous state of the button

const int pot = 0;    // select the input pin for the potentiometer 
int potV = 0;  // variable to store the value coming from the sensor
int lastpotV = 0;


//--------------- Create an AD9833 object ---------------- 
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency

void setup() {
    // This MUST be the first command after declaring the AD9833 object
    gen.Begin(); 


                 
// button code
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(pot, INPUT);

  // Apply a 1000 Hz sine wave using REG0 (register set 0). There are two register sets,
    // REG0 and REG1. 
    // Each one can be programmed for:
    //   Signal type - SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, and HALF_SQUARE_WAVE
    //   Frequency - 0 to 12.5 MHz
    //   Phase - 0 to 360 degress (this is only useful if it is 'relative' to some other signal
    //           such as the phase difference between REG0 and REG1).
    // In ApplySignal, if Phase is not given, it defaults to 0.

 
    gen.ApplySignal(SINE_WAVE,REG0,1000); // defaults to 1000hz Sine on R0
    gen.EnableOutput(false);   // Turn ON the output - it defaults to OFF
//    gen.SetOutputSource(REG1);   // Switch between registers on out puts (mystery solved)
}

void loop() {
  
  int WS = wave[X]; // 0-3
  int FS = freq[Y]; // 0-11
  int PS = phas[Z]; // 0-2
  int SG = sigon[A]; // 0-1
  
  buttonState1 = digitalRead(buttonPin1); // read the pushbutton input pin:
  buttonState2 = digitalRead(buttonPin2); // read the pushbutton input pin:
  potV = analogRead(pot); // read the potentiometer analog input pin:

  potV = map(potV, 0, 1023, 1, 17); 
  Y = potV; 
  
  if (buttonState1 != lastButtonState1) { // compare the buttonState to its previous state
    
    if (buttonState1 == HIGH) {
      ++X; 
      
      if (X >= 4){X = 0;
      }
    }
  } 

  if (buttonState2 != lastButtonState2) { // compare the buttonState to its previous state
    
    if (buttonState2 == HIGH) {
      ++A; 
      
      if (A >= 2){A = 0;
      }
    }
  } 
     
  if (buttonState1 != lastButtonState1) {
       gen.ApplySignal(WS,REG0,FS,PS);
  }
              
  if (potV != lastpotV) {
      gen.ApplySignal(WS,REG0,FS,PS);
  } 
  
  if (buttonState2 != lastButtonState2) {
        gen.EnableOutput(SG);   // Turn ON the output - it defaults to OFF
  } 
    
  // Delay a little bit to avoid bouncing
  delay(50);

  // save the current state as the last state, for next time through the loop
  lastButtonState1 = buttonState1;
  lastButtonState2 = buttonState2;
  lastpotV = potV;
  
} // loop
