/*
 
MIT License

Copyright (c) 2020 Crash Works 3D

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

DESCRIPTION
  ====================
  The purpose of this code is to automate the servos and LED eyes for the Iron Man helmet

  Motivation and inspiration comes from the early work by "XL97" of The RPF Community

DEVELOPED BY
  ====================
  Dropwire
  Cranshark
  Taff

 */
// Version.  Don't change unless authorized by Cranshark
#define VERSION "3.0.0.1"

// Uncomment this line to enable Walsh3D MK85 Jaw Control (Open/Close)
//#define WALSH85

// Uncomment this line to enable sound for the S.U.E. expansion board
#define SOUND     

// Referenced libraries
// For installation instructions see https://github.com/netlabtoolkit/VarSpeedServo
#include <VarSpeedServo.h>

// For installation instructions see: https://github.com/fasteddy516/ButtonEvents
#include <Bounce2.h>
#include <ButtonEvents.h>

#ifdef SOUND
// See: https://github.com/enjoyneering/DFPlayer
// Important!!! On the SD card copy the mp3 files into an mp3 directory
// Download and install the DFPlayer library

#include <DFPlayer.h>
#include <SoftwareSerial.h>

#endif

// Declare pin settings
const int servo1Pin = 9; // set the pin for servo 1
const int servo2Pin = 10; // set the pin for servo 2

#ifdef WALSH85
const int servo3Pin = 5; // set the pin for servo 3 (Walsh85 Jaw Control)
#endif

const int buttonPin = 2; // the pin that the pushbutton is attached to

// led control pins (need to be PWM enabled pins for fading)
const int leftEyePin =  6;  // left eye LEDs
const int rightEyePin =  3;  // right eye LEDs
const int AuxLED = 4; // Aux LED non-PWM

#ifdef SOUND
// sound board pins
const int rx_pin = 7; // set pin for receive (RX) communications
const int tx_pin = 8; // set pin for transmit (TX) communications
#endif

// Declare servo objects
VarSpeedServo servo1; // create servo object to control servo 1
VarSpeedServo servo2; // create servo object to control servo 2

#ifdef WALSH85
VarSpeedServo servo3; // create servo object to control servo 3 (Walsh85 Jaw Control)
#endif

// Declare variables for servo speed control
const int servoCloseSpeed = 100; // set the speed of the servo close function
const int servoOpenSpeed = 255; // set the speed of the servo opening recommend set to max speed to aid in lift

//Servo 3 (Walsh85 Jaw Control) variables for servo speed control
#ifdef WALSH85
const int jawCloseSpeed = 175; // set the speed of the Jaw closing for Walsh85 Helmet
const int jawOpenSpeed = 255; // set the speed of the Jaw opening for Walsh85 Helmet
#endif

// In Dual Servo Configuration the servos move in opposing directions, so the angles of the servos will be opposite to each other. 
// Normal Servo range is 0° ~ 180°, for initial setup the range has been adjusted to 20° ~ 160°, this allows for a 20° adjustment at both ends of the servo range.
// See Helmet tutorial for further information on servo setup.
const int servo1_OpenPos = 20; // set the open position of servo 1
const int servo2_OpenPos = 160; // set the open position of servo 2
const int servo1_ClosePos = 160; // set the closed position of servo 1
const int servo2_ClosePos = 20; // set the closed position of servo 2

#ifdef WALSH85
//Servo 3 (Walsh85 Jaw Control) Open / Close Angle
const int servo3_OpenPos = 90; // set the open position of servo 2
const int servo3_ClosePos = 0; // set the closed position of servo 1
#endif

// Declare variables for setup special effects (applies to LED eyes only for now)
#define SETUP_NONE 0 // No special effects, just turn on the LED eyes
#define SETUP_MOVIE_BLINK 1 // Blink LED eyes on setup, sequence based on Avengers Movie
#define SETUP_FADE_ON 2 // Slowly brighten LED eyes until fully lit

// To use the specific feature below
// use double slashes "//" to comment, or uncomment (remove double slashes) in the code below

// Uncomment this line if you don't want any special effect during setup, comment this line to disable this effect
// const int setupFx = SETUP_NONE;

// Uncomment this line if you want the movie blink special effect during setup, comment this line to disable this effect
const int setupFx = SETUP_MOVIE_BLINK;

// Uncomment this line if you want the fade on special effect during setup, comment this line to disable this effect
// const int setupFx = SETUP_FADE_ON;

// Declare variables for LED eyes special effects (applies to LED eyes only for now)
#define EYES_NONE 0 // No special effects, just turn on the LED eyes
#define EYES_MOVIE_BLINK 1 // Blink LED eyes on setup, sequence based on Avengers Movie
#define EYES_FADE_ON 2 // Slowly brighten LED eyes until fully lit

// To use the specific feature below
// use double slashes "//" to comment, or uncomment (remove double slashes) in the code below

// Uncomment this line if you don't want any special effect during setup, comment this line to disable this effect
// const int eyesFx = EYES_NONE;

// Uncomment this line if you want the movie blink special effect during setup, comment this line to disable this effect
// const int eyesFx = EYES_MOVIE_BLINK;

// Uncomment this line if you want the fade on special effect during setup, comment this line to disable this effect
const int eyesFx = EYES_FADE_ON;

// Declare variables for button control
boolean movieblinkOnClose = false; //Blink LEDs on close of faceplate, Sequence based on Avengers Movie

// Declare variable for AuxLED
boolean auxLedEnabled = true; // Set to true if you want to enable the Aux LED
boolean auxLedState = false; // Keeps track of the state of the LED on = true, off = false

// Declare variables for LED control
unsigned long fadeDelay = .1; //speed of the eye 'fade'
unsigned long callDelay = 10; //length to wait to start eye flicker after face plate comes down
unsigned long blinkSpeed = 60; //delay between init blink on/off
unsigned long currentPWM = 0; // keep track of where the current PWM level is at
boolean isOpen = true; // keep track of whether or not the faceplate is open

#ifdef SOUND
#define MP3_TYPE DFPLAYER_MINI // DEFAULT Chip type of DFPlayerMini (see documentation)
// #define MP3_TYPE DFPLAYER_FN_X10P // Chip type of DFPlayerMini (see documentation)
// #define MP3_TYPE DFPLAYER_HW_247A // Chip type of DFPlayerMini (see documentation)
// #define MP3_TYPE DFPLAYER_NO_CHECKSUM // Chip type of DFPlayerMini (see documentation)
#define MP3_SERIAL_TIMEOUT 100 //average DFPlayer response timeout 100msec..200msec
// Declare variables for sound control
const int volume = 29; // sound board volume level (30 is max)
#define SND_CLOSE 1 // sound track for helmet closing sound
#define SND_JARVIS 2 // sound track for JARVIS sound
#define SND_OPEN 3 // sound track for helmet opening sound

SoftwareSerial serialObj(rx_pin, tx_pin); // Create object for serial communications
DFPlayer mp3Obj;
#endif

// Define object for primary button to handle 
// multiple button press features:
// 1. Single Tap
// 2. Double Tap
// 3. Long Press
ButtonEvents primaryButton = ButtonEvents(); 

// State of the faceplate 1 = open, 0 = closed
#define FACEPLATE_CLOSED 0
#define FACEPLATE_OPEN 1
int facePlateCurMode = FACEPLATE_OPEN; // Keep track if the faceplate is open or closed

// State of the LED eyes 1 = on, 2 = off
#define LED_EYES_OFF 0
#define LED_EYES_ON 1

// State of the LED eyes for dimming/brightening 1 = brighten, 2 = dim
#define LED_EYES_DIM_MODE 0
#define LED_EYES_BRIGHTEN_MODE 1

int ledEyesCurMode = LED_EYES_DIM_MODE; // Keep track if we're dimming or brightening
int ledEyesCurPwm = 0; // Tracking the level of the LED eyes for dim/brighten feature
const int ledEyesIncrement = 15; // Define the increments to brighten or dim the LED eyes

/**
 * Helper Method
 * Simulate a delay in processing without disabling the processor completely
 * 
 * @param[out] period - the amount of time in milliseconds to delay
 * 
 * See: https://randomnerdtutorials.com/why-you-shouldnt-always-use-the-arduino-delay-function/
*/
void simDelay(long period){
  long delayMillis = millis() + period;
  while (millis() <= delayMillis)
  {
    int x = 0; // dummy variable, does nothing
  }
}

/**
 * Simulate the eyes slowly blinking until fully lit
 */ 
void movieblink(){
  Serial.println(F("Start Movie Blink.."));

  // pause for effect...
  simDelay(300);

  int lowValue = 21;
  int delayInterval[] = { 210, 126, 84 };
  int delayVal = 0;

  // First blink on
  for (int i = 0; i <= lowValue; i++){
    setLedEyes(i);
    setAuxLed();
    delayVal = delayInterval[0]/lowValue;
    simDelay(delayVal);
  }

  // Turn off
  setLedEyes(0);
  setAuxLed();
  simDelay(delayInterval[0]);

  // Second blink on
  for (int i = 0; i <= lowValue; i++){
    setLedEyes(i);
    setAuxLed();
    delayVal = delayInterval[1]/lowValue;
    simDelay(delayVal);
  }

  // Turn off
  setLedEyes(0);
  setAuxLed();
  simDelay(delayInterval[1]);

  // Third blink on
  setLedEyes(lowValue);
  setAuxLed();
  simDelay(delayInterval[2]);

  // Turn off
  setLedEyes(0);
  setAuxLed();
  simDelay(delayInterval[2]);

  // All on
  setLedEyes(255);
  auxLedOn();   
}

/*
 * Simulate LED eyes slowly brightening until fully lit
 */
 void fadeEyesOn(){
  ledEyesCurMode = LED_EYES_BRIGHTEN_MODE;

  // loop until fully lit
  while (ledEyesCurPwm < 255){
    setLedEyes(ledEyesCurPwm);
  
    simDelay(200);
    ledEyesBrighten();
  }
  
 }

#ifdef SOUND
/**
 * Initialization method for DFPlayer Mini board
 */
void init_player(){
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  serialObj.begin(9600);

  mp3Obj.begin(serialObj, MP3_SERIAL_TIMEOUT, MP3_TYPE, false);

  mp3Obj.stop();        //if player was runing during ESP8266 reboot
  mp3Obj.reset();       //reset all setting to default
  
  mp3Obj.setSource(2);  //1=USB-Disk, 2=TF-Card, 3=Aux, 4=Sleep, 5=NOR Flash
  
  mp3Obj.setEQ(0);      //0=Off, 1=Pop, 2=Rock, 3=Jazz, 4=Classic, 5=Bass
  mp3Obj.setVolume(volume); //0..30, module persists volume on power failure

  mp3Obj.sleep();       //inter sleep mode, 24mA
}

/**
 * Method to play the sound effect for a specified feature
 */
void playSoundEffect(int soundEffect){
  Serial.print(F("Playing sound effect: "));
  Serial.println(soundEffect);

  mp3Obj.wakeup(2);

  mp3Obj.playTrack(soundEffect);
}
#endif

/**
 * Method to open face plate
 */
 void facePlateOpen(){
  Serial.println(F("Servo Up!")); 

  // Re-attach the servos to their pins
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  #ifdef WALSH85
  servo3.attach(servo3Pin);
  #endif

  // Send data to the servos for movement
    
  servo1.write(servo1_OpenPos, servoOpenSpeed);
  servo2.write(servo2_OpenPos, servoOpenSpeed);
  
  #ifdef WALSH85
  simDelay(500);
  servo3.write(servo3_OpenPos, jawOpenSpeed);
  //simDelay(1000); // wait doesn't wait long enough for servos to fully complete...
  #endif
  
  simDelay(1000); // wait doesn't wait long enough for servos to fully complete...

  // Detach so motors don't "idle"
  servo1.detach();
  servo2.detach();

  #ifdef WALSH85
  servo3.detach();
  #endif

  facePlateCurMode = FACEPLATE_OPEN;
 }

 /**
  * Method to close face plate
  */
 void facePlateClose(){
  Serial.println(F("Servo Down"));  

  // Re-attach the servos to their pins
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  #ifdef WALSH85
  servo3.attach(servo3Pin);
  #endif

  // Send data to the servos for movement 

  #ifdef WALSH85
  servo3.write(servo3_ClosePos, jawCloseSpeed);
  simDelay(500); // Delay to allow Jaw to fully close before Faceplate closes
  #endif
  
  servo1.write(servo1_ClosePos, servoCloseSpeed);
  servo2.write(servo2_ClosePos, servoCloseSpeed);

  simDelay(1000); // wait doesn't wait long enough for servos to fully complete...

  // Detach so motors don't "idle"
  servo1.detach();
  servo2.detach();

  #ifdef WALSH85
  servo3.detach();
  #endif

  facePlateCurMode = FACEPLATE_CLOSED;
 }

/**
 * Set the brightness of the LED eyes
 * 
 * @param[out] pwmValue - the PWM value (0-255) for the LED brightness
 */
void setLedEyes(int pwmValue){
  analogWrite(rightEyePin, pwmValue);
  analogWrite(leftEyePin, pwmValue);
  ledEyesCurPwm = pwmValue;
}
 
/**
 * Method to turn on LED eyes
 */
void ledEyesOn(){
  Serial.println(F("Turning LED eyes on..."));
  
  setLedEyes(255);
  
  ledEyesCurMode = LED_EYES_DIM_MODE;
}

/**
 * Method to turn off LED eyes
 */
void ledEyesOff(){
  Serial.println(F("Turning LED eyes off..."));
  
  setLedEyes(0);

  ledEyesCurMode = LED_EYES_BRIGHTEN_MODE;
}

/**
 * Method to turn LED eyes on/off
 */
void ledEyesOnOff(){
  // LED eyes stay off when faceplate is open
  if(facePlateCurMode == FACEPLATE_CLOSED){
    if (ledEyesCurPwm > 0){
      ledEyesOff();
    } else {
      ledEyesOn();
    }
  }
}

void ledEyesDim(){
  Serial.println(F("Dimming LED eyes..."));

  ledEyesCurPwm = ledEyesCurPwm - ledEyesIncrement; // Decrease the brightness

  // Make sure we don't go over the limit
  if(ledEyesCurPwm <= 0){
    ledEyesCurPwm = 0;
  }
}

void ledEyesBrighten(){
  Serial.println(F("Brightening LED eyes..."));

  ledEyesCurPwm = ledEyesCurPwm + ledEyesIncrement; // Increase the brightness

  // Make sure we don't go over the limit
  if(ledEyesCurPwm >= 255){
    ledEyesCurPwm = 255;
  }
}

/**
 * Method to dim or brighten both LED eyes
 */
void ledEyesFade(){
  if(ledEyesCurPwm == 255){
    ledEyesCurMode = LED_EYES_DIM_MODE;
  } else if(ledEyesCurPwm == 0){
    ledEyesCurMode = LED_EYES_BRIGHTEN_MODE;
  }
  
  if(ledEyesCurMode == LED_EYES_BRIGHTEN_MODE){
    ledEyesBrighten();
  } else {
    ledEyesDim();
  }

  setLedEyes(ledEyesCurPwm);

  simDelay(200);
}

/*
 * Sets the Aux LED
 */
void setAuxLed(){
  if (auxLedEnabled) {
    if (auxLedState == false){
      auxLedOn();
    } else {
      auxLedOff();
    }
  } else {
    auxLedOff();
  }
}

/*
 * Turn the Aux LED on
 */
void auxLedOn(){
  digitalWrite(AuxLED, HIGH);
  auxLedState = true;
}

/*
 * Turn the Aux LED off
 */
void auxLedOff(){
  digitalWrite(AuxLED, LOW);
  auxLedState = false;
}

/**
 * Method to run sequence of sppecial effects when system first starts or sets up
 */
void startupFx(){
  //facePlateClose();

#ifdef SOUND
  playSoundEffect(SND_CLOSE);
  simDelay(500); // Timing for Helmet Close Sound and delay to servo closing
#endif

  facePlateClose();

  switch(setupFx){
    case SETUP_NONE:
      ledEyesOn();
      auxLedOn();
      break;
    case SETUP_MOVIE_BLINK:
      movieblink();
      break;
    case SETUP_FADE_ON:
      fadeEyesOn();
      auxLedOn();
      break;
  }

#ifdef SOUND
  simDelay(800); // Originally 2000ms
  playSoundEffect(SND_JARVIS);
#endif
}

/**
 * Method to execute special effects when the faceplate opens
 */
void facePlateOpenFx(){
  // TODO: See if we need delays in between fx
#ifdef SOUND
  playSoundEffect(SND_OPEN);
#endif

  ledEyesOff();

  facePlateOpen();
}

/**
 * Method to execute special effects when the faceplate closes
 */
void facePlateCloseFx(){
#ifdef SOUND
  playSoundEffect(SND_CLOSE);
  simDelay(1200); //Timing for Helmet Close Sound and delay to servo closing
#endif

  facePlateClose();

  switch(eyesFx){
    case EYES_NONE:
      ledEyesOn();
      auxLedOn();
      break;
    case EYES_MOVIE_BLINK:
      movieblink();
      break;
    case EYES_FADE_ON:
      fadeEyesOn();
      auxLedOn();
      break;
  }
}

/**
 * Handle faceplate special effects
 */
void facePlateFx(){
  if (facePlateCurMode == FACEPLATE_OPEN){
    facePlateCloseFx();
  } else {
    facePlateOpenFx();
  }
}

/**
 * Event handler for when the primary button is tapped once
 */
void handlePrimaryButtonSingleTap(){
  facePlateFx();
}

/**
 * Event handler for when the primary button is double tapped
 */
void handlePrimaryButtonDoubleTap(){
  ledEyesOnOff();
}

/**
 * Event handler for when the primary button is pressed and held
 */
void handlePrimaryButtonLongPress(){
  while(!primaryButton.update()){
    ledEyesFade(); // Dim or brighten the LED eyes
  }
}

/**
 * Initializes the primary button for multi-functions
 */
void initPrimaryButton(){
  // Attach the button to the pin on the board
  primaryButton.attach(buttonPin, INPUT_PULLUP);
  // Initialize button features...
  primaryButton.activeLow();
  primaryButton.debounceTime(15);
  primaryButton.doubleTapTime(250);
  primaryButton.holdTime(2000);
}

/**
 * Monitor for when the primary button is pushed
 */
void monitorPrimaryButton(){
  bool changed = primaryButton.update();

  // Was the button pushed?
  if (changed){
    int event = primaryButton.event(); // Get how the button was pushed

    switch(event){
      case(tap):
        Serial.println(F("Primary button single press..."));
        handlePrimaryButtonSingleTap();
        break;
      case (doubleTap):
        Serial.println(F("Primary button double press..."));
        handlePrimaryButtonDoubleTap();
        break;
      case (hold):
        Serial.println(F("Primary button long press..."));
        handlePrimaryButtonLongPress();
        break;
    }
  }
}

/**
 * Initialization method called by the Arduino library when the board boots up
 */
void setup() {
  // Set up serial port
  Serial.begin(115200);  
  
  simDelay(2000); // Give the serial service time to initialize

  Serial.print(F("Initializing Iron Man Servo version: "));
  Serial.println(VERSION);

#ifdef SOUND
  init_player(); // initializes the sound player
#endif

  initPrimaryButton(); // initialize the primary button
  
  pinMode(AuxLED, OUTPUT); // set output for AUX LED

  startupFx(); // Run the initial features
}

/**
 * Main program exeucution
 * This method will run perpetually on the board
 */
void loop() {
  monitorPrimaryButton(); // Since all features currently are tied to the one button...

  // Room for future features ;)
}