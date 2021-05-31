## Ultrasonic marten repeller based on the blue pill board (STM32F103C8T6).

#### Idea

Device emits sounds of natural marten enemy, at example badger. It would be good to experiment with the sounds of various predators to find sounds that are the best to deter martens. The sampling rate was extended to 88,2kHz and the sound was moved in frequency scale to higher rates, to be less annoying for humans. Wav sample resolution is 8 bits in unsigned format. The sound starts in audible range and reaches ultrasonic.

#### Software 

Main worker is a Player structure with the set of related functions. It plays a wave table via complementary PWM outputs. The Timer 1 is used to generate complementary PWM signal and the Timer 2 is used for synchronization of the original rate of samples. PWM frequency is 281,25kHz.  
Example audio wave are defined in badger.h file.   
Rest of job is done in main loop.  

Helper macros and declarations for pin manipulating (gpio.h and bb.h) comes from ebook __Poradnik STM32 v1_9.pdf, autor: Szczywronek__.

#### Hardware 

The heart of device is the blue pill board (STM32F103C8T6). Folowing things are connected to the board 
- H-bridge with piezoelectric speaker. Pins PB13, PA8
- motion detector, PB0

The prototype features a simple bridge based on complementary bipolar transistors, controlled by open drain outputs. The bridge is powered from 5V. Higher voltage should gain higher volume and extends range of sound. No filtering has been implemented on the prototype. 
Ultimately, it would be better to use MOS transistors with low Ron to better use the source of power.


<img src="device.jpg" width="300">
