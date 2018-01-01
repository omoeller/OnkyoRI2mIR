/*  read Onkyo (IR) - write Marantz CD (RI)
  
    Reads  Onkyo RI (Remote Interactive) Protocol on pin 4
    Writes Maratz CD infrared protocol via LED connected to pin 3

    Requires the IRremote library, see
    https://github.com/z3t0/Arduino-IRremote

    ===== pin connections ================================================

    pin3 ---- (x) infrared LED ---[R1 = 330 OHM] ---------- GND

    pin4 ---- o ----[R2 = 10 K OHM]------------------------ GND
              |
              |
              ...
              |
              |
              o Onkyo Amplifier RI port (middle cable)


    Optionally, connect pin2 to a pushbutton (triggers "STOP")

                 /         
                / 
    pin2 ---- v     ------[R3 = 330 OHM] ------------------ GND


    Recommended: connect Anduino GND to Amplifier GND 
    ======================================================================

    ## -------------------------------------------------------------------
    ## @TABLE OF CONTENTS:	       [TOCD: 12:29 31 Dec 2017]
    ##
    ##      [0.1] HEADER
    ##      [0.2] CONFIG (defines)
    ##  [1] GLOBALS
    ##      [1.1] ONNKYO CODES
    ##      [1.2] Translate to Marantz Codes
    ##  [2] Infrared Command Logic
    ##      [2.1] SETUP
    ##  [3] MAIN LOOP
    ## -------------------------------------------------------------------
*/


// ###############################################
// [0.1] HEADER
// ###############################################

#include "Arduino.h"
#include <IRremote.h>

// ###############################################
// [0.2] CONFIG (defines)
// ###############################################

#define START_MARKER_HIGH  25
#define N_BITS             12

#define N_REPEAT_SEND        10
#define DELAY_AFTER_SEND_US 100
#define FORGET_AFTER_MS     500

//#define RO_DEBUG

// ###################################################################
// [1] GLOBALS
// ###################################################################


/* digital pin 2 has a pushbutton attached to it */
const int pushButton = 2;
/* read out Onkyo RI protocol via pin 4 */
const int onkyoIn = 4;
/* IRremote always uses pin 3 for output */
const int irOut = 3;
IRsend irsend;

static int last_CMD = 0;

// ###############################################
// [1.1] ONNKYO CODES
// ###############################################

/* Onkyo codes - thanks to https://github.com/docbender/Onkyo-RI.git
   -> Codes_Onkyo_RI.txt (aka Remote_Interactive)

   header       3000  1000
   one          1000  2000
   zero         1000  1000
   ptrail       1000
*/
#define CD_Forward               0x0F00
#define CD_Rewind                0x0F01
#define CD_On                    0x0F04
#define CD_Clear                 0x0F08
#define CD_Eject                 0x0F0B
#define CD_8                     0x0F0C
#define CD_9                     0x0F0D
#define CD_0                     0x0F0E
#define CD_Digits                0x0F0F
#define CD_1                     0x0F10
#define CD_2                     0x0F11
#define CD_3                     0x0F12
#define CD_4                     0x0F13
#define CD_5                     0x0F18
#define CD_6                     0x0F19
#define CD_7                     0x0F1A
#define CD_Play                  0x0F1B
#define CD_Stop                  0x0F1C
#define CD_NextChapter           0x0F1D
#define CD_PrevChapter           0x0F1E
#define CD_Pause                 0x0F1F
#define CD_Random                0x0F46
#define CD_ChUp                  0x0F5C
#define CD_ChDn                  0x0F5F
#define CD_Standby               0x0F8F
#define Generic_Repeat           0x0F06 /* Measured, not in Codes_Onyko_RI.txt */

#define CDR_Repeat               0x060A /* will be used for EJECT TRAY */
/* ------------------------------------------------------------------------------- */

// ###############################################
// [1.2] Translate to Marantz Codes
// ###############################################
 
/** According to Manufacturer spec
 *    REMOTE COMMANDS TABLE FOR MODEL : CD6004
 *    @see CD6004_IR_CODE_V01_Updated.pdf
 *  Since commands are in RC5 format (5 command bits, 6 data bits),
 *  an entry "2052" means cmd=20, data=52 and thus (20 << 6) | 52
 */
unsigned int translateOnkyo2Marantz(long int code){
  switch(code){
  case CD_Forward              : return (20 << 6) | 52 ;
  case CD_Rewind               : return (20 << 6) | 50;
  case CD_Random               : return (20 << 6) | 28 ;
  case CD_1                    : return (20 << 6) |  1 ;
  case CD_2                    : return (20 << 6) |  2 ;
  case CD_3                    : return (20 << 6) |  3 ;
  case CD_4                    : return (20 << 6) |  4 ;
  case CD_5                    : return (20 << 6) |  5 ;
  case CD_6                    : return (20 << 6) |  6 ;
  case CD_7                    : return (20 << 6) |  7 ;
  case CD_8                    : return (20 << 6) |  8 ;
  case CD_9                    : return (20 << 6) |  9 ;
  case CD_0                    : return (20 << 6) |  0 ;
  case CD_Play                 : return (20 << 6) | 53 ;
  case CD_Stop                 : return (20 << 6) | 54 ;
  case CD_NextChapter          : return (20 << 6) | 32 ;
  case CD_PrevChapter          : return (20 << 6) | 33 ;
  case CD_Pause                : return (20 << 6) | 48 ;
  case CD_On                   : return (20 << 6) | 12 ; /* Power On/Off */

    /* not supported
       case CD_Standby              : return (20 << 6) | 1202LU ;
    */

  case Generic_Repeat:
  case CDR_Repeat              : return (20 << 6) | 45 ; /* tray open/close */

  }
  return 0;
}




// ###################################################################
// [2] Infrared Command Logic
// ###################################################################

/**
 * Translates the (so far) read data to Infrared Output
 * Uses the IRremote library to translate RC5 Commands
 * Depends on @see N_REPEAT_SEND and DELAY_AFTER_SEND_US
 * 
 * @param isHigh  button state (pin 2)
 * @param o_cmd   the read Onkyo RI command
 */
void printOutput(int isHigh, int o_cmd){
  static int lastMCom = 0;
  static bool toggle;
  static int repetitions = N_REPEAT_SEND;
#ifdef RO_DEBUG
  static int counter = 0;
#endif
  int MCom;

  if(isHigh){ // always STOP, this if for debugging
    lastMCom = 0;
    repetitions = 0;
    MCom =  translateOnkyo2Marantz(CD_Stop);
  }
  else {
    MCom =  translateOnkyo2Marantz(o_cmd);
  }
  
  if((lastMCom != MCom) || (repetitions < N_REPEAT_SEND)){
    repetitions++;
    if(lastMCom != MCom){
      Serial.print("NEW IR CMD:");
      Serial.println(MCom);
      repetitions = 0;
      toggle++;
    }
    lastMCom = MCom;
    if(MCom > 0){
      irsend.sendRC5(((1 & toggle) << 12) | (int)MCom, 12); // Marantz uses RC5, LSB, 11bit+1 toggle
      delayMicroseconds(DELAY_AFTER_SEND_US);
    }
  }
  
#ifdef RO_DEBUG
  // let the on-board LED blink in a pattern that corresponds to the last IR Command sent
  // since the main cycle is about 100us, we use 1000 cycles (0.1 s) per bit
  digitalWrite(LED_BUILTIN, ( (1 << (counter/1000)) & lastMCom ? HIGH: LOW));
  counter++;
  if(counter/1000 >= N_BITS  ){
    counter = 0;
  }
#endif
}

// ###############################################
// [2.1] SETUP
// ###############################################

void setup() {
  // initialize serial communication at 9600 bits per second for debugging:
  Serial.begin(9600);

  // make the pushbutton's pin an input:
  pinMode(pushButton, INPUT);
  pinMode(onkyoIn, INPUT);
  
  pinMode(irOut, OUTPUT);
#ifdef RO_DEBUG
  pinMode(LED_BUILTIN, OUTPUT);
#endif
}

// ###################################################################
// [3] MAIN LOOP
// ###################################################################

/** Main loop works as simple state machine.
 *  STATES:
 *  0: waiting for START marker (30 cycles of HIGH)
 *  1: got first high, waiting for at least @see START_MARKER_HIGH (back to 0 if not stable)
 *  2: waiting for pause
 *  3: waiting for next HIGH (and store bit depending on how it takes)
 *
 * The main loop runs with a cycle of 100us (more or less);
 * so to detect the START marker (3000us HIGH) it needs to see HIGH for at least
 * START_MARKER_HIGH times
 * The counter is also used to check whether the LOW after a bit was 2000
 * (by checking for > 11 times);
 * On a LOW for 28 times (error condition) the START detection is restarted.
 *
 * The trailer is just ignored after a command and next start marker is waited for.
 */
void loop() {
  static int state = 0;
  static long int last_change_ms;
  static int n_bit = 0;
  static int current_CMD = 0; 
  static int count;

  // read the input pin:
  int buttonPress = digitalRead(pushButton);
  // evaluate protocol input
  int isHigh = digitalRead(onkyoIn);
  long int start = micros();
#ifdef RO_DEBUG
  int prevState = state;
#endif  
  
  switch(state){
  case 0:
    if(isHigh){
      count = 1;
      state++;
    }
    break;
   
  case 1:
    if(isHigh){
      count++;
      if(count >= START_MARKER_HIGH){
	n_bit = N_BITS;
	current_CMD = 0;
	state++;
      }
    } 
    else {
      state = 0;
    }
    break;
    
  case 2:
    if(! isHigh){
      count = 0;
      state++;
    }
    break;
   
  case 3:
    if(isHigh){
      if(count > 28){ // error, restart
#ifdef RO_DEBUG
	Serial.println("IMPLAUSIBLE GAP - RESTART");
#endif
	state = 0;
      }
      else {
	if(n_bit < N_BITS){
	  if(count > 11){
	    current_CMD |= (1 << n_bit); 
	  }
	}
	n_bit--;
	if(n_bit < 0){
#if (defined RO_DEBUG) || 1
	  Serial.print("Next Onkyo CMD:");
	  Serial.println(current_CMD);
#endif
	  last_CMD = current_CMD;
	  last_change_ms = millis();
	  current_CMD = 0;
	  state = 0;          
	}
	state = 2;
      } 
    }
    else { 
      count++;
    }
    break;

  default:
    Serial.println("INTERNAL ERROR - RESTARTING");
    state = 0;
  }

#ifdef RO_DEBUG
  if(prevState != state){
    Serial.print("STATE:");
    Serial.println(state);
  }
#endif
  
  if((last_CMD > 0) && (millis() - last_change_ms > FORGET_AFTER_MS)){
    last_CMD = 0;   
  }

  // translate button / protocol to IR output
  printOutput(buttonPress, last_CMD);

  delayMicroseconds(100 - (micros() - start));
}

// -------------------------------------------------------------------------
