// Demo sketch for a speed controlled stepper with an LCD connected to I2C port via MCP23008 I/O expander
// 2012 KvL http://opensource.org/licenses/mit-license.php

#include <PortsLCD.h>
#include "TimerOne.h"

#define MinPulseTime (200L)
#define MaxPulseTime (200*1000L)

PortI2C _myI2C (1, PortI2C::KHZ400);
LiquidCrystalI2C _lcd (_myI2C);
Port _myMotor (2);
Port _altButtons (3);

volatile long pulses = 0; //updated in ISR
long usBetweenPulses = 0;
bool positiveDirection = true;

void setup() 
{
	Serial.begin(57600);
	Serial.write("Hello.\n");
	_myMotor.mode(OUTPUT);
	_myMotor.mode2(OUTPUT);
	_altButtons.mode(INPUT);
	_altButtons.mode2(INPUT);
	Serial.write("Motor init.\n");
	// set up the LCD's number of rows and columns: 
	_lcd.begin(16, 2);
	// Print a message to the LCD.
	_lcd.print("Single axis");
	_lcd.setCursor(0, 1);
	// turn the backlight on
	_lcd.backlight();
	Serial.write("LCD init.\n");
	Timer1.initialize(MaxPulseTime);
	Timer1.attachInterrupt(pulse, MaxPulseTime);
	Timer1.start();
 }

void pulse() 
{
    //if( usBetweenPulses !=0)
	{
		_myMotor.digiWrite2(positiveDirection); // direction
		_myMotor.digiWrite(!_myMotor.digiRead()); // toggle step bit
		if( positiveDirection )
			pulses ++;
		else
			pulses --;	 
	} 
    Timer1.resume();
}

void loop() 
{
	_lcd.setCursor(0, 1);
	if( usBetweenPulses != 0)
	{
		if( positiveDirection )
			_lcd.print('+');
		else
			_lcd.print('-');
		// 16 microsteps, 200 pulses/rev => 3200 pulses/rev, 
		// toggle at interrupt: another factor 2
		float revsPerDay = 3200/(usBetweenPulses * 1e-6f * 24*60*60 * 2);
		_lcd.print(revsPerDay);
	    _lcd.print(' '); // empty for the case the prev print was a char less
	} 
	else 
	{
		_lcd.print("Stall    ");
	}
	_lcd.setCursor(8, 1);
	_lcd.print(pulses);
	_lcd.print("P ");

	delay(100);
	handleInput();
  
	long period = clamp(usBetweenPulses, MinPulseTime, MaxPulseTime);
	// set for next pulse, the timer keeps running 
        Timer1.setPeriod(period);
        sei();
}


// ms value of last press (edge trigger)
// 0 means not pressed
int upPressedMillis = 0;
int downPressedMillis = 0;

void handleInput()
{
	bool upPressed = _altButtons.digiRead();
	bool downPressed = _altButtons.digiRead2();
	
	if(upPressed && downPressed)
	{
		upPressedMillis = 0;
		downPressedMillis = 0;
		return;
	}
	int mils = millis(); // timestamp 'now', all logic should be based on this moment
	if( mils = 0)
		mils = 1; // prevent an overflow to mess up the 'unpressed' value
	
	if( upPressed )
	{
		if(upPressedMillis > 0 && mils - upPressedMillis < 500)
			return; // a single press will be a single increment, holding the button down will quickly increment
		
		Serial.write('+');
		if( upPressedMillis == 0)
			upPressedMillis = mils;
		if( positiveDirection)
			DecreasePulseTime(); // go faster up
		else
			IncreasePulseTime();
	} 
	else
		upPressedMillis = 0; //flag as not pressed
	
	if( downPressed )
	{
		if(downPressedMillis > 0 && mils - downPressedMillis < 500)
			return; // a single press will be a single increment, holding the button down will quickly increment
		Serial.write('-');
		if( downPressedMillis == 0)
			downPressedMillis = mils;
		if( !positiveDirection)
			DecreasePulseTime(); // go faster down
		else
			IncreasePulseTime();
	} 
	else
		downPressedMillis = 0; //flag as not pressed
		
}

void IncreasePulseTime()
{ // Decrease speed
	cli(); // critical section since we're changing usBetweenPulses and positiveDirection
	if( usBetweenPulses == 0 )
	{
		// stall, set direction
		usBetweenPulses = MaxPulseTime;
		positiveDirection = false;
	} else {		
		unsigned long delta = usBetweenPulses >> 5;		
		// exponentially increase
		usBetweenPulses += delta; 
		// if the pulse time gets too long, stall.
		if( usBetweenPulses > MaxPulseTime )
			usBetweenPulses = 0;
	}
	sei();
}


void DecreasePulseTime()
{ // Increase speed
	cli(); // critical section since we're changing usBetweenPulses and positiveDirection
	if( usBetweenPulses == 0 )
	{
		// stall, set direction
		usBetweenPulses = MaxPulseTime;
		positiveDirection = true;
	} else {
		unsigned long delta = usBetweenPulses >> 5;
		usBetweenPulses -= delta;
		if( usBetweenPulses < MinPulseTime)
			usBetweenPulses = MinPulseTime;
	}
	sei();
}

long clamp( long value, long lower, long upper)
{
	if( value > upper )
		value = upper;
	if( value < lower )
		value = lower;
	return value;
}
