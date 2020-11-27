/*  
    DDS 3 Channel Waveform Generator
        Nano Every
        3 x AD9833 signal generators SPI
        MPC4231 Digital Potentiometer SPI - controls volume levels of generators 2 and 3
        OPA1656ID audio op amp
    Ins:
        1 x 4 membrane keypad
            1 = reset generator frequency trims, volumes and triangle/square wave settings
            2 = step between different combinations of triangle/sine waves on each generator
            3 = step fundamental frequency up a semitone (cycles between an array of even tempered frequencies based on 440Hz)
            4 = step between different oscillator preset chords and more (3OSC is all generators at the same freq)
        Rotary Encoder
            Rotate CW = increase generator frequency or volume
            Rotate CCW = decrease generator frequency or volume
            Short Press = step between different sensitivities for freq/vol adjust
            Long Press = step between controlling fre_B, fre_C, vol_B, vol_C
    Outs:
        128 x 64 OLED Screen i2C
        Mono 3.5mm audio Minijack
*/

#include <AD9833.h>

#include <math.h>

#include <Keypad.h>

#include <SimpleRotary.h>

#include <ss_oled.h>

#include <McpDigitalPot.h>

#include <SPI.h>

/* pins for AD9833 gens */
#define FNC_PIN 5     
#define FNC_PIN2 4
#define FNC_PIN3 6 
    
/* rotary encoder */
SimpleRotary rotary(9,8,7); // pins for rotary encoder
int rDir = 0;
int rBtn = 0;
byte lastDir = 0;
int osc_change = 0; // which generator is being adjusted
double sense_rot []= {0.1, 1, 10}; // multiplier for increase in value
int sensitivity = 0; // index to sense_rot
bool rot = 0;

/* Keypad stuff */
const byte ROWS = 1; // Define keypad 1 row
const byte COLS = 4; //4 columns
char keys[ROWS][COLS] = {{'1','2','3','4'}};
byte rowPins[ROWS] = {A6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A0, A1, A2, A3}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char key = 0;

/* oled stuff */
#define SDA_PIN -1 // A4
#define SCL_PIN -1 // A5
#define RESET_PIN -1 
#define OLED_ADDR -1
#define FLIP180 0
#define INVERT 0
#define USE_HW_I2C 1 // Use the default Wire library
SSOLED ssoled;

/* Digital Potentiometer */
#define MCP_DIGITAL_POT_SLAVE_SELECT_PIN 10 
McpDigitalPot digitalPot = McpDigitalPot( MCP_DIGITAL_POT_SLAVE_SELECT_PIN, 10000.00 );

/* Global Variables */

/* Fundamental frequencies for chords */
double fun_array[12] = { 220.000000, 233.081881, 246.941651, 261.625565, 277.182631, 293.664768, 
    311.126984, 329.627557, 349.228231, 369.994423, 391.995436, 415.304698};

/* Strings for oled display */
int chord_name1 [10] = {"Maj ", "Min ", "Maj7", "Min7", "Dim ", "Dim7", "Aug ", "Aug7", "3OSC", "HIGH"};
int fun_name [12] = {"A ","A#","B ","C ","C#","D ","D#","E ","F ","F#","G ","G#",};

int chord = 8; // initialise to OSC3 for oscillator test and splash screen
int fundamental_f = 0;

double fre_A = 220.0; // main frequencies for ad9833s
double fre_B = 226.7;
double fre_C = 211.1;

double trim_B = 0.0; // frequency control for gens C and B
double trim_C = 0.0;
double shift1 = 0;
double shift2 = 0;
double trim = 0.0;
double trim2 = 0.0;

int vol_B = 100; // volume control for gens C and B
int vol_C = 100;
int lastvol_B = 100;
int lastvol_C = 100;

int hwave[2] = {SINE_WAVE, TRIANGLE_WAVE};  // wave selection for gens
int wave_A = 0;
int wave_B = 1;
int wave_C = 0;
int gen_waves = 0;

/* debugging */
const double semitone =  1.0; //1.059463;
const double tune_gen2 = 1.0;
const double tune_gen3 = 1.0; // .9925;

/* create ad9833 instances */
AD9833 gen(FNC_PIN); 
AD9833 gen2(FNC_PIN2);
AD9833 gen3(FNC_PIN3); 

void setup() {

    gen.Begin(); 
    gen2.Begin(); 
    gen3.Begin();
    gen.ApplySignal(hwave[wave_A],REG0,fre_A,0); 
    gen2.ApplySignal(hwave[wave_B],REG0,((fre_B-tune_gen2)+trim_B),0); // *.9925
    gen3.ApplySignal(hwave[wave_C],REG0,((fre_C-tune_gen3)+trim_C),0); 

    SPI.begin(); 

    rotary.setDebounceDelay(10);
    rotary.setErrorDelay(50);
 
    // oled stuff // Standard HW I2C bus at 400Khz
    oledInit(&ssoled, OLED_128x64, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L);  

    // splash and initialisation
    /*Digital Potentiometer initialise volume*/
    digitalPot.scale = 100.0; 
    digitalPot.setResistance(0, 100);
    digitalPot.setResistance(1, 100);

    splash_screen ();

    chord = 0; // set chord and waves for main screen
    wave_B = 0;
    wave_C = 0;
    fundamental_f = 0; 
    play_chord (fun_array [fundamental_f] , chord);
    update_oled (); // main screen
   
}

void loop(){
    
    rDir = rotary.rotate();
    if (rDir) {
        check_rotary_dial ();
    }

    rBtn = rotary.pushType(500);
    if (rBtn) {
        check_rotary_btn ();
    }
    
    key = keypad.getKey();
    if (key){
        check_keypad (key);
    }
   
}

void check_rotary_dial () {
    switch (rDir) {
        case 1:
            if (osc_change == 0){
                trim=trim-(1*sense_rot[sensitivity]);
                trim_B = (trim*semitone);
                update_fre_B ();
                update_osc ();
                gen2.ApplySignal(hwave[wave_B],REG0,((fre_B-tune_gen2)+trim_B),0); // *.9925
            }
            if (osc_change == 1){
                trim2=trim2-(1*sense_rot[sensitivity]);
                trim_C = (trim2*semitone);
                update_fre_C ();
                update_osc ();
                gen3.ApplySignal(hwave[wave_C],REG0,((fre_C-tune_gen3)+trim_C),0);
            }
            if (osc_change == 2){
                vol_B = vol_B - 5;
                if (vol_B <= 0){
                    vol_B = 0;
                }
                vol_B = constrain (vol_B, 0, 100);
                if (vol_B != lastvol_B){
                    digitalPot.setResistance(0, vol_B);
                    update_vol ();
                }
                lastvol_B = vol_B;
            }
            if (osc_change == 3){
                
                vol_C = vol_C - 5;
                if (vol_C <= 0){
                    vol_C = 0;
                }
                vol_C = constrain (vol_C, 0, 100);
                if (vol_C != lastvol_C){
                    digitalPot.setResistance(1, vol_C);
                    update_vol ();
                }
                lastvol_C = vol_C;
            }
        break;
        case 2:
            if (osc_change == 0){
                trim=trim+(1*sense_rot[sensitivity]);
                trim_B = (trim*semitone);
                update_osc ();
                update_fre_B ();
                gen2.ApplySignal(hwave[wave_B],REG0,((fre_B-tune_gen2)+trim_B),0); // *.9925
            }
            if (osc_change == 1){
                trim2=trim2+(1*sense_rot[sensitivity]);
                trim_C = (trim2*semitone);
                update_osc ();
                update_fre_C ();
                gen3.ApplySignal(hwave[wave_C],REG0,((fre_C-tune_gen3)+trim_C),0);
            }
            if (osc_change == 2){
                vol_B = vol_B + 5;
                if (vol_B >= 101){
                    vol_B = 100;
                }
                vol_B = constrain (vol_B, 0, 100);
                if (vol_B != lastvol_B){
                    update_vol (); 
                    digitalPot.setResistance(0, vol_B);
                }
                lastvol_B = vol_B;
            }
            if (osc_change == 3){
                
                vol_C = vol_C + 5;
                if (vol_C >= 101){
                    vol_C = 100;
                }
                vol_C = constrain (vol_C, 0, 100);
                if (vol_C != lastvol_C){
                    update_vol (); 
                    digitalPot.setResistance(1, vol_C);
                }
                lastvol_C = vol_C;
            }
        break;
    }
    return;
}

void check_rotary_btn () {
    switch (rBtn){
        case 2:
            osc_change++;
            if (osc_change >= 4){
                osc_change = 0;
                sensitivity = 0;
            }
            lastDir = rDir;   
            update_osc ();
        break;

        case 1:
            sensitivity++;
            if (sensitivity >= 3){
                sensitivity = 0;
            }
        break;
    }
}

void check_keypad (int key1){
    switch (key1){
        case '1': {  
            chord++;
            if (chord>=10){
                chord=0;
            }
            play_chord (fun_array [fundamental_f] , chord);
        break;

        case '2': 
            fundamental_f++;
            if (fundamental_f==12){
                fundamental_f=0;
            }
            play_chord (fun_array [fundamental_f] , chord);   
        break;

        case '3':  
            trim=0;
            trim2=0;
            trim_B = 0;
            trim_C = 0;
            sensitivity = 0;
            gen_waves = 0;
            wave_A = 0;
            wave_B = 0;
            wave_C = 0;
            osc_change = 0;
            vol_B = 100;
            vol_C = 100;
            digitalPot.setResistance(0, vol_B);
            digitalPot.setResistance(1, vol_C);
            update_oled ();
            play_chord (fun_array [fundamental_f] , chord); 
        break;

        case '4':
            gen_waves++;
            if (gen_waves>=8){
                gen_waves=0;
            }

            if (gen_waves==1){
                wave_A = 0;
                wave_B = 0;
                wave_C = 1;   
            }
            if (gen_waves==2){
                wave_A = 0;
                wave_B = 1;
                wave_C = 0;   
            }
            if (gen_waves==3){
                wave_A = 1;
                wave_B = 0;
                wave_C = 0;  
            }
            if (gen_waves==4){
                wave_A = 0;
                wave_B = 1;
                wave_C = 1; 
            }
            if (gen_waves==5){
                wave_A = 1;
                wave_B = 1;
                wave_C = 0; 
            }
            if (gen_waves==6){
                wave_A = 1;
                wave_B = 0;
                wave_C = 1; 
            }
            if (gen_waves==7){
                wave_A = 1;
                wave_B = 1;
                wave_C = 1;   
            }
            if (gen_waves==0){
                wave_A = 0;
                wave_B = 0;
                wave_C = 0;   
            }
            update_wave ();
            play_chord (fun_array [fundamental_f] , chord);
        break;
        }
    }
    return;
}

void play_chord (double fundamental, int chord_name){ 

    if (chord_name == 0) {      // Major 
        shift1 = 4;
        shift2 = 7;
    }
    if (chord_name == 1) {      // "Minor"
        shift1 = 3;
        shift2 = 7;
    }
    if (chord_name == 2) {      //, ""Major7"
        shift1 = 3;
        shift2 = 11;
    }
    if (chord_name == 3) {      //"Minor7"
        shift1 = 7;
        shift2 = 10;
    }
    if (chord_name == 4) {      //"Diminished"
        shift1 = 3;
        shift2 = 6;
    }
    if (chord_name == 5) {      //"Diminished7"
        shift1 = 3;
        shift2 = 9;
    }
    if (chord_name == 6) {      //"Augmented"
        shift1 = 4;
        shift2 = 8;
    }
    if (chord_name == 7) {      // "Augmented7"
        shift1 = 4;
        shift2 = 10;
    }
    if (chord_name == 8) {      // "3 OSC"
        shift1 = 0;
        shift2 = 0;
    }
    if (chord_name == 9) {      // "Shift"
        shift1 = 7;
        shift2 = 14;
    }
    fre_A = fundamental;
    fre_B = pow(double (2),double (shift1/12))*fundamental;
    fre_C = pow(double (2),double (shift2/12))*fundamental;
    gen.ApplySignal(hwave[wave_A],REG0,fre_A,0); 
    gen2.ApplySignal(hwave[wave_B],REG0,((fre_B*tune_gen2)+trim_B),0); // *.9925
    gen3.ApplySignal(hwave[wave_C],REG0,((fre_C*tune_gen3)+trim_C),0);
    update_oled ();
    return ;
}

void update_oled (){ // updates whole screen
    update_fre ();
    update_osc ();
    update_vol (); 
    update_wave ();
    oledWriteString(&ssoled, 0, 0, 2, fun_name [fundamental_f] , FONT_STRETCHED, 0, 1);
    oledWriteString(&ssoled, 0, 43, 2, chord_name1 [chord] , FONT_STRETCHED, 0, 1);  
    return;  
}

void update_wave (){ // updates wave stars
    switch (gen_waves) {
        case 0:
        oledWriteString(&ssoled, 0, 124, 2,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)" ", FONT_SMALL, 0, 1); 
        break;
        case 1:
        oledWriteString(&ssoled, 0, 124, 2,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)" ", FONT_SMALL, 0, 1); 
        break;
        case 2:
        oledWriteString(&ssoled, 0, 124, 2,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)" ", FONT_SMALL, 0, 1); 
        break;
        case 3:
        oledWriteString(&ssoled, 0, 124, 2,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)"*", FONT_SMALL, 0, 1); 
        break;
        case 4:
        oledWriteString(&ssoled, 0, 124, 2,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)" ", FONT_SMALL, 0, 1);
        break;
        case 5:
        oledWriteString(&ssoled, 0, 124, 2,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)"*", FONT_SMALL, 0, 1);
        break;
        case 6:
        oledWriteString(&ssoled, 0, 124, 2,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)" ", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)"*", FONT_SMALL, 0, 1);
        break;
        case 7:
        oledWriteString(&ssoled, 0, 124, 2,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 3,(char *)"*", FONT_SMALL, 0, 1); 
        oledWriteString(&ssoled, 0, 124, 4,(char *)"*", FONT_SMALL, 0, 1);
        break;
    }
    return;
}

void update_osc () { // updates bottom line

    if ((trim_B <= 1.6) && (trim_B >= -1.6)) {
      oledWriteString(&ssoled, 0, 43, 7,(char *)"OSCB", FONT_NORMAL, 1, 1); 
    }
    else { 
       oledWriteString(&ssoled, 0, 43, 7,(char *)"OSCB", FONT_NORMAL, 0, 1); 
    }
    
    if ((trim_C <= 1.6) && (trim_C >= -1.6)) {
     oledWriteString(&ssoled, 0, 86, 7,(char *)"OSCC", FONT_NORMAL, 1, 1);  
    }
    else {
      oledWriteString(&ssoled, 0, 86, 7,(char *)"OSCC", FONT_NORMAL, 0, 1); 
    }

    if (osc_change == 0) {
        oledWriteString(&ssoled, 0, 0, 7,(char *)"FR:B", FONT_NORMAL, 0, 1);
        // oledWriteString(&ssoled, 0, 76, 7,(char *)"*", FONT_SMALL, 0, 1);
        // oledWriteString(&ssoled, 0, 120, 7,(char *)" ", FONT_SMALL, 0, 1);  
    }

    if (osc_change == 1) {
        oledWriteString(&ssoled, 0, 0, 7,(char *)"FR:C", FONT_NORMAL, 0, 1);
        // oledWriteString(&ssoled, 0, 120, 7,(char *)"*", FONT_SMALL, 0, 1); 
        // oledWriteString(&ssoled, 0, 76, 7,(char *)" ", FONT_SMALL, 0, 1); 
    }
    if (osc_change == 2) {
        oledWriteString(&ssoled, 0, 0, 7,(char *)"VO:B ", FONT_NORMAL, 0, 1);
        // oledWriteString(&ssoled, 0, 76, 7,(char *)"*", FONT_SMALL, 0, 1); 
        // oledWriteString(&ssoled, 0, 120, 7,(char *)" ", FONT_SMALL, 0, 1); 
    }
    if (osc_change == 3) {
        oledWriteString(&ssoled, 0, 0, 7,(char *)"VO:C ", FONT_NORMAL, 0, 1);
        // oledWriteString(&ssoled, 0, 120, 7,(char *)"*", FONT_SMALL, 0, 1);
        // oledWriteString(&ssoled, 0, 76, 7,(char *)" ", FONT_SMALL, 0, 1);  
    }
    return;
}

void update_vol (){ // updates oled volume display for gen

    int vdisplay1 = map(vol_C, 0, 100, 1, 6 );
    int vdisplay2 = map(vol_B, 0, 100, 1, 6 );
    
    switch (vdisplay1) {
         
        case 6:
            oledWriteString(&ssoled, 0, 84, 6,(char *)"...... ", FONT_SMALL, 0, 1); 
        break; 
        case 5:
            oledWriteString(&ssoled, 0, 84, 6,(char *)".....  ", FONT_SMALL, 0, 1);
        break; 
        case 4:
            oledWriteString(&ssoled, 0, 84, 6,(char *)"....   ", FONT_SMALL, 0, 1);
        break; 
        case 3:
            oledWriteString(&ssoled, 0, 84, 6,(char *)"...    ", FONT_SMALL, 0, 1);
        break; 
        case 2:
            oledWriteString(&ssoled, 0, 84, 6,(char *)"..     ", FONT_SMALL, 0, 1);
        break; 
        case 1:
            oledWriteString(&ssoled, 0, 84, 6,(char *)".      ", FONT_SMALL, 0, 1);
        break;
    }
    switch (vdisplay2) {
   
        case 6:
            oledWriteString(&ssoled, 0, 41, 6,(char *)"...... ", FONT_SMALL, 0, 1); 
        break; 
        case 5:
            oledWriteString(&ssoled, 0, 41, 6,(char *)".....  ", FONT_SMALL, 0, 1);
        break; 
        case 4:
            oledWriteString(&ssoled, 0, 41, 6,(char *)"....   ", FONT_SMALL, 0, 1);
        break; 
        case 3:
            oledWriteString(&ssoled, 0, 41, 6,(char *)"...    ", FONT_SMALL, 0, 1);
        break; 
        case 2:
            oledWriteString(&ssoled, 0, 41, 6,(char *)"..     ", FONT_SMALL, 0, 1);
        break; 
        case 1:
            oledWriteString(&ssoled, 0, 41, 6,(char *)".      ", FONT_SMALL, 0, 1);
        break;
    }
    return;
}

void update_fre (){ 
     update_fre_A ();
     update_fre_B ();
     update_fre_C ();
     return;
}

void update_fre_A (){
    char buffer0 [6] = {};
    String str1 = String(fre_A, 2);
    str1.toCharArray(buffer0,6);
    oledWriteString(&ssoled, 0, 0, 5, buffer0, FONT_SMALL, 0, 1);
    return;
}

void update_fre_B (){
    char buffer1 [6] = {};
    String str2 = String(fre_B+trim_B, 2);
    str2.toCharArray(buffer1,6);
    oledWriteString(&ssoled, 0, 43, 5, buffer1, FONT_SMALL, 0, 1); 
    return;
}

void update_fre_C (){
    char buffer2 [6] = {};
    String str3 = String(fre_C+trim_C, 2);
    str3.toCharArray(buffer2,6);
    oledWriteString(&ssoled, 0, 86, 5, buffer2, FONT_SMALL, 0, 1); 
    return;
}

/*  
    Enables AD9833s one by one and displays amusing captions
*/
void splash_screen () {
        oledFill(&ssoled, 0, 1);
        gen.EnableOutput(true); 

        oledWriteString(&ssoled, 0, 0, 7, (char *)"Checking OSCA", FONT_SMALL, 0, 1);
        oledWriteString(&ssoled, 0, 0, 0, (char *)"OPENING:", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID.", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"      ", FONT_STRETCHED, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID...", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"        ", FONT_STRETCHED, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"         ", FONT_STRETCHED, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"         ", FONT_STRETCHED, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"         ", FONT_STRETCHED, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"VOID....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 1, (char *)"         ", FONT_STRETCHED, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 0, (char *)"            ", FONT_SMALL, 0, 1);
        oledWriteString(&ssoled, 0, 0, 7, (char *)"Checking OSCC", FONT_SMALL, 0, 1);
        gen3.EnableOutput(true);
        delay(250);

        oledWriteString(&ssoled, 0, 0, 2, (char *)"INITIALISING SCREAM..", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"INITIALISING SCREAM...", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"INITIALISING SCREAM..", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"INITIALISING SCREAM...", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 2, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 7, (char *)"Checking OSCB", FONT_SMALL, 0, 1);
        gen2.EnableOutput(true);

        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(150);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(150);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(100);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                      ", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"SCREAM INITIALISED....", FONT_SMALL, 0, 1);
        delay(50);
        oledWriteString(&ssoled, 0, 0, 3, (char *)"                       ", FONT_SMALL, 0, 1);
        oledWriteString(&ssoled, 0, 0, 7, (char *)"                 ", FONT_SMALL, 0, 1);
        delay(500);
        oledFill(&ssoled, 0, 1);

        oledWriteString(&ssoled, 0, 0, 0,(char *)"ronedronedronedronedroned", FONT_SMALL, 1, 1); // stays static
        oledWriteString(&ssoled, 0, 0, 7,(char *)"FREQ", FONT_NORMAL, 0, 1);
        oledWriteString(&ssoled, 0, 43, 7,(char *)"OSCB", FONT_NORMAL, 0, 1);
        oledWriteString(&ssoled, 0, 86, 7,(char *)"OSCC", FONT_NORMAL, 0, 1); 
    
        return;
}
