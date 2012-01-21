// Demo sketch for a speed controlled stepper with an LCD connected to I2C port via MCP23008 I/O expander
// 2012 KvL http://opensource.org/licenses/mit-license.php

#include <PortsLCD.h>
#include "TimerOne.h"

#define MinPulseTime (200)
#define MaxPulseTime (200*1000)

PortI2C _myI2C (1);
LiquidCrystalI2C _lcd (_myI2C);
Port _myMotor (2);
Port _altButtons (3);

volatile long pulses; //updated in ISR
long usBetweenPulses = 0;
bool positiveDirection = true;

void setup() {

  // turn the backlight off, briefly
  _lcd.backlight();
  _myMotor.mode(OUTPUT);
  // set up the LCD's number of rows and columns: 
  _lcd.begin(16, 2);
  // Print a message to the LCD.
  _lcd.print("Single axis");
  _lcd.setCursor(0, 1);
  Timer1.attachInterrupt(pulse);
}

void pulse() {
	if( usBetweenPulses !=0)
	{
		_myMotor.digiWrite2(positiveDirection) // direction
		_myMotor.digiWrite(true);
		if( positiveDirection )
			pulses ++;
		else
			pulses --;	
		_myMotor.digiWrite(false);
	}
	usBetweenPulses = clamp(usBetweenPulses, MinPulseTime, MaxPulseTime);
	// set for next pulse, the timer keeps running 
	Timer1.setPeriod(usBetweenPulses); 
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  _lcd.setCursor(0, 1);
  if( usBetweenPulses != 0)
  {
	if( positiveDirection )
		_lcd.print('+');
	else
		_lcd.print('-');
	  // 16 microsteps, 200 pulses/rev => 3200 pulses/rev
	  float revsPerDay = 3200/(usBetweenPulses * 1e-6f);
	  _lcd.print(revsPerDay);
  } 
  else 
  {
	_lcd.print("Stall");
  }
  _lcd.setCursor(8, 1);
  _lcd.print(' ');
  _lcd.print(pulses);
  
  delay(100);
  handleInput();
}


// ms value of last press (edge trigger)
// 0 means not pressed
int upPressedMillis;
int downPressedMillis;

void handleInput()
{
	bool upPressed = _altButtons.digiRead();
	bool downPressed = _altButtons.digiRead2();
	
	if( (upPressed && downPressed)
		return;
	
	int mils = millis(); // timestamp 'now', all logic should be based on this moment
	if( mils = 0)
		mils = 1; // prevent an overflow to mess up the 'unpressed' value
	
	if( upPressed )
	{
		if(upPressedMillis > 0 && mils - upPressedMillis < 500)
			return; // a single press will be a single increment, holding the button down will quickly increment
		if( upPressedMillis == 0)
			upPressedMillis = mils
		if( positiveDirection)
			IncreasePulseTime();
		else
			DecreasePulseTime();
	} 
	else
		upPressedMillis = 0; //flag as not pressed
	
	if( downPressed )
	{
		if(downPressedMillis > 0 && mils - downPressedMillis < 500)
			return; // a single press will be a single increment, holding the button down will quickly increment
		if( downPressedMillis == 0)
			downPressedMillis = mils
		if( !positiveDirection)
			IncreasePulseTime();
		else
			DecreasePulseTime();
	} 
	else
		downPressedMillis = 0; //flag as not pressed
		
}

void IncreasePulseTime()
{
	cli(); // critical section since we're changing usBetweenPulses and positiveDirection
	if( usBetweenPulses == 0 )
	{
		// stall, set direction
		usBetweenPulses = MaxPulseTime;
		positiveDirection = true;
	} else {		
		unsigned long delta = usBetweenPulses >> 6;		
		// exponentially increase
		usBetweenPulses += delta 
		// if the pulse time gets too long, stall.
		if( usBetweenPulses > MaxPulseTime )
			usBetweenPulses = 0;
	}
	sei();
}


void DecreasePulseTime()
{
	cli(); // critical section since we're changing usBetweenPulses and positiveDirection
	if( usBetweenPulses == 0 )
	{
		// stall, set direction
		usBetweenPulses = MaxPulseTime;
		positiveDirection = false;
		return;
	}
	unsigned long delta = usBetweenPulses >> 6;
	usBetweenPulses -= delta;
	sei();
}

long clamp( long value, long lower, long upper)
{
	if( value > upper )
		value = upper;
	if( value < lower )
		value = lower;
}
