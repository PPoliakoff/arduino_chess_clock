/*
Chess clock
===========
P. poliakoff 2020

This has been tested with Arduino uno r3 and with Arduino micro

This sketch uses 2 LCD displays
The circuit:
* LCD RS pin to digital pin 12
* LCD Enable pin to digital pin 11 for LCD1 pin 10 for LCD 0
* LCD D4 pin to digital pin 5
* LCD D5 pin to digital pin 4
* LCD D6 pin to digital pin 3
* LCD D7 pin to digital pin 2
* LCD R/W pin to ground
* LCD VSS pin to ground
* LCD VCC pin to 5V
* 10K pot
* ends to +5V and ground
* wiper to LCD VO pin
* 
* switches
* player 0: pin 6
* player 1: pin 7
* pause : pin 8
*/

// Directly based on the Arduino example LiquidCrystal HelloWorld
#include <LiquidCrystal.h>
#include <EEPROM.h>

// EEPROM structure
const int play_time_minutes =0;
const int play_time_seconds = 1;
const int delay_seconds =2;

byte settings[4];
const byte MAX_SETTINGS[]={99,59,99,1}; // max 99:59 play time and max 99sec delay the last value is a boolean  No/Yes 
// hardware declaration
const int SWITCH[2]={6,7};
const int SWITCH_PAUSE=8;
LiquidCrystal lcd[2] = { LiquidCrystal(12, 11, 5, 4, 3, 2),LiquidCrystal(12, 10, 5, 4, 3, 2) };

// clock related variables
int long totalDelay;
int currentPlayer=0;
long playerTime[2];
long playerDelay[2];
bool paused=true;
bool setting=false;
int activeField;
const int DEBOUNCE_DELAY_MAX=10;
int debounceDelay;


void setup() {

// read settings from EEPROM
settings[play_time_minutes]=EEPROM.read(play_time_minutes);
settings[play_time_seconds]=EEPROM.read(play_time_seconds);
settings[delay_seconds]=EEPROM.read(delay_seconds);
if(settings[play_time_minutes]>MAX_SETTINGS[play_time_minutes]||
  settings[play_time_seconds]>MAX_SETTINGS[play_time_seconds]||
  settings[delay_seconds]>MAX_SETTINGS[delay_seconds])
 {// unitialized or corrupted settings
  // reset to default setting: 08:00 play time + 12 secondes delay
  settings[play_time_minutes]=8;
  settings[play_time_seconds]=0;
  settings[delay_seconds]=12;
  saveSettings(); 
 }


//setup switch
  pinMode(SWITCH[0],INPUT_PULLUP);
  pinMode(SWITCH[1],INPUT_PULLUP);
  pinMode(SWITCH_PAUSE,INPUT_PULLUP);

// set up the LCD's number of columns and rows:
  lcd[0].begin(16, 2);
  lcd[1].begin(16, 2);

 
 
  // setup interrupts on timer 2 for 8kHz
  cli();
  #ifdef __AVR_ATmega32U4__
    TCCR3A = 0;
    TCCR3B = 0;
    TCNT3  = 0;
    OCR3A = 249;// = (16*10^6) / (8000*8) - 1
    TCCR3B |= (1 << WGM32);  // configure timer3 for ctc
    TCCR3B |= (1 << CS31); //prescaler divied by 8   
    TIMSK3 |= (1 << OCIE3A);

  #else
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;
    // configure for 8khz 
    OCR2A = 249;// = (16*10^6) / (8000*8) - 1
    TCCR2A |= (1 << WGM21);  // configure timer2 for ctc
    TCCR2B |= (1 << CS21); //prescaler divied by 8   
    TIMSK2 |= (1 << OCIE2A);
  #endif
    sei();

  
if (digitalRead(SWITCH_PAUSE)==LOW)
 {
  //pause button was pressed at reset time we are in setup mode
  setting=true;
  activeField=0;
  settings[3]=0; // by default do not save
 }
 else
 {
  //pause button was not pressed at reset time: we ar in play mode
  // display initial values
  resetTime();
  }
}

void loop() 
{
if (setting!=0)
 {     
 char buf[17];
 int loopDelay=100;
 lcd[0].noBlink();
 lcd[1].noBlink();
 sprintf(buf,"Play time Min:%02d", settings[play_time_minutes]);
 printLCD(0,0,buf);
 sprintf(buf,"Play time Sec:%02d",settings[play_time_seconds]); 
 printLCD(0,1,buf);
 sprintf(buf,"Delaytime Sec:%02d",settings[delay_seconds]);
 printLCD(1,0,buf);
 sprintf(buf,"Save:    %s",settings[3]==0?"NO ":"YES");
 printLCD(1,1,buf);
 lcd[activeField>>1].setCursor(15,activeField&1);
 lcd[activeField>>1].blink();
 
 if (digitalRead(SWITCH[0])==LOW)
    { // increment setting
      settings[activeField]++;
      if(settings[activeField]>MAX_SETTINGS[activeField])
       {
        settings[activeField]=0;
       }
    }
  if (digitalRead(SWITCH[1])==LOW)
    { // decrement setting
      settings[activeField]--;
      if(settings[activeField]==255) // the setting is a byte so it wrap around at 255 instead of -1
       {
        settings[activeField]=MAX_SETTINGS[activeField];
       }
    }
  if (digitalRead(SWITCH_PAUSE)==LOW)
    {
    loopDelay=300; // prevent too fast field select change
    activeField++;
    if(activeField>3)
      {
        if(settings[3]==1)
          {
            setting=0; //exit setting mode
            saveSettings();
             // display initial values
            lcd[1].noBlink();
            resetTime();
          }
        activeField=0;
      }
    }
 delay(loopDelay);
 }
else
  { 
  if (digitalRead(SWITCH_PAUSE)==LOW  && debounceDelay==0)
    { // toggle pause
    debounceDelay=DEBOUNCE_DELAY_MAX;
    paused=!paused;
    printStatus();
    }
  else if (debounceDelay>0)
    {
    debounceDelay--;
    }

  if (digitalRead(SWITCH[currentPlayer])==LOW)
    { // swap active player
      currentPlayer^=1;
      playerDelay[currentPlayer^1]=totalDelay;
      printTime(currentPlayer^1);
    }
  //refresh the time display
  printTime(currentPlayer);
  delay(30);
  }
}


void resetTime()
{

  long totalTime=8000L*(settings[play_time_minutes]*60+settings[play_time_seconds]); // the interrup is triggered 8000 per second
  
  totalDelay=8000L*settings[delay_seconds]; // the interrup is triggered 8000 per second

  // initalize variables
  playerTime[0]=totalTime;
  playerDelay[0]=totalDelay;

  playerTime[1]=totalTime;
  playerDelay[1]=totalDelay;

  printTime(0);
  printTime(1);
  printStatus();  
}


//update the display top line in play mode
void printStatus()
{
  if(paused)
    {
    char buf[]="---- PAUSED ----";
    printLCD(0,0,buf);
    printLCD(1,0,buf);
    }
  else
    {    
    char buf[]="                ";
    printLCD(0,0,buf);
    printLCD(1,0,buf);
    }
}

// display the remaining time on the lower line
void printTime(int player)
{
char buf[]="****TIME OUT****";
if (playerTime[player]>0)
 {
  sprintf(buf,"%s %02d:%02d + %02d %s",
    player==currentPlayer?">>":"  ",
    int(playerTime[player]/8000/60),
    int(playerTime[player]/8000)%60,
    int((playerDelay[player]+7999)/8000),
    player==currentPlayer?"<<":"  ");
 }
printLCD(player,1,buf);
}

// display a message at on the specified line of the specified LCD display
void printLCD(int lcdIndex,int line,char *message)
{
lcd[lcdIndex].setCursor(0,line);
lcd[lcdIndex].print(String(message));
}

// transfer the content of "settings" to the EEPROM
void saveSettings()
{
  EEPROM.write(play_time_minutes,settings[play_time_minutes]);
  EEPROM.write(play_time_seconds,settings[play_time_seconds]);
  EEPROM.write(delay_seconds,settings[delay_seconds]);
}

//interrupt routine triggered 8000 times per second
#ifdef __AVR_ATmega32U4__
ISR(TIMER3_COMPA_vect)
#else
ISR(TIMER2_COMPA_vect)
#endif
{
  if(!paused)
    {
    if(playerDelay[currentPlayer]>0)
      {
        playerDelay[currentPlayer]--;
      }
    else
      {
      if(playerTime[currentPlayer]>0)
        {
        playerTime[currentPlayer]--;
        }
      }
    }
}
