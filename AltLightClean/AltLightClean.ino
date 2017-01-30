//###############################################################################
// Declarations Section
//###############################################################################

const byte interruptPin = 19; // Use A5 for Input Button wire
volatile bool ButtonPress = 0;  // Setup bit for tracking the press
int minutes = 0;
bool finished = 1;
unsigned long lastSecond = 0;
int seconds = 0;
int fives = 0;
bool complete = 0;
bool started = 0;
bool justSpoke = 0;
char *Filename = new char[13];
char *NumSecs = new char[3];
char *MPString = ".mp3";
char *MinChar = new char[7];
char *SecChar = new char[7];

// Specifically for use with the Adafruit Feather, the pins are pre-set here!

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <Adafruit_VS1053.h>

// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather M0 or 32u4  <----- This is used for Altz Light!
#if defined(__AVR__) || defined(ARDUINO_SAMD_FEATHER_M0)
  #define VS1053_CS       6     // VS1053 chip select pin (output)
  #define VS1053_DCS      0     // VS1053 Data/command select pin (output)
  #define CARDCS          5     // Card chip select pin
  // DREQ should be an Int pin *if possible* (not possible on 32u4)
  #define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin
#endif


Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

//###############################################################################
// Setup - main  -> This runs once
//###############################################################################
void setup()
{
  //  ****  FOR DEBUG ONLY ---- Setup Serial Terminal  -------  *******
  // Serial.begin(115200);
  // Wait for serial port to be opened, remove this line for 'standalone' operation
  // while (!Serial) { delay(1); }

  // Serial.println("\n\nAdafruit VS1053 Feather Test");
  //  ****  END DEBUG ONLY ---- Setup Serial Terminal  -------  *******
  
  
  // First, configure PWMs on pins 10 to 13 for 40Hz at 40% duty cycle
  
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
                    GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |         // Enable GCLK4
                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                     GCLK_GENCTRL_ID(4);          // Select GCLK4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization now 6
  
  // Enable the port multiplexer for the digital pin D11(PA16), D13(PA17). D10(PA18), D12(PA19)
  PORT->Group[g_APinDescription[11].ulPort].PINCFG[g_APinDescription[11].ulPin].bit.PMUXEN = 1;
  PORT->Group[g_APinDescription[13].ulPort].PINCFG[g_APinDescription[13].ulPin].bit.PMUXEN = 1;
  PORT->Group[g_APinDescription[10].ulPort].PINCFG[g_APinDescription[10].ulPin].bit.PMUXEN = 1;
  PORT->Group[g_APinDescription[12].ulPort].PINCFG[g_APinDescription[12].ulPin].bit.PMUXEN = 1;

  // Connect the TCC0 timer to digital output D13 = PA17 = ODD - port pins are paired odd PMUO and even PMUXE
  // F & E specify the timers: TCC0, TCC1 and TCC2
  PORT->Group[g_APinDescription[11].ulPort].PMUX[g_APinDescription[11].ulPin >> 1].reg = PORT_PMUX_PMUXO_E | PORT_PMUX_PMUXE_E;
  PORT->Group[g_APinDescription[10].ulPort].PMUX[g_APinDescription[10].ulPin >> 1].reg = PORT_PMUX_PMUXO_F | PORT_PMUX_PMUXE_F;

  // Feed GCLK4 to TCC2 (and TC3)
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TCC2 (and TC3)
                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                     GCLK_CLKCTRL_ID_TCC2_TC3;    // Feed GCLK4 to TCC2 (and TC3)
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Feed GCLK4 to TCC0 and TCC1
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TCC0 and TCC1
                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                     GCLK_CLKCTRL_ID_TCC0_TCC1;   // Feed GCLK4 to TCC0 and TCC1
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Dual slope PWM operation: timers countinuously count up to PER register value then down 0
  REG_TCC2_WAVE |= TCC_WAVE_POL(0xF) |            // Reverse the output polarity on all TCC2 outputs
                   TCC_WAVE_WAVEGEN_DSBOTH;       // Setup dual slope PWM on TCC2
  while (TCC2->SYNCBUSY.bit.WAVE);               // Wait for synchronization

  // Dual slope PWM operation: timers countinuously count up to PER register value then down 0
  REG_TCC0_WAVE |= TCC_WAVE_POL(0xF) |            // Reverse the output polarity on all TCC0 outputs
                   TCC_WAVE_WAVEGEN_DSBOTH;       // Setup dual slope PWM on TCC0
  while (TCC0->SYNCBUSY.bit.WAVE);               // Wait for synchronization

  // Each timer counts up to a maximum or TOP value set by the PER register,
  // this determines the frequency of the PWM operation:
  REG_TCC2_PER = 50100; // Set the frequency of the PWM on TCC2 to 40Hz
  while (TCC2->SYNCBUSY.bit.PER);                // Wait for synchronization

  // Each timer counts up to a maximum or TOP value set by the PER register,
  // this determines the frequency of the PWM operation:
  REG_TCC0_PER = 50100; // Set the frequency of the PWM on TCC0 to 40Hz
  while (TCC0->SYNCBUSY.bit.PER);                // Wait for synchronization

  // Set the PWM signal to output 20ms and 20ms
  REG_TCC2_CC0 = 20000;                              // TCC2 CC0 - on D11
  REG_TCC2_CC1 = 20000;                      // TCC2 CC1 - on D13
  while (TCC2->SYNCBUSY.bit.CC1);                   // Wait for synchronization

  // Set the PWM signal to output 20ms and 20ms
  REG_TCC0_CC2 = 20000;                            // TCC0 CC0 - on D10
  REG_TCC0_CC3 = 20000;                           // TCC0 CC1 - on D12
  while (TCC0->SYNCBUSY.bit.CC3);                 // Wait for synchronization

  // Divide the 12MHz signal by 4 giving 3MHz (333.33ns) TCC2 timer tick and enable the outputs
  REG_TCC2_CTRLA |= TCC_CTRLA_PRESCALER_DIV4 |    // Divide GCLK4 by 4
                    TCC_CTRLA_ENABLE;             // Enable the TCC2 output
  while (TCC2->SYNCBUSY.bit.ENABLE);              // Wait for synchronization

  // Divide the 12MHz signal by 4 giving 3MHz (333.33ns) TCC0 timer tick and enable the outputs
  REG_TCC0_CTRLA |= TCC_CTRLA_PRESCALER_DIV4 |    // Divide GCLK4 by 4
                    TCC_CTRLA_ENABLE;             // Enable the TCC0 output
  while (TCC0->SYNCBUSY.bit.ENABLE);              // Wait for synchronization

// Setup IO pins for GPIO tasks

  pinMode(interruptPin, INPUT_PULLUP);  // Setup button input pin
  attachInterrupt(digitalPinToInterrupt(interruptPin), buttonIn, FALLING);  // Trigger on falling edge
  pinMode(8, OUTPUT);
  pinMode(18, OUTPUT);  // Line to control power switch for LED power supply
  minutes = 0;
  seconds = 0;
  finished = 1;

  digitalWrite(8, LOW);
  digitalWrite(18, LOW);  // Make sure power is off to LEDs
  
// Setup Music Player
  musicPlayer.begin();
  SD.begin(CARDCS);
  musicPlayer.setVolume(10,10);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);


  musicPlayer.playFullFile("HiDad.mp3");  // Say Hi Daddy!

// Wait here for first button press
  while(!ButtonPress) {
    delay(1);
  }
// Setup millisecond counter
lastSecond = millis();
// Turn on power to LED Strip Power Supply

  digitalWrite(18, HIGH);  // Turn on LEDs
}
 
//###############################################################################
// Loop - main
//###############################################################################
 
// This loop detects the button press, adds 15 minutes to the run time, runs 
// and then shuts down.  It speaks the run time as the button is pressed.
// During running, it will announce a message every five minutes
// When shutting down, it will leave the lights on and warn that lights will go out
// in one minute, then run a timer and fade the lights at the end while saying
// "goodbye"
// Time is kept in only minutes and seconds.

void loop() {


 
  // Get time
    while(millis() - lastSecond > 1000) {
        lastSecond += 1000;
        if(seconds > 0 && minutes >= 0) {  //If a second has passed...
          seconds--;                       //...count down one second
        }
        else if(minutes > 0) {
          seconds = 59;           // reset the seconds to another minute
          minutes--;              // decrement the minute
          justSpoke = 0;          // clear "Just Spoke" bit to signal we are ok to signal when the next mult of 5 occurs
       }
    }
    if(minutes > 180) {
      minutes = 180;              // cap runtime to three hours
    }
    while(seconds > 60 || seconds < 0) {
      if (seconds > 60) {
      seconds -= 60;     // Take away a minute if more than a minute of seconds
      }
      else {
        seconds += 60;   //Add a minute if we went under 0 seconds
      }
      if(minutes > 0) {
        minutes--;       //Count down one minute of minutes
      }
    }
   String MinString = String(itoa(minutes, MinChar, 10));
   String SecString = String(itoa(seconds, SecChar, 10));
   //   Serial.print(MinString);
   //   Serial.print(":");
   //   Serial.println(SecString);
   //   delay(100);
  // When button is pressed, add 15 minutes to the timer and ignore drift.
  if (ButtonPress) {
  minutes += 15;  // Add 15 minutes to the run time
  if (started == 0) {
  started = 1;
  musicPlayer.playFullFile("Welcome.mp3");  // Say Hello!
  // Setup millisecond counter
     lastSecond = millis();
  }
  finished = 0; // Start task, if not started
  digitalWrite(8, HIGH); //Turn on Green LED for testing purposes
  delay(750);  // Debounce switch for 1/4 second
  digitalWrite(8, LOW); //Turn off Green LED for testing purposes
  ButtonPress = 0; // Clear ButtonPress
  

  // Speak minutes remaining
      String MinString = String(itoa(minutes, MinChar, 10));
      strcpy (Filename,itoa(minutes, MinChar, 10));
      strcat (Filename,MPString);
  if (finished == 0) {
    if (musicPlayer.stopped()) {
      musicPlayer.playFullFile(Filename);
    }
    if (musicPlayer.stopped()) {
      musicPlayer.playFullFile("MinsRem.mp3");
    }
  }
  }
 
  // Check time remaining
  if (minutes > 0 && justSpoke == 0) {
   // If we are not in the final minute check if we are a multiple of five minutes.
    if(minutes % 5 == 0) {
          while (!musicPlayer.stopped()) {      // Check voice has stopped
            delay(10);
          }
      String MinString = String(itoa(minutes, MinChar, 10));
      strcpy (Filename,itoa(minutes, MinChar, 10));
      strcat (Filename,MPString);
      musicPlayer.playFullFile(Filename);      // speak time remaining
          while (!musicPlayer.stopped()) { 
            delay(10);
          }
      musicPlayer.playFullFile("MinsRem.mp3");
      justSpoke = 1;
    }
  }
  else{
   // If this is the final minute, countdown seconds remaining and then dim lights to off.
   if (seconds % 10 == 0 && justSpoke == 0) {
         while (!musicPlayer.stopped()) {      // Check voice has stopped
          delay(10);
         }
      String MinString = String(itoa(minutes, MinChar, 10));
      strcpy (Filename,itoa(seconds, MinChar, 10));
      strcat (Filename,MPString);
      musicPlayer.playFullFile(Filename);  // Speak remaining seconds
         while (!musicPlayer.stopped()) {      // Check voice has stopped
          delay(10);
         }
      musicPlayer.playFullFile("ScndsRem.mp3");
     if (seconds == 0) {
     finished = 1;
         while (!musicPlayer.stopped()) {      // Check voice has stopped
          delay(10);
         }
      musicPlayer.playFullFile("done.mp3");  // Say goodbye
     }
  }
  }
  if (finished == 1 && started == 1) {  //Dim the lights
    musicPlayer.playFullFile("LightOff.mp3");  // Warn that the lights are going out
    musicPlayer.playFullFile("LightsOn.mp3");  // Ask to turn on lights in room
    for(int dutycycle = 40000; dutycycle > 0; dutycycle--){
  // Set the PWM signal to output dutycycle/1000ms
  REG_TCC2_CC0 = dutycycle;                  // TCC2 CC0 - on D11
  REG_TCC2_CC1 = dutycycle;                  // TCC2 CC1 - on D13
  while (TCC2->SYNCBUSY.bit.CC1);            // Wait for synchronization

  // Set the PWM signal to output dutycycle/1000ms
  REG_TCC0_CC2 = dutycycle;                  // TCC0 CC0 - on D10
  REG_TCC0_CC3 = dutycycle;                  // TCC0 CC1 - on D12
  while (TCC0->SYNCBUSY.bit.CC3);            // Wait for synchronization
  delay(1);
    }
    pinMode(10,INPUT);  //Shut off the lights
    pinMode(11,INPUT);
    pinMode(12,INPUT);
    pinMode(13,INPUT);
    complete = 1;       //...and we're done!
    digitalWrite(18, LOW); // Make sure power is off to LEDs
  }
while(complete && started){  //halt
//  Serial.println("Halted");
}
}
void buttonIn() {
  ButtonPress = 1;
}

