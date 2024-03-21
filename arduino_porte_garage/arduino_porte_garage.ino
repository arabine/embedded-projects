// Somfy Receiver
// =============================================================================================

#include <EEPROM.h>

// =============================================================================================
// RF SOMFY PARAMETERS
// =============================================================================================

// Set the pin that receives data from your 433.42 Mhz Receiver
#define TRANSCEIVER_RX_PIN 19

// Aurel transceivers have a pin that let the hardware switch to RX and TX mode
#define TRANSCEIVER_MODE_PIN 15

// Maximal remotes stored in the receivers eeprom
#define MAX_REMOTES 16

// Aurel transceivers have a pin that must be set to HIGH to enable the transmitter
#define TRANSCEIVER_ENABLE_PIN 22

// Auto switch off timer in seconds, set to 0 to disable.
#define SWITCH_OFF_SECONDS 3600


// =============================================================================================
// CONTROLE DU MOTEUR DE GARAGE
// =============================================================================================
const int pwmForwardPin = 11;
const int pwmBackwardPin = 10;
const int finDeCourseBas = 8; // capteur fin de course bas
const int finDeCourseHaut = 9; // capteur fin de course bas

const int onOffSwitch = 13;

#define PROG_BUTTON 2

// the output pin to control your relay to switch on or off your device
#define PIN_DEVICE  14


// DO NOT CHANGE THESE SETTINGS BELOW
#define TRANSCEIVER_TX HIGH
#define TRANSCEIVER_RX  LOW

int state = 0;                // status got increased during receiving radio signals. on state 3 we are ready to receive the payload data
bool halfway = false;         // Boolean is set if a statechange occurs at the start of a new bit. We then still have to wait for a falling or rising edge
int bits = 0;                 // counter for the received payload bits
uint8_t frame[7];             // payload buffer
uint8_t lastFrame[7];         // last received payload buffer, used to recognize repeated frames

struct Remote
{
  uint8_t rollingCode[2];     // the current rolling code 
  uint8_t address[3];         // the address of the remote
};

int remoteCount = 0;          // number of known remotes stored in the eeprom
Remote remotes[MAX_REMOTES];  // initialize an array for the maximum remotes stored in the eeprom

bool progModeEnabled = false; // Boolean that will be set to true if programming mode is enabled to add or delete a remote
int progModeSeconds = 0;      // Programming mode seconds counter. If counter hits 60 seconds programming mode will be terminated

bool switchedOn = false;      // Boolean that will be set to true if the device is switched on
int switchedOnSeconds = 0;    // Seconds counter. If counter hits SWITCH_OFF_SECONDS seconds the device will be switched off

bool progButtonPressed = false;
unsigned long progButtonMillis = 0;
unsigned long resetButtonMillis = 0;



// =============================================================================================
// PERIODIC RESET STUFF
// =============================================================================================
 static unsigned long startTime = 0;
 static const unsigned long oneDayInMs = 24 * 3600 * 1000;
void(* resetFunc) (void) = 0;


// =============================================================================================
// STATE CHART PORTE GARAGE
// =============================================================================================
typedef enum {
  PORTE_STOPPED,
  PORTE_MOVING,
  
} garage_state_t;

static bool actionPorteGarage = false;
static garage_state_t garageState = PORTE_STOPPED;
static unsigned long startGarageTime = 0;
static const unsigned long tenSecondsInMs = 20 * 1000;
#define DOOR_FORWARD  1
#define DOOR_BACKWARD  2

static int porteGarageDir = DOOR_FORWARD;


int fdc_bas = 0;
int fdc_bas_prev = 0;
int fdc_haut = 0;
int fdc_haut_prev = 0;

int on_off_switch_state = 0;
int on_off_switch_state_prev = 0;


void DoorGoForward()
{
  analogWrite(pwmBackwardPin, 0);
  analogWrite(pwmForwardPin, 255);
}

void DoorGoBackward()
{
  analogWrite(pwmBackwardPin, 255);
  analogWrite(pwmForwardPin, 0);
}

void DoorStop()
{
  garageState = PORTE_STOPPED;
  analogWrite(pwmBackwardPin, 0);
  analogWrite(pwmForwardPin, 0);
}


// =============================================================================================
// SETUP
// =============================================================================================
/*
 * the setup function runs once when you press reset or power the board
 */
void setup() 
{
  // set the Aurel transceiver to RX mode
  pinMode(TRANSCEIVER_MODE_PIN, OUTPUT);
  digitalWrite(TRANSCEIVER_MODE_PIN, TRANSCEIVER_RX);

  // initialize digital pin LEDs and device as an output.
  pinMode(PIN_DEVICE, OUTPUT);
  

  // PROGRAM BUTTON
  pinMode(PROG_BUTTON, INPUT_PULLUP);

  // enable Aurel transmitter
  pinMode(TRANSCEIVER_ENABLE_PIN, OUTPUT);
  digitalWrite(TRANSCEIVER_ENABLE_PIN, HIGH);

  // Porte de garage
  pinMode(pwmForwardPin, OUTPUT);
  pinMode(pwmBackwardPin, OUTPUT);
  pinMode(finDeCourseBas, INPUT_PULLUP);
  pinMode(finDeCourseHaut, INPUT_PULLUP);

  // onOffSwitch
  pinMode(onOffSwitch, INPUT_PULLUP);

  fdc_bas = digitalRead(finDeCourseBas);
  fdc_haut = digitalRead(finDeCourseHaut);
  fdc_haut_prev = fdc_haut;
  fdc_bas_prev = fdc_bas;


  on_off_switch_state = digitalRead(onOffSwitch);
  on_off_switch_state_prev = on_off_switch_state;

  DoorStop();

  
  // start serial communication
  Serial.begin(115200);
  Serial.println("\n\t=======================================");
  Serial.println("\tSomfy RTS receiver on the Arduino Mega.");
  Serial.println("\t======================================\n");

    

  // load previous programmed remotes from the eeprom
  loadRemotes();
  showRemotes();
  showHelp();

  startTime = millis();
  
  Serial.println("\tReceived frames:\n");
  
  // assign an interrupt on pin x on state change
  pinMode(TRANSCEIVER_RX_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(TRANSCEIVER_RX_PIN), stateChange, CHANGE);

  // setup two different timers
  noInterrupts(); // disable all interrupts
  setupTimer1();
  setupTimer3();
  interrupts(); // enable all interrupts
}


// Comment effacer les télécommandes :
// remoteCount = 0;
// saveRemotes();
// setProgMode(false);



static bool requestProgMode = false;
static bool newRemoteAdded = false;

/*
 * the loop function runs over and over again forever
 */


void loop() {
  static int d = 0;
  // read input from serial monitor
  if (Serial.available() > 0) {
    parseSerial((byte)Serial.read());
  }

  if (requestProgMode)
  {
    requestProgMode = false;
    Serial.println("Programming mode. Add or delete a remote by pressing the PROG button on the remote\n");
    setProgMode(true);

  }

  if (newRemoteAdded)
  {
    newRemoteAdded = false;
    Serial.print("\tAction: New remote added!");
  }

  // --------------------  CAPTEURS PORTE GARAGE   --------------------
  // LES capteurs MC005 sont actifs au niveau bas
  
  fdc_bas = digitalRead(finDeCourseBas);
  if (fdc_bas != fdc_bas_prev) {
    fdc_bas_prev = fdc_bas;
    Serial.print("--> Capteur BAS\n");
   // DoorStop();
  }

  fdc_haut = digitalRead(finDeCourseHaut);
  if (fdc_haut != fdc_haut_prev) {
    fdc_haut_prev = fdc_haut;
    Serial.print("--> Capteur HAUT\n");
   // DoorStop();
  }


  on_off_switch_state = digitalRead(onOffSwitch);
  if (on_off_switch_state != on_off_switch_state_prev) {
    on_off_switch_state_prev = on_off_switch_state;
    Serial.print("--> SWITCH !\n");
    actionPorteGarage = true;
   // DoorStop();
  }

  
  // --------------------  ACTION PORTE GARAGE   --------------------
  if (actionPorteGarage)
  {
    actionPorteGarage = false;
    startGarageTime = millis();

    
    if (garageState == PORTE_MOVING)
    {
      Serial.print("\tGarage: OFF");
      DoorStop();
    }
    else
    {
      garageState = PORTE_MOVING;
      Serial.print("\tGarage: ON");
      // ACTION DE LA PORTE

      if (porteGarageDir == DOOR_FORWARD)
      {
          DoorGoForward();
          porteGarageDir = DOOR_BACKWARD;
      }
      else
      {
          DoorGoBackward();
          porteGarageDir = DOOR_FORWARD;
      }
      
    }
  }


  if (garageState == PORTE_MOVING)
  {
    if ((millis() - startGarageTime) >= tenSecondsInMs)
    {
      startGarageTime = 0;
      Serial.print("\tGarage: OFF (timeout)");
      DoorStop();
    }
  }


  // --------------------  PERIODIC RESET   --------------------
 // if ((millis() - startTime) >= oneDayInMs)
 // {
 //   Serial.print("\tRESET!!!!");
 //   resetFunc(); //call reset
 // }

 
}

/*
 * Timer1 is used to measure the time elapsed between the pulse changes while receiving the payload.
 * 
 * We use 16mhz cpu / prescaler 8 = 2.000.000 counts per second (Hz). 
 * If we divide the counts by 2 then we have the microseconds: 16000000 / 8 / 2 = 1000000
 * 
 * And we set an overflow interrupt that we use as a timeout. So when the timer overflows the data is stated as corrupt and 
 * the payload capture will be aborted.
 */
void setupTimer1() 
{
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
  TCNT1 = 0;      // set counter to zero
  
  TCCR1B |= (1 << CS11); // 8 prescaler
  TIMSK1 |= (1 << TOIE1); // enable overflow interupt
}

/*
 * interupt service routine called when timer1 overflows which indicates corrupt data
 */
ISR(TIMER1_OVF_vect) { // Interrupt service run when Timer/Counter1 OVERFLOWs
  state = 0; // reset 
}

/*
 * With timer3 we will count seconds. This seconds are used for two different clocks.
 * 
 * 1. time out after one minute in programming mode if no new remote with PROG button is received
 * 2. time out after one hour after switch is turned on to switch it off automaticly
 * 
 * We use 16mhz cpu / prescaler 1024 - 1 = 15624 counts per second (Hz). 
 * 
 * And we set an timer compare interrupt so that we can count seconds if the counter hits the number of 15624.
 */
void setupTimer3() 
{
  // set timer interupt for each second
  TCCR3A = 0;     // set entire TCCR3A register to 0
  TCCR3B = 0;     // same for TCCR3B

  // set compare match register to desired timer count
  OCR3A = 15624;
  // turn on CTC mode:
  TCCR3B |= (1 << WGM12);
  // Set CS30 and CS32 bits for 1024 prescaler:
  TCCR3B |= (1 << CS30) | (1 << CS32);
  // enable timer compare interrupt:
  TIMSK3 |= (1 << OCIE3B);
}

/*
 * interupt service routine called on every second. Used for two long term timers
 */
ISR(TIMER3_COMPB_vect)
{
  // keep programming mode for a maximum of 60 seconds
  if(progModeEnabled) {
    progModeSeconds++;
    if(progModeSeconds >= 60) {
      setProgMode(false);
    }
  }

  // automatic turn off the device after SWITCH_OFF_SECONDS seconds
//  if(switchedOn) {
//    switchedOnSeconds++;
//    if(SWITCH_OFF_SECONDS && switchedOnSeconds >= SWITCH_OFF_SECONDS) {
//      toggleSwitch(false);
//    }
//  }
}


/*
 * Handle available commands on the serial monitor
 */
void parseSerial(byte button)
{
  // do not accept buttons that are under ascii 20
  if(button < 20) {
    return;
  }
  
  // accept uppercase too
  if(button >= 65 && button <= 90) {
    button += 32;
  }

  switch (button) {
    case 'r' :
      Serial.println("Reset. All programmed remotes are now deleted\n");
      remoteCount = 0;
      saveRemotes();
      break;
    case 'p' :
      requestProgMode = true;
      break;
    case '1' :
      Serial.println("On. The device is switched on and will automaticly switched off after one hour\n");
      toggleSwitch(true);
      break;
    case '0' :
      Serial.println("Off. The device is off.\n");
      toggleSwitch(false);
      break;
    case 'l' :
      showRemotes();
      break;
    case 'h' :
      showHelp();
      break;
    default:
      Serial.println("Unknown command. Available commands are R for Reset, P for Programming a new remote, 1 and 0 for On and Off\n");
      toggleSwitch(false);
      break;
  }
}

/*
 * this function will be called on every falling or rising edge from the receivers RX pin
 * 
 * It changes the state if new data starts
 * 
 * state 0: Idle
 * state 1: Hardware sync received
 * state 2: Software sync received
 * state 3: Receiving data which will be processed if we received 56 bits 
 */
void stateChange() {
  int val = digitalRead(TRANSCEIVER_RX_PIN);  
  int tm = TCNT1 / 2;
  
  switch (state) {
    case 0: // found a hardware sync HIGH - Goto state 1
      if(inRange(tm, 2416) && val == LOW) {
        state = 1;
      }
      break;
    case 1: // found a hardware sync LOW - Goto state 2
      if(inRange(tm, 2416) && val == HIGH) {
        state = 2;
      }
      break;
    case 2: // found a software sync! - Goto state 3 - Ready to receive the bites
      if(inRange(tm, 4750) && val == LOW) {
        state = 3;
        bits = 0;
      }
      break;
    case 3: // receiving payload
      int b = bits / 8;
      if(inRange(tm, 604)) {
        if(halfway) {
          bitWrite(frame[b], 7 - bits % 8, val);
          bits++;
          halfway = false;
        } else {
          halfway = true;
        }
      } else {
        bitWrite(frame[b], 7 - bits % 8, val);
        bits++;
      }
      if(bits == 56) {
        state = 0;

        if(!isRepeatedFrame()) {
          memcpy(&lastFrame[0], &frame[0], 7);
          processFrame();
        }
        
      }
      break;
    default:
      state = 0;
      break;
  }
  
  TCNT1 = 0;
}



/*
 * after we have received 56 bits (or 7 bytes) of data we will process it
 */
void processFrame() 
{
  // unscramble
  unscrambleFrame(frame);

  int btn = getCommand(frame);            // which button is pressed on the remote?
  Remote r;                               // initialize a new remote object and copy the received data in it
  memcpy(&r, &frame[2], 5);
  int index = findRemoteWithAddress(r);   // try to find a remote in our list with the same address. Will be -1 if it is an unknown remote


  if(calculateChecksum(frame) != 0) {
    Serial.println("\tChecksum mismatch!");
    return;
  }
  
  Serial.print("\tData (payload): ");     // print received data on the serial monitor
  printHex(frame, 7);

  Serial.print("\tCommand: ");
  Serial.print(btn);

  Serial.print("\tRolling code: ");
  Serial.print(getRollingCode(r));

  // La prochaine trame reçue: on l'enregistre si on est en prog mode
  if(progModeEnabled) {
    if(index == -1) {
      if(addRemote(r)) {
        newRemoteAdded = true;
      } else {
        Serial.print("\tERROR: No space available for new remote!");
      }
    } else {
      deleteRemote(index);
      Serial.print("\tAction: Remote deleted!");
    }
    setProgMode(false);
  }

  bool valid = false;
  if(index >= 0 && isRollingCodeValid(index, r))
  {
    valid = true;
  }

  if (!valid)
  {
    Serial.print("\tInvalid rolling code.");
    return;
  }

  switch(btn) {
  //  case 8: // PROG button
      
  //    break;
    case 1: // MY button
        Serial.print("\tAction: MY");
      break;
    case 2: // UP button
        Serial.print("\tAction: ON");
        actionPorteGarage = true;
     //   toggleSwitch(true);
     
      break;
    case 4: // DOWN button
        Serial.print("\tAction: OFF");
        actionPorteGarage = true;
      //  toggleSwitch(false);
      break;
    case 15:
      Serial.print("\tAction: INVERT");
      actionPorteGarage = true;
      break;
 }

  Serial.println("\n");
}

/*
 * When a frame has been received it is scrambled. This function decodes the received frame
 */
void unscrambleFrame(uint8_t* frame) 
{
  uint8_t temp[7];

  memcpy(temp, frame, 7);
  
  // unscramble
  for (int i=1; i < 7; i++) {
    frame[i] = temp[i] ^ temp[i-1];
  }
}

/*
 * After that a frame has been decoded we could calculate the checksum which should be always zero on received frames.
 */
uint8_t calculateChecksum(uint8_t *frame)
{
  // Checksum calculation: a XOR of all the nibbles
  byte checksum = 0;
  for (byte i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  
  return checksum & 0b1111; // We keep the last 4 bits only
}

/*
 * Compare received frames to avoid repeations
 */
bool isRepeatedFrame()
{
  for(int i = 0; i < 7 ; i++) {
    if(frame[i] != lastFrame[i]) {
      return false;
    }
  }

  return true;
}

/*
 * We count micro seconds between RX state changes but it doesn't need to be precise and may be 150 micros less or over
 */
bool inRange(int tm, int ms) {
   return tm > ms-150 && tm < ms+150;
}

/*
 * get the command (pressed button(s)) out of the data
 */
int getCommand(uint8_t* frame) {
  return frame[1] >> 4;
}

/*
 * add a new remote to the remotes array and save it to the eeprom
 */
bool addRemote(Remote newRemote) 
{
  if(remoteCount >= MAX_REMOTES - 1) {
    return false;
  }
  
  remotes[remoteCount++] = newRemote;
  saveRemotes();

  return true;
}

/*
 * Delete a remote from the remotes array and save the array to the eeprom
 */
void deleteRemote(int index) {
  if(index < remoteCount-1) {
    for(int i = index + 1 ; i < remoteCount ; i++) {
      memcpy(&remotes[i-1], &remotes[i], 5);
    }
  }
  remoteCount--;
  saveRemotes();
}

/* 
 *  check if remote is already in the remotes array and return remote index or -1
 */ 
int findRemoteWithAddress(Remote r) {
  for(int i = 0 ; i < remoteCount ; i++) {
    if(memcmp(&r.address[0], &remotes[i].address[0], 3) == 0) {
      return i;
    }
  }

  return -1;
}

/*
 * check if a rolling code is valid. The remote may be 25 presses before the receiver
 * Also save the remotes to the eeprom again to update the rolling code there
 */
bool isRollingCodeValid(int index, Remote r) 
{
  if(getRollingCode(r) - getRollingCode(remotes[index]) < 25 ||
  (getRollingCode(r) +100) - (getRollingCode(remotes[index]) +100) < 25) { // in case of overflow 
    memcpy(&remotes[index].rollingCode[0] , &r.rollingCode[0], 2);
    saveRemotes();
    return true;
  }

  return false;
}

/*
 * get the rolling code into an unsigned integer so that we can increase and compare the rolling codes
 */
unsigned int getRollingCode(Remote r)
{
  unsigned int rollingCode;
  
  memcpy(&rollingCode, &r.rollingCode[0], 1);
  rollingCode = rollingCode << 8;
  memcpy(&rollingCode, &r.rollingCode[1], 1);

  return rollingCode;
}

/*
 * Load the remotes array from the eeprom
 */
void loadRemotes() {
  EEPROM.get(0, remoteCount);
  for(int i = 0 ; i < remoteCount ; i++) {
    EEPROM.get(2 + i * sizeof(Remote), remotes[i]);
  }
}

/*
 * Save the remotes array to the eeprom
 */
void saveRemotes() {
  EEPROM.put(0, remoteCount);
  for(int i = 0 ; i < remoteCount ; i++) {
    EEPROM.put(2 + i * sizeof(Remote), remotes[i]);
  }
}

/*
 * Enable or disable the programming mode
 */
void setProgMode(bool mode) {
  progModeSeconds = 0;
  progModeEnabled = mode;
}

/*
 * Switch the device on or off
 */
void toggleSwitch(bool on) {
  switchedOnSeconds = 0;
  switchedOn = on;
  digitalWrite(PIN_DEVICE, on); 
}

/*
 * help function to print hexadecimal values on the serial monitor with leading zeros
 */
void printHex(uint8_t* bytes, int len) {
  for (byte i = 0; i < len; i++) {
    if (bytes[i] >> 4 == 0) {  // Displays leading zero in case the most significant nibble is a 0.
      Serial.print("0");
    }
    Serial.print(bytes[i], HEX); Serial.print(" ");
  }
}

/*
 * show information about the remotes
 */
void showRemotes()
{
  Serial.print("\tNumber of remotes in memory: ");
  Serial.print(remoteCount, HEX);
  Serial.println("\n");
  for(int i = 0 ; i < remoteCount ; i++) {
    Serial.print("\tRemote ");
    Serial.print(i+1);
    Serial.print(":\tAddress: ");
    printHex(remotes[i].address, 3);
    Serial.print("\tRolling code: ");
    printHex(remotes[i].rollingCode, 2);
    Serial.println("");
  }
  Serial.println("");
}

/*
 * show help information
 */
void showHelp()
{
  Serial.println("\tAvailable commands:\n");
  Serial.println("\t1:\t Switch the device on");
  Serial.println("\t0:\t Switch the device off");
  Serial.println("\tP:\t Enter programming mode to add a new receiver");
  Serial.println("\tL:\t List the remotes stored in the eeprom");
  Serial.println("\tH:\t Show this help list");
  Serial.println("\tR:\t Reset the receiver. Be carefull, all remotes will be deleted!\n\n");
  
  Serial.println("\tPROG button:\n");
  Serial.println("\tpress 2.5 seconds:\t Enter programming mode to add a new receiver");
  Serial.println("\tpress 30 seconds:\t Reset the receiver. Be carefull, all remotes will be deleted!\n\n");
}
