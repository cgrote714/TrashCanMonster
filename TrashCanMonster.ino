/* lines 274 to 290 are where the prop timing is adjusted */

#include <EEPROM.h>
const int pinButton = 0;
const int pinTrigger = 1;
const int pinRelaySFX1 = 4;
const int pinRelaySFX2 = 5;
const int pinRelaySFX3 = 6;
const int pinRelaySFX4 = 7;
const int pinRelayLight = 3;
const int pinRelayValve = 2;
const int Short = 1;
const int Hold = 2;
const int ShortReset = 0;
const int HoldReset = -1;
long timeButtonOn = 0;
long timeButtonOff = 0;
int modeButton = 0;
String sStep;
long tStep = 0;
int RunMode = 0;
String sMenu; //name of current menu we are in
long tAltShow = 0; //alternating message timer
int mAltShow = 0; //current alternating message
long tMenu = 0; //time since last menu option navigated (for timeout return to main mode)
int SFX1 = 0;
int SFX2 = 0;
int SFX3 = 0;
int SFX4 = 0;
int SFXs = 0; //SFX sequential or random
int StartupMode=0; //0=always start in Mode Off, 1=resume last set mode
int curSound=0; //current sound we are editing
String sOrder;
String sStartup;
int manualPos=0;
long tSleep=0;
int triggerCount=0;
int triggerCenter;
const int triggerThreshold=30;
const int triggerCountThreshold=500;
String propMode = "Idle";
int SelectedSound;
int LastSound=0;
long tnt=0; //time not triggered

char ESC = 0xFE;

byte s1[8] = {
  0b11011,
  0b10011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b10001,
  0b11111
};
byte s2[8] = {
  0b10001,
  0b01110,
  0b11110,
  0b11101,
  0b11011,
  0b10111,
  0b00000,
  0b11111
};
byte s3[8] = {

  0b00000,
  0b11101,
  0b11011,
  0b11101,
  0b11110,
  0b01110,
  0b10001,
  0b11111
};
byte s4[8] = {
  0b11101,
  0b11001,
  0b10101,
  0b01101,
  0b00000,
  0b11101,
  0b11101,
  0b11111
};

void setup() {
  // Initialize LCD
  Serial1.begin(9600);
  initLCD(); 
  WriteLCD1("Trashcan Monster");
  WriteLCD2("(c)2016 C. Grote");
  LCDon();

  // Initialize Trigger
  initTrigger();
  
  // Initialize Prop
  randomSeed(analogRead(5));
  initOutputs();
  loadEEPROM();
  delay(2000);
  ClearLCD();
  displayMode();
  sMenu = "Mode";
  tnt=millis();
  propMode = "Completed"; //initial state confirms trigger is not active before enabling
}

void loop(){
  if (sMenu=="Mode") {
    altShow("Press to change","Hold for options");
    tMenu=0;
    if (ButtonPressed() == Short) {
      modeButton=ShortReset;
      RunMode += 1;
      if (RunMode > 3) {RunMode = 0;};
      displayMode();
      saveEEPROM();
      tAltShow=0;
    }
    if (ButtonPressed() == Hold) {
      modeButton=HoldReset;
      saveEEPROM();
      sMenu="Sounds";
      tMenu=millis();
      ClearLCD();
      curSound=1;
      navSound(curSound);
      tAltShow=0;
    }
  }
  
  if (sMenu=="Sounds") {
    altShow("Hold to toggle","Press to advance");
    if (ButtonPressed() == Short) {
      modeButton=ShortReset;
      curSound+=1;
      if (curSound>4) {
        LCDbcOff();
        saveEEPROM();
        sMenu="Order";
        tAltShow=0;
        tMenu=millis();
        ClearLCD();
        WriteLCD1("Sound Order: " + sOrder);
      }
      navSound(curSound);
    }
    if (ButtonPressed() == Hold) {
      modeButton=HoldReset;
      if (curSound==1 && SFX1 != 1) {SFX1=1;}
      else if (curSound==1 && SFX1==1) {SFX1=0;}
      else if (curSound==2 && SFX2 != 1) {SFX2=1;}
      else if (curSound==2 && SFX2==1) {SFX2=0;}
      else if (curSound==3 && SFX3 != 1) {SFX3=1;}
      else if (curSound==3 && SFX3==1) {SFX3=0;}
      else if (curSound==4 && SFX4 != 1) {SFX4=1;}
      else if (curSound==4 && SFX4==1) {SFX4=0;}
      navSound(curSound);
    }
  }

  if (sMenu=="Order") {
    altShow("Hold to change","Press for next");
    if (ButtonPressed() == Short) {
      modeButton=ShortReset;
      saveEEPROM();
      sMenu="Startup";
      tAltShow=0;
      ClearLCD();
      WriteLCD1("PowerOn: " + sStartup);
    }
    if (ButtonPressed() == Hold) {
      modeButton=HoldReset;
      if (SFXs != 1) {SFXs=1;} else {SFXs=0;}
      sOrder = "Rnd";
      if (SFXs==1) {sOrder="Seq";}
      WriteLCD1("Sound Order: " + sOrder);
    }
  }

  if (sMenu=="Startup") {
    altShow("Hold to change","Press for next");
    if (ButtonPressed() == Short) {
      modeButton=ShortReset;
      saveEEPROM();
      sMenu="Manual";
      tAltShow=0;
      ClearLCD();
      WriteLCD1("Down  Read:    ");
    }
    if (ButtonPressed() == Hold) {
      modeButton=HoldReset;
      if (StartupMode != 1) {StartupMode=1;} else {StartupMode=0;}
      sStartup = "Off";
      if (StartupMode==1) {sStartup="Restore";};
      WriteLCD1("PowerOn: " + sStartup);
    }
  }

  if (sMenu=="Manual") {
    tMenu=0; //stay parked in this screen - no timeout
    tSleep=millis(); //no sleep in this screen either
    updateManRead();
    if (manualPos==0) {
      altShow("Hold to RAISE","Press for next");
      if (ButtonPressed() == Short) {
          modeButton=ShortReset;
          sMenu="Mode";
          tAltShow=0;
          displayMode();
      }
      if (ButtonPressed() == Hold) {
        modeButton=HoldReset;
        OpenValve();
        manualPos=1;
        WriteLCD1("Up    Read:   ");
        tAltShow=0;
      }
    }
    else {
      altShow("Hold to LOWER","Hold to LOWER");
      if (ButtonPressed() == Short) {
        modeButton=ShortReset;
        // do nothing
      }
      if (ButtonPressed() == Hold) {
        modeButton=HoldReset;
        CloseValve();
        manualPos=0;
        WriteLCD1("Down  Read:    ");
        tAltShow=0;
      }
    }
  }

  if (sMenu=="Sleep") {
    if (ButtonPressed() == Short) {
        modeButton=ShortReset;
        // do nothing
    }
    if (ButtonPressed() == Hold) {
        modeButton=HoldReset;
        tSleep=millis();
        LCDback(8);
        sMenu="Mode";
        tAltShow=0;
    }
  }

  if (sMenu != "Manual") {
    // Regular prop run routine
    if (propMode=="LightOff") {SoundsOff();propMode="Completed";}
    if (propMode=="ValveClosed") {LightOff();}
    if (propMode=="ReadyToClose") {CloseValve();}
    if (propMode=="ValveOpen") {OpenDelay();}
    if (propMode=="SoundPlaying") {OpenValve();}
    if (propMode=="LightOn") {PlaySound();}
    if (propMode=="Triggered") {SelectSound();LightOn();}
    CheckTrigger();
  }
  
  if (sMenu != "Mode" && tMenu !=0 && millis()-tMenu>10000) {LCDbcOff();saveEEPROM();sMenu="Mode";displayMode();}
  if (sMenu != "Sleep" && millis()-tSleep > 30000) {LCDback(1);sMenu="Sleep";WriteLCD2("Hold to wake");}
  
}

void SoundsOff() {
  digitalWrite(pinRelaySFX1, HIGH);
  digitalWrite(pinRelaySFX2, HIGH);
  digitalWrite(pinRelaySFX3, HIGH);
  digitalWrite(pinRelaySFX4, HIGH);
}

void OpenDelay() {
  // certain sounds may take longer to finish or may want to add canned air
  if (SelectedSound==1) {delay(2000);};
  if (SelectedSound==2) {delay(1710);};
  if (SelectedSound==3) {delay(2200);};
  if (SelectedSound==4) {delay(1500);};
  propMode="ReadyToClose";
}

void PlaySound() {
  // some sounds may take a while to get started
  if (RunMode==1 || RunMode==3) {
  if (SelectedSound==1) {digitalWrite(pinRelaySFX1, LOW);delay(100);};
  if (SelectedSound==2) {digitalWrite(pinRelaySFX2, LOW);delay(100);};
  if (SelectedSound==3) {digitalWrite(pinRelaySFX3, LOW);delay(10);};
  if (SelectedSound==4) {digitalWrite(pinRelaySFX4, LOW);delay(300);};
  }
  propMode="SoundPlaying";
}

void initTrigger() {
  // see if trigger is alternating.  If it is, wait for it to stop.  Set center value.
  triggerCenter=0;
  int minVal=1023;
  int maxVal=0;  
  int checks=0;
  while (triggerCenter==0) {
    if (analogRead(pinTrigger) < minVal) {minVal = analogRead(pinTrigger);}
    if (analogRead(pinTrigger) > maxVal) {maxVal = analogRead(pinTrigger);}
    if (maxVal-minVal < 10) {triggerCenter=analogRead(pinTrigger);}
    checks += 1;
    if (checks > 2000) {
      checks=0;
      maxVal=0;
      minVal=0;
    }
  }
}

void CloseValve() {
  if (RunMode>1 || sMenu=="Manual") {digitalWrite(pinRelayValve, HIGH);}
  propMode="ValveClosed";
}

void OpenValve() {
  if (RunMode>1 || sMenu=="Manual") {digitalWrite(pinRelayValve, LOW);}
  propMode="ValveOpen";
}

void LightOff() {
  if (RunMode > 0) {digitalWrite(pinRelayLight, HIGH);}
  propMode="LightOff";
}

void LightOn() {
  if (RunMode > 0) {digitalWrite(pinRelayLight, LOW);}
  propMode="LightOn";
}

void SelectSound() {
  int numSounds = SFX1+SFX2+SFX3+SFX4;
  if (SFXs==1) {
    // Sequential
    SelectedSound=LastSound+1;
    if (SelectedSound>numSounds) {SelectedSound=1;}
    if (SFX1==0 && SelectedSound==1) {SelectedSound=2;}
    if (SFX2==0 && SelectedSound==2) {SelectedSound=3;}
    if (SFX3==0 && SelectedSound==3) {SelectedSound=4;}
    if (SFX4==0 && SelectedSound==4) {SelectedSound=1;}
    LastSound=SelectedSound;
  }
  else {
    // Random
    SelectedSound=random(1,numSounds);
    int pos=0;
    for (int i=1; i<=4; i++) {
      if (i==1 && SFX1==1) {pos+=1;}
      if (i==2 && SFX2==1) {pos+=1;}
      if (i==3 && SFX3==1) {pos+=1;}
      if (i==4 && SFX4==1) {pos+=1;}
      if (pos==SelectedSound) {SelectedSound=i;}
    }
  }
}

void SelectSound2() {
  if (SFXs==1) {
    SelectedSound=LastSound+1;
    if (SelectedSound>4) {SelectedSound=1;};
  }
  else {
    SelectedSound=random(1,4);
  }
  LastSound=SelectedSound;
}

void CheckTrigger() {
  if (isTriggered() && propMode=="Idle") {triggerCount+=1;} 
  if (triggerCount > triggerCountThreshold && propMode=="Idle") {propMode="Triggered";triggerCount=0;}
  if (propMode=="Completed") {
    if (isTriggered()==true) {tnt=millis();}
    if (isTriggered()==false && millis()-tnt > 2000) {propMode="Idle";}
  }
  else {tnt=millis();}
}

boolean isTriggered() {
  if (abs(analogRead(pinTrigger) - triggerCenter) > triggerThreshold) {return true;}
  else {return false;}
  
  //if (readTrigger()>60) {return true;}
  //else {return false;} 
}

int readTrigger() {
  int minVal=1023;
  int maxVal=0;
  for (int i; i<1500; i++) {
    if(analogRead(pinTrigger)<minVal) {minVal=analogRead(pinTrigger);}
    if(analogRead(pinTrigger)>maxVal) {maxVal=analogRead(pinTrigger);}
  }
  return maxVal-minVal;
}
void updateManRead() {
  
  if (millis() % 347 == 0) {
    initTrigger();
    Serial1.write(ESC);Serial1.write(0x45); Serial1.write(0x00);
    LCDmoveN(13);
    Serial1.print(RightPad(String(analogRead(pinTrigger))," ",4));
    //Serial1.print(RightPad(String(readTrigger())," ",4));
  }
}

String RightPad(String s1, String s2, int n1)  {
  if (s1.length() < n1) {
    for (int x=s1.length(); x < n1; x++) {
      s1+=s2;
    }
  }
  return s1;
}

void altShow(String msg1, String msg2) {
  if (tAltShow==0) {WriteLCD2(msg1);tAltShow=millis();mAltShow=1;navSound(curSound);}
  if (tAltShow != 0 && millis()-tAltShow > 2000 && mAltShow==1) {WriteLCD2(msg2);tAltShow=millis();mAltShow=2;navSound(curSound);}
  if (tAltShow != 0 && millis()-tAltShow > 2000 && mAltShow==2) {WriteLCD2(msg1);tAltShow=millis();mAltShow=1;navSound(curSound);}
}

void navSound(int sfx) {
  if (sMenu=="Sounds") {
  loadSounds();
  Serial1.write(ESC);Serial1.write(0x45); Serial1.write(0x00);
  LCDmoveN(7+(2*sfx));
  LCDbcOn();
  }
}

int ButtonPressed() {
  if(analogRead(pinButton) > 512) {
    if (timeButtonOn < 10000) {timeButtonOn += 1;}
    if (timeButtonOn > 4000 && modeButton == ShortReset) {modeButton=Hold;tMenu=millis();tSleep=millis();}
    if (modeButton != ShortReset) {timeButtonOn=0;}
    timeButtonOff = 0;
    } 
  else {
    if (timeButtonOff < 1000) {timeButtonOff += 1;};
    if (timeButtonOff > 50 && timeButtonOn > 50) {modeButton=Short;timeButtonOn=0;tMenu=millis();tSleep=millis();};
    if (timeButtonOff > 50 && modeButton == HoldReset) {modeButton=ShortReset;timeButtonOn=0;};    
  }
  return modeButton;
}

void loadEEPROM() {
  RunMode = EEPROM.read(0);
  SFX1 = EEPROM.read(1);
  SFX2 = EEPROM.read(2);
  SFX3 = EEPROM.read(3);
  SFX4 = EEPROM.read(4);
  SFXs = EEPROM.read(5);
  sOrder = "Rnd";
  if (SFXs==1) {sOrder = "Seq";}
  StartupMode = EEPROM.read(6);
  if (StartupMode==0) {RunMode=0;};
  sStartup = "Off";
  if (StartupMode==1) {sStartup="Restore";};
}

void saveEEPROM() {
  EEPROM.write(0,RunMode);
  EEPROM.write(1,SFX1);
  EEPROM.write(2,SFX2);
  EEPROM.write(3,SFX3);
  EEPROM.write(4,SFX4);
  EEPROM.write(5,SFXs);
  //StartupMode=1;
  EEPROM.write(6,StartupMode);
}

void displayMode() {
  if (RunMode==0) {WriteLCD1("Mode: Off");}
  if (RunMode==1) {WriteLCD1("Mode: Sound Only");}
  if (RunMode==2) {WriteLCD1("Mode: Move Only");}
  if (RunMode==3) {WriteLCD1("Mode: Sound+Move");}
}

void initOutputs() {
  // Set output to high before setting mode to output
  // to prevent false triggering of ACTIVE LOW relays
  digitalWrite(pinRelaySFX1,HIGH);pinMode(pinRelaySFX1,OUTPUT);
  digitalWrite(pinRelaySFX2,HIGH);pinMode(pinRelaySFX2,OUTPUT);
  digitalWrite(pinRelaySFX3,HIGH);pinMode(pinRelaySFX3,OUTPUT);
  digitalWrite(pinRelaySFX4,HIGH);pinMode(pinRelaySFX4,OUTPUT);
  digitalWrite(pinRelayLight,HIGH);pinMode(pinRelayLight,OUTPUT);
  digitalWrite(pinRelayValve,HIGH);pinMode(pinRelayValve,OUTPUT);
}

void loadSounds(){
  WriteLCD1("Sounds:");
  Serial1.write(ESC);Serial1.write(0x45); Serial1.write(0x00);
  LCDmoveN(9);
  if (SFX1==1) {Serial1.write(1);} else {Serial1.write("1");};
  LCDmoveN(2);
  if (SFX2==1) {Serial1.write(2);} else {Serial1.write("2");};
  LCDmoveN(2);
  if (SFX3==1) {Serial1.write(3);} else {Serial1.write("3");};
  LCDmoveN(2);
  if (SFX4==1) {Serial1.write(4);} else {Serial1.write("4");};
}

void LCDbcOn(){Serial1.write(ESC);Serial1.write(0x4B);}
void LCDbcOff(){Serial1.write(ESC);Serial1.write(0x4C);}

void LCDmoveN(int m){
  for(int x=1; x<m; x++){Serial1.write(0xFE);Serial1.write(0x4A);}
}

void ClearLCD(){Serial1.write(ESC); Serial1.write(0x51);}

void WriteLCD1(String stext){
  Serial1.write(ESC);
  Serial1.write(0x45);
  Serial1.write(0x00);
  Serial1.print(stext);
  for(int x=stext.length(); x<16; x++){Serial1.print(" ");}
}

void WriteLCD2(String stext){
  String ch = "";
  Serial1.write(ESC);
  Serial1.write(0x45);
  Serial1.write(0x40);
  Serial1.print(stext);
  for(int x=stext.length(); x<16; x++){Serial1.print(" ");}
}

void LCDon(){Serial1.write(ESC); Serial1.write(0x41);}

void LCDoff(){Serial1.write(ESC); Serial1.write(0x42);}

void initLCD() {

  // Define custom characters

  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(1);Serial1.write(s1,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(2);Serial1.write(s2,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(3);Serial1.write(s3,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(4);Serial1.write(s4,8);

  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(1);Serial1.write(s1,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(2);Serial1.write(s2,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(3);Serial1.write(s3,8);
  Serial1.write(ESC);Serial1.write(0x54);Serial1.write(4);Serial1.write(s4,8);
  
  // Initialize LCD module
  LCDoff();
  ClearLCD();

  // Set Contrast
  Serial1.write(ESC);
  Serial1.write(0x52);
  Serial1.write(50);
  // Set Backlight
  Serial1.write(ESC);
  Serial1.write(0x53);
  Serial1.write(8);
  
  LCDbcOff();
  
}

void LCDback(int brightness) {
  Serial1.write(ESC);
  Serial1.write(0x53);
  Serial1.write(brightness);
}

