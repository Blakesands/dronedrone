/*
   ApplySignal.ino
   2018 WLWilliams
   Library code found at: https://github.com/Billwilliams1952/AD9833-Library-Arduino

*/

#include <AD9833.h>     // Include the library
#define FNC_PIN 5       // Can be any digital IO pin

// My code

#define FNC_PIN2 4       // Define another pin for separate ad9833
#define DATA  11  ///< SPI Data pin number
#define CLK   13  ///< SPI Clock pin number

int X = 0;  // 0-1 Sine or Triangle Wave
int Y = 0; // 0-20 Frequencies
int Z = 0;  // 0-3 Phase
int A = 0; // 0-1 off/on
int H = 0; // Harmonic for gen2

int wave[4] = {SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, HALF_SQUARE_WAVE}; // not using the square waves
int freq[21] = {330, 330, 330, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 927, 1024, 2048, 4096, 4096, 4096};
int phas[4] = {0, 90, 180, 270};
int sgnl[2] = {false, true};
int hmon[6] = {2, 3, 4, 5, 6, 7}; // add harmonic content on second ad9833

// button code
const int buttonPin1 = 3;     // the number of the pushbutton pin
int buttonState1 = 0;         // current state of the button
int lastButtonState1 = 0;     // previous state of the button

const int buttonPin2 = 2;     // the number of the pushbutton pin
int buttonState2 = 0;         // current state of the button
int lastButtonState2 = 0;     // previous state of the button

const int buttonPin3 = 6;     // the number of the pushbutton pin
int buttonState3 = 0;         // current state of the button
int lastButtonState3 = 0;     // previous state of the button

const int pot = A0;    // select the input pin for the potentiometer
int potV = 0;  // variable to store the value coming from the sensor
int lastpotV = 0;


//--------------- Create an AD9833 object ----------------
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency
AD9833 gen2(FNC_PIN2);       // Defaults to 25MHz internal reference frequency
void setup() {
  // This MUST be the first command after declaring the AD9833 object
  gen.Begin();
  gen2.Begin();


  // button code
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  pinMode(pot, INPUT);

  // Apply a 1000 Hz sine wave using REG0 (register set 0). There are two register sets,
  // REG0 and REG1.
  // Each one can be programmed for:
  //   Signal type - SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, and HALF_SQUARE_WAVE
  //   Frequency - 0 to 12.5 MHz
  //   Phase - 0 to 360 degress (this is only useful if it is 'relative' to some other signal
  //           such as the phase difference between REG0 and REG1).
  // In ApplySignal, if Phase is not given, it defaults to 0.


  gen.ApplySignal(SINE_WAVE, REG0, 440, 0); // defaults to 1000hz Sine on R0
  gen2.ApplySignal(TRIANGLE_WAVE, REG0, 880, 0); // defaults to 1000hz Sine on R0
  gen.EnableOutput(true);   // Turn ON the output - it defaults to OFF
  gen2.EnableOutput(true);   // Turn ON the output of gen2 - it defaults to OFF
  //  gen.SetOutputSource(REG1);   // Switch between 2 registers on 1 output (mystery solved)
}

void loop() {

  int WS = wave[X]; // 0-3
  int FS = freq[Y]; // 0-11
  int PS = phas[Z]; // 0-2
  int SG = sgnl[A]; // 0-1
  int HM = hmon[H]; // harmonic multiplier

  buttonState1 = digitalRead(buttonPin1); // read the pushbutton input pin:
  buttonState2 = digitalRead(buttonPin2); // read the pushbutton input pin:
  potV = analogRead(pot); // read the potentiometer analog input pin:
  buttonState3 = digitalRead(buttonPin3); // read the pushbutton input pin:
 
  potV = map(potV, 0, 1023, 1, 21);
  Y = potV;
//  Y = constrain(Y, 0, 21);  // limits range of sensor values to between 10 and 150

  if (buttonState1 != lastButtonState1) { // compare the buttonState to its previous state

    if (buttonState1 == HIGH) {
      ++X;

      if (X >= 4) {
        X = 0;
      }
    }
  }

  if (buttonState2 != lastButtonState2) { // compare the buttonState to its previous state

    if (buttonState2 == HIGH) {
      ++A;

      if (A >= 2) {
        A = 0;
      }
    }
  }

  if (buttonState3 != lastButtonState3) { // compare the buttonState to its previous state

    if (buttonState3 == HIGH) {
      ++H;

      if (H >= 7) {
        H = 0;
      }
    }
  }

  if (buttonState1 != lastButtonState1) {
    gen.ApplySignal(WS, REG0, FS, PS);
    gen2.ApplySignal(WS, REG0, HM*FS, PS);    
  }

  if (potV != lastpotV) {
    gen.ApplySignal(WS, REG0, FS, PS);
    gen2.ApplySignal(WS, REG0, HM*FS, PS);       
  }

  if (buttonState2 != lastButtonState2) {
    gen.EnableOutput(SG);   // Turn ON the output - it defaults to OFF
    gen2.EnableOutput(SG);   // Turn ON the output - it defaults to OFF      
  }


  if (buttonState3 != lastButtonState3) {
    gen.ApplySignal(WS, REG0, FS, PS);    
    gen2.ApplySignal(WS, REG0, HM*FS, PS);       
  }

  delay(50);

  lastButtonState1 = buttonState1; // save the current state as the last state, for next time through the loop
  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;  
  lastpotV = potV;

} // loop
