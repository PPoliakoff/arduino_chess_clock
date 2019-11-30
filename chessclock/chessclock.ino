/*
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
*/

// Directly based on the Arduino example LiquidCrystal HelloWorld
#include <LiquidCrystal.h>

const int SWITCH[2]={6,7};
const int SWITCH_PAUSE=8;
const long totalTime=8L*60L*8000L;
const long delaySetting=12L*8000L;
const int DEBOUNCE_DELAY_MAX=50;

LiquidCrystal lcd[2] = { LiquidCrystal(12, 11, 5, 4, 3, 2),LiquidCrystal(12, 10, 5, 4, 3, 2) };
int currentPlayer=0;
long playerTime[2];
long playerDelay[2];
bool paused=false;
int debounceDelay;



void setup() {

//setup switch
  pinMode(SWITCH[0],INPUT_PULLUP);
  pinMode(SWITCH[1],INPUT_PULLUP);
  pinMode(SWITCH_PAUSE,INPUT_PULLUP);

// set up the LCD's number of columns and rows:
  lcd[0].begin(16, 2);
  lcd[1].begin(16, 2);

// initalize variables
  playerTime[0]=totalTime;
  playerTime[1]=playerTime[0];
  playerDelay[0]=delaySetting;
  playerDelay[1]=playerDelay[0];
  
// setup interrupts on timer 2
  cli();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  // configure for 8khz 
  OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS21);   
  TIMSK2 |= (1 << OCIE2A);
  sei();

}

void loop() 
{
if (digitalRead(SWITCH_PAUSE)==LOW  && debounceDelay==0)
  {
  debounceDelay=DEBOUNCE_DELAY_MAX;
  paused=!paused;
  if(paused)
    {
    printLCD(0,0,"PAUSED");
    printLCD(1,0,"PAUSED");
    }
  else
    {
    printLCD(0,0,"      ");
    printLCD(1,0,"      ");
    }
  }
else if (debounceDelay>0)
  {
  debounceDelay--;
  }
if (digitalRead(SWITCH[currentPlayer])==LOW)
  {
    currentPlayer^=1;
    playerDelay[currentPlayer^1]=delaySetting;
  }
char buf[32];
sprintf(buf,"%02d:%02d +%02d",int(playerTime[currentPlayer]/8000/60),int(playerTime[currentPlayer]/8000)%60,int((playerDelay[currentPlayer]+7999)/8000));
printLCD(currentPlayer,1,String(buf));
delay(30);
}

void printLCD(int lcdIndex,int line,String message)
{
lcd[lcdIndex].setCursor(0,line);
lcd[lcdIndex].print(message);
}

ISR(TIMER2_COMPA_vect)
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
