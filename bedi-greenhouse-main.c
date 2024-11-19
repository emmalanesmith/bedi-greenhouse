/*
Plant Bed(i) Greenhouse:
An automated plant incubator for bringing up houseplants, powered by a LEGO Mindstorms EV3 Robot.
By: Sevita Moiseev, Emma Lane-Smith, Kira Costen, Meeji Koo
Last Updated: 11/18/2024
*/

#include "PC_FileIO.c"
#include "mindsensors‐motormux.h"

//Fail-safe max times (found empirically)
const float MAX_PUMP_TIME = 0.0; //set empirically*******************
const float MAX_X_AXIS_TIME = 7500;
const float MAX_Y_AXIS_TIME = 9100;
const float MAX_ROTATION_TIME = 0.0; //set empirically**********************

//Rotation constants (found empirically)
const float ROTATION_DISTANCE = 24.1;
const int ROTATION_SPEED = 25; //set empirically******************************
const int MAX_ROTATIONS = 2; //change direction after 2 turns

//Wheel radii and conversion factors (found empirically)
const float ROTATION_WHEEL_RADIUS = 5.7;
const float Y_AXIS_WHEEL_RADIUS = 1.9;
const float X_AXIS_WHEEL_RADIUS = 0.6;
const float ROTATION_CONVERSION_FACTOR = 2.0*PI*ROTATION_WHEEL_RADIUS/360.0; 
const float Y_AXIS_CONVERSION_FACTOR = 2.0*PI*Y_AXIS_WHEEL_RADIUS/360.0;
const float X_AXIS_CONVERSION_FACTOR = 2.0*PI*X_AXIS_WHEEL_RADIUS/360.0;

//Water cycle constants (found empirically)
const int PUMP_SPEED = 25; //set empirically**************
const float Y_AXIS_LENGTH = 12.0; //actual = 14.0 cm 
const float X_AXIS_LENGTH = 16.0; //actual = 18.0 cm 
const float X_AXIS_SPEED = 10.0;
const float Y_AXIS_SPEED = 3.0;

//Fail integers (for fail-safe error message)
const int ROTATION_FAILED = 0;
const int PUMP_FAILED = 1;
const int AXIS_FAILED = 2;

//Wait time between messages in milliseconds
const int WAIT_MESSAGE = 2500; 

/*
MOTOR A: x direction on 2D axis
MOTOR B: x direction on 2D axis
MOTOR C: y direction on 2D axis
MOTOR D: rotation of greenhouse base
MULTIPLEXER M1: peristaltic pump
*/

/*
SENSOR 1: multiplexer
SENSOR 2: colour sensor
*/
void configureSensors()
{
	// initialize, for the multiplexer connected to S2
	SensorType[S1] = sensorI2CCustom;
	MSMMUXinit();
	wait1Msec(50);
	SensorType[S2] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S2] = modeEV3Color_Color;
	wait1Msec(50);
}

bool checkFillLevel()
{
	if (SensorMode[S2] == (int)colorWhite)
		return false;
	else
		return true;
}

void displayFillLevel()
{
	if (checkFillLevel())
		displayTextLine(5, "Water available in tank.");
	else
		displayTextLine(5, "Empty water tank. Please add water.");
}

//Returns time when pump started
float startPump()
{
	float startTime = time1[T1];
	MSMMotor(mmotor_S1_1, PUMP_SPEED);
	return startTime;
}

/*
Resets 2D axis to starting position
First x-axis, then y-axis
Returns false if fails
*/
bool resetWaterCycle()
{
	bool executed = true;
	float startTime = time1[T1];
	nMotorEncoder[motorC] = nMotorEncoder[motorB] = nMotorEncoder[motorA] = 0;
	
	motor[motorB] = motor[motorA] = -X_AXIS_SPEED; //x-axis
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH) && (time1[T1] - startTime < MAX_X_AXIS_TIME))
	{}
	motor[motorB] = motor[motorA] = 0;
	if (time1[T1] - startTime > MAX_X_AXIS_TIME)
		executed = false;

	if (executed)
	{
		startTime = time1[T1];
		motor[motorC] = -Y_AXIS_SPEED; //y-axis
		while((abs(nMotorEncoder[motorC])*Y_AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - startTime < MAX_Y_AXIS_TIME))
		{}
		motor[motorC] = 0;
		if (time1[T1] - startTime > MAX_Y_AXIS_TIME)
			executed = false;
	}
	return executed;
}

/*
Reads in user array and saves settings
settings[0]: water cycle interval, settings[1]: rotation cycle interval
settings[2]: day, settings[3]: month, settings[4]: year
*/
void readUserSettings(TFileHandle& config, string& plantName, float* settings)
{
	string header = " "; //ignore headers on config file
	readTextPC(config, header);
	readTextPC(config, plantName);
	
	readTextPC(config, header);
	readFloatPC(config, *settings);
	readTextPC(config, header);
	readFloatPC(config, *(settings++));
	
	readTextPC(config, header);
	readFloatPC(config, *(settings++));
	readTextPC(config, header);
	readFloatPC(config, *(settings++));
	readTextPC(config, header);
	readFloatPC(config, *(settings++));
}

/*
Powers the motors to turn 90 degrees (at ROTATION_SPEED)
Switches directions after 180 degrees (MAX_ROTATIONS)
int& numRotations: number of rotations thus far
bool& clockwise: true for CW, false for CCW
Returns false if fails
*/
bool rotateGreenhouse(int& numRotations, bool& clockwise)
{
	bool executed = true;
	float startTime = time1[T1];
	if (numRotations == MAX_ROTATIONS)
	{
		clockwise = !clockwise; //change direction
		numRotations = 0;
	}

	nMotorEncoder[motorD] = 0;
	if (clockwise)
		motor[motorD] = ROTATION_SPEED;
	else
		motor[motorD] = -ROTATION_SPEED;

	while((abs(nMotorEncoder[motorD])*ROTATION_CONVERSION_FACTOR < ROTATION_DISTANCE) && (time1[T1] - startTime < MAX_ROTATION_TIME)) //fail-safe
	{}
	motor[motorD] = 0;
	
	if (time1[T1] - startTime > MAX_ROTATION_TIME)
		executed = false;
	return executed;
}

/*
Checks if water is available
Starts the pump and the 2D axis motors
Stops pump and returns motors to origin
Returns false if fails
*/
bool activateWaterCycle()
{
	bool executed = true;
	while (!checkFillLevel()) //no water
	{
		displayFillLevel();
	}
	float startTime = time1[T1]; // fail safe timer
	startPump();

	//activate 2D axis
	nMotorEncoder[motorC] = nMotorEncoder[motorB] = nMotorEncoder[motorA] = 0;
	motor[motorB] = motor[motorA] = X_AXIS_SPEED;
	motor[motorC] = Y_AXIS_SPEED;
	float xStartTime = time1[T1]; //fail safe
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH) && (time1[T1] - xStartTime < MAX_X_AXIS_TIME) && (time1[T1] - startTime < MAX_PUMP_TIME))
	{
		float yStartTime = time1[T1];
		while((abs(nMotorEncoder[motorC])*Y_AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - yStartTime < MAX_Y_AXIS_TIME))
		{}
		motor[motorC] *= -1;
		nmotorEncoder[motorC] = 0;
		if (time1[T1] - startTime > MAX_Y_AXIS_TIME)
			executed = false;
	}
	motor[motorC] = motor[motorA] = motor[motorB] = 0; //stop axis
	MSMotorStop(mmotor_S1_1); //stop pump
	if ((time1[T1] - startTime > MAX_PUMP_TIME) || (time1[T1] - xStartTime > MAX_X_AXIS_TIME))
		executed = false;
	return executed;
	//UPDATE: resetWaterCycle will be kept seperate**
}

/*
Prompts the user to enter the current time and saves to float array
*/
void setStartTime(float* settings)
{
	float* hourP = (6 + settings);
	float hour = *hourP;
	float minute = *(++hourP);
	float period = *(++hourP);
	
	// prompt user
	displayTextLine(3, "Please enter the current time:");
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(3, "Use up/down arrows change #s");
	displayTextLine(4, "Use enter to go next");
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(3, "Please enter the current time:");

	// generate time as user changes it
	int timeSet = 0;
	/*
		setTime = 0; Change hours
		setTime = 1; Change minutes
		setTime = 2; Change period (a.m./p.m.)
	*/

	while (timeSet != 2)
	{
		// generate updated time after toggling
		if (minute< 10) displayTextLine(4, "%d:0%d %s", hour, minute, period);
		else displayTextLine(4, "%d:%d %s", hour, minute, period);

		// toggle settings
		while(!getButtonPress(buttonAny))
		{}
		if (getButtonPress(buttonDown))
		{
			if (timeSet == 0 && hour>1) hour--;
			else if (timeSet == 1 && minute>0) minute--;
		}
		else if (getButtonPress(buttonUp))
		{
			if (timeSet == 0 && hour < 12) hour++;
			else if (timeSet == 1 && minute < 59) minute++;
		}
		else if (getButtonPress(buttonEnter))timeSet++;
		wait1Msec(500);
	}

	while (timeSet == 2)
	{
		while(!getButtonPress(buttonAny))
		{}
		if (getButtonPress(buttonUp) || getButtonPress(buttonDown))
		{
			if (period == 0) period = 1;
			else period = 0;
		}
		
		string periodDisplay = " ";
		if (period == 0) periodDisplay = "a.m.";
		else periodDisplay = "p.m.";
		// generate updated time after toggling
		if (minute< 10) displayTextLine(4, "%d:0%d %s", hour, minute, periodDisplay);
		else displayTextLine(4, "%d:%d %s", hour, minute, periodDisplay);

		if (getButtonPress(buttonEnter)) timeSet = 3;

		wait1Msec(500);
	}
}

/*
Displays plant's stats (name, number of cycles, current date and time, etc.)
*/
void generateStats(string plantName, float* settings)
{
	float timeWater = *settings;
	float timeRotation = *(++settings);
	float day = *(++settings);
	float month = *(++settings);
	float year = *(++settings);
	float hour = *(++settings);
	float minute = *(++settings);
	float period = *(++settings);
	float newHour = *(++settings);
	float newMinute = *(++settings);
	float runTime = time1[T1];

	int daysInMonth[12] = {31, 28,31, 30, 31, 30, 31 ,31 ,30, 31, 30, 31}; // index corresponds to month-1

	// calculations for number of water cycle and rotations
	int numWaterCycles = runTime / timeWater;
	int numRotations = runTime / timeRotation;

	//Calculating current time and date
	if (period == 1) hour += 12;

	// counting total number of full days, sets correct month and day
	newMinute = runTime/1000/60 + minute; // total minutes
	newHour = (newMinute/60) + hour; // total hours
	newMinute -= ((newMinute/60)*60); // final number of minutes
	day += (newHour/24); // total days
	wait1Msec(10000);
	newHour -= ((newHour/24)*24);// final number of hours

	wait1Msec(10000);

	// changing a.m./p.m.
	if (newHour >= 0 && newHour <= 11)
	{
		period = 0;
	}
	else if (newHour >= 12)
	{
		period = 1;
	}

	// convert from 24h to 12h clock
	if (newHour == 0) newHour = 12;
	if (newHour > 12) newHour -= 12;

	else period = 0;
	bool correctDate = false;
	while (!correctDate)
	{
		if (day > daysInMonth[month-1])
		{
			day -= daysInMonth[month-1];
			month++;
		}
		if (month > 12)
		{
			year++;
			month = 1;
		}

		if (day <= daysInMonth[month-1] && month <= 12) correctDate = true;
	}


	// display stats
	displayTextLine(4, "Plant name: %s", plantName);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Total run time in milliseconds: %d", runTime);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Number of water cycles: %d", numWaterCycles);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Number of rotations: %d", numRotations);
	wait1Msec(WAIT_MESSAGE);

	// correct display of date
	if (month<10) displayTextLine(4, "%d/0%d/%d", month, day, year);
	else displayTextLine(4, "%d/%d/%d", month, day, year);
	wait1Msec(WAIT_MESSAGE);

	// correct display of time
	string periodDisplay = " ";
	if (period == 0) periodDisplay = "a.m.";
	else periodDisplay = "p.m.";
	if (newMinute < 10) displayTextLine(4, "%d:0%d %s", newHour, newMinute, periodDisplay);
	else displayTextLine(4, "%d:%d %s", newHour, periodDisplay);
	wait1Msec(WAIT_MESSAGE);

}

//kira
void generateEndFile(TFileHandle& fout, string plantName, float* settings, int* date)
{
	/*
 	writeEndlPC(fout);
  	string s, float f, int i, etc.
  	writeTextPC(fout, s);
   	writeFloatPC(fout, f);
    	writeFloatPC(fout, "%.2f", f); //this is how they do it on the doc but i feel like it's writeTextPC not writeFloatPC
     	writeIntPC(fout, i);
	*/

	//don't include time
}

//kira
void generateFailFile(TFileHandle& fout, string plantName, float* settings, int* date, int taskFailed)
{
	/* ROTATION_FAILED = 0 PUMP_FAILED = 1 AXIS_FAILED = 2*/
}

//emma
void activateGreenhouse(float* settings, string plantName, int* date)
{}

void safeShutDown()
{
	MSMotorStop(mmotor_S1_1); //stop pump
	motor[motorD] = 0; //stop rotation
	resetWaterCycle();
	//generateFailFile();
}

task main()
{
	clearTimer(T1);
	configureSensors();
	
	TFileHandle config;
	bool configOpen = openReadPC(config, "config.txt");
	TFileHandle fout;
	bool foutOpen = openWritePC(fout, "output.txt"); //syntax

	if (configOpen && foutOpen)
	{
		string plantName = " ";
		float settings[10] = {0.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0}; //water cycle interval, rotation cycle interval, day, month, year, start hour, start minute, am/pm (0/1), current hour, current minute
		readUserSettings(config, plantName, settings);

		/*
		TESTING
		*/
		while (!checkFillLevel()) //no water
		{
			displayFillLevel();
		}

		closeFilePC(fout);
		closeFilePC(config);
	}
	else
	{
		displayTextLine(5, "ERROR opening files.");
	}
}
