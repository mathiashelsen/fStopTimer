#define USE_TIMER_1     true
#define USE_TIMER_2     false
#define USE_TIMER_3     false
#define USE_TIMER_4     false
#define USE_TIMER_5     false

#define TIMER_INTERVAL_MS        100L

#include <TimerInterrupt.h>
#include <TimerInterrupt.hpp>
#include <ISR_Timer.h>
#include <ISR_Timer.hpp>

#include <TM1637Display.h>
#include <Arduino.h>

#define DIGPIN_RELAY 10
#define DIGPIN_START  4
#define DIGPIN_ON 3
#define DIGPIN_FSTOP 5

#define ANAPIN_BASETIME A1
#define ANAPIN_FSTOP A0

#define DISPLAY_A_CLK 8
#define DISPLAY_A_DIO 9
#define DISPLAY_B_CLK 6
#define DISPLAY_B_DIO 7


TM1637Display display_a(DISPLAY_A_CLK, DISPLAY_A_DIO);
TM1637Display display_b(DISPLAY_B_CLK, DISPLAY_B_DIO);

typedef enum state {IDLE, EXP_NORMAL, EXP_FSTOP, WAIT_FSTOP} State;

volatile int numTicks;
volatile int baseTime;
volatile int fStop;
volatile int enableRelay;

volatile int fStopTicks[5];
volatile int numFStop;

volatile State myState;

void TimerHandler(void)
{
  switch(myState)
  {
    case IDLE: runIdleChecks(); break;
    case EXP_NORMAL: runExpNormal(); break;
    case EXP_FSTOP: runExpFStop(); break;
    case WAIT_FSTOP: waitFStop(); break;
    default: break;
  }

  // Do this every time
  digitalWrite(DIGPIN_RELAY, 1-enableRelay);
}

void runIdleChecks()
{
  // Read pot meters first
  uint32_t rawVal = analogRead(ANAPIN_BASETIME);
  rawVal += analogRead(ANAPIN_BASETIME);
  rawVal += analogRead(ANAPIN_BASETIME);
  rawVal += analogRead(ANAPIN_BASETIME);

  rawVal = rawVal / 16;
  rawVal = rawVal * 4;

  baseTime = map(rawVal, 0, 1024, 1, 301);
  display_a.showNumberDec(baseTime);

   // Read pot meters first
  rawVal = analogRead(ANAPIN_FSTOP);
  rawVal += analogRead(ANAPIN_FSTOP);
  rawVal += analogRead(ANAPIN_FSTOP);
  rawVal += analogRead(ANAPIN_FSTOP);

  rawVal = rawVal / 16;
  rawVal = rawVal * 4;

  fStop = map(rawVal, 0, 1024, -30, 31);
  display_b.showNumberDec(fStop);

  if(digitalRead(DIGPIN_ON) == 0)
  {
    enableRelay = 1;
  }

  if( digitalRead(DIGPIN_FSTOP) == 0)
  {
    int runningTicks = 0;
    for(int i = 0; i < 5; i++)
    {
      float currentStop = (i - 2.0)/2.0 * abs(fStop);
      fStopTicks[i] = (int) (baseTime * pow(2.0, currentStop/10.0));

      if(i > 0)
      {
        fStopTicks[i] -= runningTicks;
        
      }
      runningTicks += fStopTicks[i];

      Serial.println(fStopTicks[i]);
    }
    numFStop = 0;
    numTicks = fStopTicks[0];
    myState = EXP_FSTOP;
    enableRelay = 1;
  }

  if(digitalRead(DIGPIN_START) == 0)
  {
    numTicks = (int) (baseTime * pow(2.0, (float)fStop/10.0));
    myState = EXP_NORMAL;
    enableRelay = 1;
  }
}

void runExpNormal()
{
  if(numTicks <= 0)
  {
    enableRelay = 0;
    myState = IDLE;
    display_a.clear();
    display_a.showNumberDec(baseTime);
  }else
  {
    numTicks--;
    display_a.showNumberDec(numTicks);
  }
}

void runExpFStop()
{
  if(numTicks <= 0)
  {
    enableRelay = 0;
    numFStop++;
    if(numFStop > 4)
    {
      display_a.clear();
      display_a.showNumberDec(baseTime); 
      myState = IDLE;
    }
    else
    {
      display_a.clear();
      display_a.showNumberDec(fStopTicks[numFStop]);
      myState = WAIT_FSTOP;
    }
  }else
  {
    numTicks--;
    display_a.showNumberDec(numTicks);
  }
}

void waitFStop()
{
  if( digitalRead(DIGPIN_FSTOP) == 0)
  {
    enableRelay = 1;
    numTicks = fStopTicks[numFStop];
    myState = EXP_FSTOP;  
  }

}

void setup() {
   myState = IDLE;
  baseTime = 0;
  fStop = 0;
  enableRelay = 0; 
  
  display_a.setBrightness(0x01);
  display_a.clear();
  display_a.showNumberDec(baseTime);

  display_b.setBrightness(0x01);
  display_b.clear();
  display_b.showNumberDec(-30);

  pinMode(DIGPIN_RELAY, OUTPUT);
  digitalWrite(DIGPIN_RELAY, 1-enableRelay);

  pinMode(DIGPIN_START, INPUT_PULLUP);
  pinMode(DIGPIN_ON, INPUT_PULLUP);
  pinMode(DIGPIN_FSTOP, INPUT_PULLUP);

  int rawVal = analogRead(ANAPIN_BASETIME);
  baseTime = map(rawVal, 0, 1024, 10, 300);
  display_a.showNumberDec(baseTime);

  ITimer1.init();
  ITimer1.attachInterruptInterval(TIMER_INTERVAL_MS, TimerHandler);  

  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

}
