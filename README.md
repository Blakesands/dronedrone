# dronedrone
An Arduino based musical drone Shruti box using AD9833 Waveform Generators

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
