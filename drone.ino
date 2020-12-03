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
#define ROTARY_LONG_PRESS 2
#define ROTARY_SHORT_PRESS 1
SimpleRotary rotary(9,8,7); // pins for rotary encoder

double rotary_sens []= {0.1, 1, 10}; // multiplier for increase in value as pot turns
int SENSE = 0; // index to rotary_sens

#define frequency_B 0
#define frequency_C 1
#define volume_B 2
#define volume_C 3
#define frequency_A 4

int gen_param = 4; // which generator/value is being adjusted 

/* Keypad stuff */
#define ROWS 1 // Define keypad 1 row
#define COLS 4 //4 columns
char keys[ROWS][COLS] = {{'1','2','3','4'}};
byte rowPins[ROWS] = {A6}; 
byte colPins[COLS] = {A0, A1, A2, A3}; 
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
double fun_array[15] = { 220.000000, 233.081881, 246.941651, 261.625565, 277.182631, 293.664768, 
    311.126984, 329.627557, 349.228231, 369.994423, 391.995436, 415.304698, 440.0, 110.0, 55.0 };

/* Strings for oled display */
char* chord_name1 [10] = {"Maj ", "Min ", "Maj7", "Min7", "Dim ", "Dim7", "Aug ", "Aug7", "3OSC", "HIGH"};
char* fun_name [15] = {"A3","A#","B ","C ","C#","D ","D#","E ","F ","F#","G ","G#", "A4", "A2", "A1"};

int chord = 8; // initialise to OSC3 for oscillator test and splash screen
int fundamental_f = 0;

double fre_A = 220.0; // main frequencies for ad9833s
double fre_B = 226.7;
double fre_C = 211.1;
double trim_A = 0.0;  // frequency trim control for gens C and B
double trim_B = 0.0;
double trim_C = 0.0;
double shift1 = 0; // plays chords relative to fre_A
double shift2 = 0;

int vol_B = 100; // volume control for gens C and B
int vol_C = 100;
int lastvol_B = 100;
int lastvol_C = 100;

int hwave[2] = {SINE_WAVE, TRIANGLE_WAVE};  // wave selection for gens
int wave_index = 0;
//                  0  1  2  3  4  5  6  7
int wave_on_A[8] = {0, 0, 0 ,1, 0, 1, 1, 1};
int wave_on_B[8] = {0, 0, 1 ,0, 1, 1, 0, 1};
int wave_on_C[8] = {0, 1, 0 ,0, 1, 0, 1, 1};
int wave_A = 0;
int wave_B = 1;
int wave_C = 0;

// /* debugging */
// const double semitone =  1.0; //1.059463;
// const double tune_gen2 = 1.0;
// const double tune_gen3 = 1.0; // .9925;

/* create ad9833 instances */
AD9833 gen(FNC_PIN); 
AD9833 gen2(FNC_PIN2);
AD9833 gen3(FNC_PIN3); 

void setup() {

    gen.Begin(); 
    gen2.Begin(); 
    gen3.Begin();
    gen.ApplySignal(hwave[wave_A],REG0,(fre_A+trim_A),0); 
    gen2.ApplySignal(hwave[wave_B],REG0,(fre_B+trim_B),0); // *.9925
    gen3.ApplySignal(hwave[wave_C],REG0,(fre_C+trim_C),0); 

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

// TODO setup timer and interrups for lfo
    // OCR0A = 0xAF;
    // TIMSK0 |= _BV(OCIE0A);

}

void loop(){
    
    int rotary_dial = rotary.rotate();
    if (rotary_dial) {
        check_rotary_dial (rotary_dial);
    }

    char keypress = keypad.getKey();
    if (keypress){
        check_keypad (keypress);
    }

    int rotary_button = rotary.pushType(500);
    if (rotary_button) {
        check_rotary_btn (rotary_button);
    }
   
}

void check_rotary_dial (int rotary_dial) {
    switch (rotary_dial) {
        case 1:
            dialTurnRight();
            break;
        case 2:
            dialTurnLeft();
            break;
    }
}

void dialTurnLeft() {
    switch (gen_param) {
        case frequency_A:
            trim_A=trim_A-(1*rotary_sens[SENSE]);
            gen.ApplySignal(hwave[wave_B],REG0,(fre_A+trim_A),0); // *.9925
            oled_freq_A ();
            oled_osc ();
        break;
        case frequency_B:
            trim_B=trim_B-(1*rotary_sens[SENSE]);
            gen2.ApplySignal(hwave[wave_B],REG0,(fre_B+trim_B),0); // *.9925 -tune_gen2)
            oled_freq_B ();
            oled_osc ();
        break;
        case frequency_C:
            trim_C=trim_C-(1*rotary_sens[SENSE]);
            gen3.ApplySignal(hwave[wave_C],REG0,(fre_C+trim_C),0);
            oled_freq_C ();
            oled_osc ();
        break;
        case volume_B:
            vol_B = vol_B - 5;
            if (vol_B <= 0){
                vol_B = 0;
            }
            vol_B = constrain (vol_B, 0, 100);
            if (vol_B != lastvol_B){
                digitalPot.setResistance(0, vol_B);
                oled_vol ();
            }
            lastvol_B = vol_B; //needed??
        break;
        case volume_C:
            vol_C = vol_C - 5;
            if (vol_C <= 0){
                vol_C = 0;
            }
            vol_C = constrain (vol_C, 0, 100);
            if (vol_C != lastvol_C){
                digitalPot.setResistance(1, vol_C);
                oled_vol ();
            }
            lastvol_C = vol_C;
        break;
    }
}

void dialTurnRight() {
    switch (gen_param) {
        case frequency_A:
            trim_A=trim_A+(1*rotary_sens[SENSE]);
            gen.ApplySignal(hwave[wave_B],REG0,(fre_A+trim_A),0); // *.9925
            oled_freq_A ();
            oled_osc ();
        break;
        case frequency_B:
            trim_B=trim_B+(1*rotary_sens[SENSE]);
            gen2.ApplySignal(hwave[wave_B],REG0,(fre_B+trim_B),0); // *.9925 -tune_gen2)
            oled_freq_B ();
            oled_osc ();
        break;
        case frequency_C:
            trim_C=trim_C+(1*rotary_sens[SENSE]);
            gen3.ApplySignal(hwave[wave_C],REG0,(fre_C+trim_C),0);
            oled_freq_C ();
            oled_osc ();
        break;
        case volume_B:
            vol_B = vol_B + 5;
            if (vol_B >= 101){
                vol_B = 100;
            }
            vol_B = constrain (vol_B, 0, 100);
            if (vol_B != lastvol_B){
                oled_vol (); 
                digitalPot.setResistance(0, vol_B);
            }
            lastvol_B = vol_B;
        break;
        case volume_C:
            vol_C = vol_C + 5;
            if (vol_C >= 101){
                vol_C = 100;
            }
            vol_C = constrain (vol_C, 0, 100);
            if (vol_C != lastvol_C){
                oled_vol (); 
                digitalPot.setResistance(1, vol_C);
            }
            lastvol_C = vol_C;
        break;
    }
     
}

void check_rotary_btn (int rotary_button) {
    switch (rotary_button){
        case ROTARY_SHORT_PRESS:
            gen_param++;
            if (gen_param >= 5){
                gen_param = 0;
            }   
            oled_gen ();
        break;

        case ROTARY_LONG_PRESS:
            SENSE++;
            if (SENSE >= 3){
                SENSE = 0;
            }
        break;
    }
}

void check_keypad (char keypress){
    switch (keypress){
        case '1':  
            chord++;
            if (chord>=10){
                chord=0;
            }
            play_chord (fun_array [fundamental_f] , chord);
        break;
        case '2': 
            fundamental_f++;
            if (fundamental_f>=15){
                fundamental_f=0;
            }
            play_chord (fun_array [fundamental_f] , chord);   
        break;
        case '3':  // reset button
            reset_param ();
        break;
        case '4':
            wave_index++;
            if (wave_index>=8){
                wave_index=0;
            }
            wave_A = wave_on_A[wave_index];
            wave_B = wave_on_B[wave_index];
            wave_C = wave_on_C[wave_index];
            oled_wave ();
            play_chord (fun_array [fundamental_f] , chord);
        break;
    }  
}

void reset_param () {
    trim_A = 0;
    trim_B = 0;
    trim_C = 0;
    wave_index = 0;
    wave_A = 0;
    wave_B = 0;
    wave_C = 0;
    gen_param = 0;
    vol_B = 100;
    vol_C = 100;
    digitalPot.setResistance(0, vol_B);
    digitalPot.setResistance(1, vol_C);
    update_oled ();
    play_chord (fun_array [fundamental_f] , chord); 
}

void play_chord (double fundamental, int chord_name){ 

    int chords[10][2] = {
        {4,7}, // "Major"
        {3,7}, // "Minor"
        {3,11}, // ""Major7"
        {7,10}, // "Minor7"
        {3,6}, // "Diminished"
        {3,9}, // "Diminished7"
        {4,8}, // "Augmented"
        {4,10}, // "Augmented7"
        {0,0}, // "3 OSC"
        {7,14}, // "High"
    };

    int shift1 = chords[chord][0];
    int shift2 = chords[chord][1];
    fre_A = fundamental;
    fre_B = pow(double (2),double (shift1/12))*fundamental;
    fre_C = pow(double (2),double (shift2/12))*fundamental;
    gen.ApplySignal (hwave[wave_A],REG0,(fre_A+trim_A),0); 
    gen2.ApplySignal(hwave[wave_B],REG0,(fre_B+trim_B),0); // *.9925
    gen3.ApplySignal(hwave[wave_C],REG0,(fre_C+trim_C),0);
    update_oled ();
}

void update_oled (){ // updates whole screen
    oled_freq ();
    oled_osc ();
    oled_gen ();
    oled_vol (); 
    oled_wave ();
    oledWriteString(&ssoled, 0, 0, 2, fun_name [fundamental_f] , FONT_STRETCHED, 0, 1);
    oledWriteString(&ssoled, 0, 43, 2, chord_name1 [chord] , FONT_STRETCHED, 0, 1);  
}

void oled_wave (){ // updates wave stars
    char* indicator[2] = {" ", "*"};
    oledWriteString(&ssoled, 0, 124, 2,indicator[wave_on_A[wave_index]], FONT_SMALL, 0, 1); 
    oledWriteString(&ssoled, 0, 124, 3,indicator[wave_on_B[wave_index]], FONT_SMALL, 0, 1); 
    oledWriteString(&ssoled, 0, 124, 4,indicator[wave_on_C[wave_index]], FONT_SMALL, 0, 1);
}

void oled_osc () { // updates bottom line
    if ((trim_B <= 1.6) && (trim_B >= -1.6)) {
      oledWriteString(&ssoled, 0, 43, 7,(char *)"GENB", FONT_NORMAL, 1, 1); 
    }
    else { 
       oledWriteString(&ssoled, 0, 43, 7,(char *)"GENB", FONT_NORMAL, 0, 1); 
    }
    
    if ((trim_C <= 1.6) && (trim_C >= -1.6)) {
     oledWriteString(&ssoled, 0, 86, 7,(char *)"GENC", FONT_NORMAL, 1, 1);  
    }
    else {
      oledWriteString(&ssoled, 0, 86, 7,(char *)"GENC", FONT_NORMAL, 0, 1); 
    }
}
// updates which gen/value is selected
void oled_gen (){
    switch (gen_param) {
        case frequency_A:
        oledWriteString(&ssoled, 0, 1, 7,(char *)"FRQ:A", FONT_SMALL, 0, 1);
        break;
        case frequency_B:
        oledWriteString(&ssoled, 0, 1, 7,(char *)"FRQ:B", FONT_SMALL, 0, 1);
        break;
        case frequency_C:
        oledWriteString(&ssoled, 0, 1, 7,(char *)"FRQ:C", FONT_SMALL, 0, 1);
        break;
        case volume_B:
        oledWriteString(&ssoled, 0, 1, 7,(char *)"VOL:B ", FONT_SMALL, 0, 1);
        break;
        case volume_C:
        oledWriteString(&ssoled, 0, 1, 7,(char *)"VOL:C ", FONT_SMALL, 0, 1);
        break;
    }
}
 // updates oled volume display for gen
void oled_vol (){
    int vdisplayB = map(vol_C, 0, 100, 1, 6 );
    int vdisplayC = map(vol_B, 0, 100, 1, 6 );
    char* volumeBar[] = {
        ".      ",
        "..     ",
        "...    ",
        "....   ",
        ".....  ",
        "...... ",
    };
    oledWriteString(&ssoled, 0, 84, 6,volumeBar[vdisplayB], FONT_SMALL, 0, 1); 
    oledWriteString(&ssoled, 0, 84, 6,volumeBar[vdisplayC], FONT_SMALL, 0, 1); 
}

void oled_freq (){ 
     oled_freq_A ();
     oled_freq_B ();
     oled_freq_C ();
}

void oled_freq_A (){
    char buffer_A [6] = {};
    String str1 = String(fre_A, 2);
    str1.toCharArray(buffer_A,6);
    oledWriteString(&ssoled, 0, 0, 5, buffer_A, FONT_SMALL, 0, 1);
}

void oled_freq_B (){
    char buffer_B [6] = {};
    String str2 = String(fre_B+trim_B, 2);
    str2.toCharArray(buffer_B,6);
    oledWriteString(&ssoled, 0, 43, 5, buffer_B, FONT_SMALL, 0, 1); 
}

void oled_freq_C (){
    char buffer_C [6] = {};
    String str3 = String(fre_C+trim_C, 2);
    str3.toCharArray(buffer_C,6);
    oledWriteString(&ssoled, 0, 86, 5, buffer_C, FONT_SMALL, 0, 1); 
}

/*  
    Enables AD9833s one by one and displays amusing captions
*/
void splash_screen () {

    oledFill(&ssoled, 0, 1);
    gen.EnableOutput(true); 
    oledWriteString(&ssoled, 0, 0, 0, "WaveGen A Check", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 1, "OPENING:", FONT_SMALL, 0, 1);

    char* messageList[] = {
        "VOID.",
        "     ",
        "VOID..",
        "      ",
        "VOID...",
        "       ",
        "VOID....",
        "        ",
        "VOID.....",
        "         ",
        "VOID......",
        "          ",
    };

    const int messageCount = sizeof(messageList) / sizeof(messageList[0]);

    for (int i; i < messageCount; i++) {
        oledWriteString(&ssoled, 0, 0, 2, messageList[i], FONT_SMALL, 0, 1);
        delay(100);
    }
    
    gen3.EnableOutput(true);
    oledWriteString(&ssoled, 0, 0, 0, "WaveGen B Check", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 1, "INITIALISING:", FONT_SMALL, 0, 1);
    
    char* messageList2[] = {
        "SCREAM.",
        "       ",
        "SCREAM..",
        "        ",
        "SCREAM...",
        "         ",
        "SCREAM....",
        "          ",
        "SCREAM.....",
        "           ",
        "SCREAM......",
        "            ",
    };

    const int messageCount2 = sizeof(messageList2) / sizeof(messageList2[0]);

    for (int i; i < messageCount2; i++) {
        oledWriteString(&ssoled, 0, 0, 2, messageList2[i], FONT_SMALL, 0, 1);
        delay(100);
    }

    gen2.EnableOutput(true);
    oledWriteString(&ssoled, 0, 0, 0, "WaveGen C Check", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 1, "SCREAM INITIALISED", FONT_SMALL, 0, 1);
   
    char* messageList3[] = {
        "                  ",
        "SCREAM INITIALISED",
        "                  ",
        "SCREAM INITIALISED",
        "                  ",
        "SCREAM INITIALISED",
        "                  ",
        "SCREAM INITIALISED",
        "                  ",
        "SCREAM INITIALISED",
        "                  ",
        "SCREAM INITIALISED",
    };

    const int messageCount3 = sizeof(messageList3) / sizeof(messageList3[0]);

    for (int i; i < messageCount3; i++) {
        oledWriteString(&ssoled, 0, 0, 1, messageList3[i], FONT_SMALL, 0, 1);
        delay(100);
    }

    oledFill(&ssoled, 0, 1);
    oledWriteString(&ssoled, 0, 0, 0, "ronedronedronedronedroned", FONT_SMALL, 1, 1); // stays static
    reset_param ();
    fundamental_f = 0; 
    play_chord (fun_array [fundamental_f] , chord);
    update_oled (); // main screen
}
