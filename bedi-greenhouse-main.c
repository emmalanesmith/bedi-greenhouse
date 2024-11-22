/*
Plant Bed(i) Greenhouse:
An automated plant incubator for bringing up houseplants, powered by a LEGO Mindstorms EV3 Robot.
By: Sevita Moiseev, Emma Lane-Smith, Kira Costen, Meeji Koo
Last Updated: 11/22/2024
*/

/*
NOTE:
ENTER USER SETTINGS ON LINES 516-521
*/

#include "mindsensors‐motormux.h"

//Fail-safe max times (found empirically)
const float MAX_PUMP_TIME = 19500; //axis time + 1 sec
const float MAX_X_AXIS_TIME = 18500; //measured 16410 runtime
const float MAX_Y_AXIS_TIME = 10500; //measured 8700 runtime
const float MAX_ROTATION_TIME = 20000;

//Rotation constants (found empirically)
const float ROTATION_DISTANCE = 28;
const int ROTATION_SPEED = 20;
const int MAX_ROTATIONS = 2; //change direction after 2 turns

//Wheel radii and conversion factors (found empirically)
const float ROTATION_WHEEL_RADIUS = 2.5;
const float Y_AXIS_WHEEL_RADIUS = 1.9;
const float X_AXIS_WHEEL_RADIUS = 0.6;
const float ROTATION_CONVERSION_FACTOR = 2.0*PI*ROTATION_WHEEL_RADIUS/360.0; 
const float Y_AXIS_CONVERSION_FACTOR = 2.0*PI*Y_AXIS_WHEEL_RADIUS/360.0;
const float X_AXIS_CONVERSION_FACTOR = 2.0*PI*X_AXIS_WHEEL_RADIUS/360.0;

//Water cycle constants (found empirically)
const int PUMP_SPEED = 100;
const float Y_AXIS_LENGTH = 8.5; //full rail 14.0 cm
const float X_AXIS_LENGTH = 5; //full rail 18.0 cm; cut off due to axis design
const float BUFFER_LENGTH = 3.25; //due to change in direction; 3.5 originally
const float X_AXIS_SPEED = 5.0;
const float Y_AXIS_SPEED = 3.0;

//Fail integers (for fail-safe error message)
const int NO_FAILURE = -1;
const int ROTATION_FAILED = 0;
const int PUMP_FAILED = 1;
const int AXIS_FAILED = 2;

//Wait time between messages in milliseconds
const int WAIT_MESSAGE = 2500; 

/*
MOTOR A: x direction on 2D axis (1)
MOTOR B: x direction on 2D axis (2)
MOTOR C: y direction on 2D axis
MOTOR D: peristaltic pump
MULTIPLEXER M1: rotation of greenhouse base
*/

/*
SENSOR 1: multiplexer
SENSOR 3: touch
SENSOR 4: colour sensor
*/
void configureSensors()
{
	SensorType[S1] = sensorI2CCustom;
	wait1Msec(50);
	SensorType[S3] = sensorEV3_Touch;
	wait1Msec(50);
	SensorType[S4] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S4] = modeEV3Color_Color;
	wait1Msec(50);
}

void clearScreen()
{
	displayTextLine(3, " ");
	displayTextLine(4, " ");
	displayTextLine(5, " ");
	displayTextLine(6, " ");
}

bool checkFillLevel()
{
	if (SensorValue[S4] == (int)colorWhite) //no water, ping pong ball at bottom
		return false;
	else
		return true;
}

void displayFillLevel()
{
	if (checkFillLevel())
		displayTextLine(5, "Water available in tank.");
	else
	{
		displayTextLine(4, "Empty water tank.");
		displayTextLine(5, "Please add water.");
	}
}

//Starts pump and returns time of start
float startPump()
{
	float startTime = time1[T1];
	motor[motorD] = PUMP_SPEED;
	return startTime;
}

/*
Resets x-axis to starting position
Returns false if fails
taskFailed updates to AXIS_FAILED (2) or NO_FAILURE (-1)
*/
bool resetWaterCycle(int& taskFailed)
{
	bool executed = true; //assume no failure
	float startTime = time1[T1];
	nMotorEncoder[motorB] = 0; //error when combined in one line
	nMotorEncoder[motorA] = 0;
	
	motor[motorB] = -X_AXIS_SPEED; //x-axis motors
	motor[motorA] = -X_AXIS_SPEED;
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < (X_AXIS_LENGTH+BUFFER_LENGTH)) && (time1[T1] - startTime < MAX_X_AXIS_TIME))
	{}
	motor[motorB] = 0;
	motor[motorA] = 0;
	
	if (time1[T1] - startTime > MAX_X_AXIS_TIME) //exceeded timer
	{
		taskFailed = AXIS_FAILED;
		executed = false;
	}
	else if (SensorValue[S3] == 1) //emergency stop button
	{
		executed = false;
	}
	return executed;
}

/*
Powers the motors to turn 90 degrees (at ROTATION_SPEED)
Switches directions after 180 degrees (MAX_ROTATIONS)
int& numRotations: number of rotations thus far
bool& clockwise: true for CW, false for CCW
Returns false if fails
taskFailed updates to ROTATION_FAILED (0) or NO_FAILURE (-1)
*/
bool rotateGreenhouse(int& numRotations, bool& clockwise, int& taskFailed)
{
	bool executed = true;
	float startTime = time1[T1];
	if (numRotations == MAX_ROTATIONS)
	{
		clockwise = !clockwise; //change direction
		numRotations = 0; //reset counter
	}
	else
		numRotations++;
	
	
	if (clockwise)
		MSMMotor(mmotor_S1_1, -ROTATION_SPEED); //CW
	else
		MSMMotor(mmotor_S1_1, ROTATION_SPEED); //CCW
	
	MSMMotorEncoderReset(mmotor_S1_1);
	while((abs(MSMMotorEncoder(mmotor_S1_1))*ROTATION_CONVERSION_FACTOR < ROTATION_DISTANCE) && (time1[T1] - startTime < MAX_ROTATION_TIME)) //fail-safe
	{}
	MSMotorStop(mmotor_S1_1);
	
	if (time1[T1] - startTime > MAX_ROTATION_TIME) //exceeded timer
	{
		taskFailed = ROTATION_FAILED;
		executed = false;
	}
	return executed;
}

/*
Checks if water is available
Starts the pump and the 2D axis motors
Stops pump
Returns false if fails
taskFailed updates as AXIS_FAILED (2), PUMP_FAILED (1), or NO_FAILURE (-1)
*/
bool activateWaterCycle(int& taskFailed)
{
	bool executed = true;
	while (!checkFillLevel()) //no water
	{
		displayFillLevel(); //prompts user until water is filled
	}
	float startTime = time1[T1]; // fail safe timer
	clearScreen();
	startPump();

	//activate 2D axis (error caused when combined in one line)
	nMotorEncoder[motorA] = 0;
	nMotorEncoder[motorB] = 0;
	nMotorEncoder[motorC] = 0;
	motor[motorB] = X_AXIS_SPEED;
	motor[motorA] = X_AXIS_SPEED;
	motor[motorC] = Y_AXIS_SPEED;
	
	float xStartTime = time1[T1]; //fail safe
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH) && (time1[T1] - xStartTime < MAX_X_AXIS_TIME) && (time1[T1] - startTime < MAX_PUMP_TIME) && (SensorValue[S3] == 0))
	{
		// y-axis iterates multiple times while x-axis makes its first iteration
		float yStartTime = time1[T1];
		while((abs(nMotorEncoder[motorC])*Y_AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - yStartTime < MAX_Y_AXIS_TIME))
		{}
		motor[motorC] *= -1; //change y-axis direction
		nMotorEncoder[motorC] = 0;
		if (time1[T1] - yStartTime > MAX_Y_AXIS_TIME) //exceeded y-axis timer
		{
			taskFailed = AXIS_FAILED;
			executed = false;
		}
	}
	if (SensorValue[S3] == 1) //emergency button pressed
		{
			executed = false;
		}
	motor[motorC] = 0; //stop axis
	motor[motorA] = 0;
	motor[motorB] = 0;
	motor[motorD] = 0; //stop pump
	
	if (time1[T1] - xStartTime > MAX_X_AXIS_TIME) //exceeded x-axis timer
	{
		taskFailed = AXIS_FAILED;
		executed = false;
	}
	else if (time1[T1] - startTime > MAX_PUMP_TIME) //exceeded pump timer
	{
		taskFailed = PUMP_FAILED;
		executed = false;
	}
	
	return executed;
}

/*
Prompts the user to enter the current time and saves to float variables
*/
void setStartTime(float& hour, float& minute, float& period)
{

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
	
	string periodDisplay = "a.m.";
	
	while (timeSet != 2)
	{
		// generate updated time after toggling
		if (minute< 10) displayTextLine(4, "%d:0%d %s", hour, minute, periodDisplay);
		else displayTextLine(4, "%d:%d %s", hour, minute, periodDisplay);

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
	clearScreen();
}

/*
Displays plant's stats (name, number of cycles, current date and time, etc.)
*/
void generateStats(string plantName, float timeWater, float timeRotation, float day, float month, float year, float hour, float minute, float& period, float newHour, float newMinute, bool executed, int taskFailed)
{
	float runTime = time1[T1];

	int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31 ,31 ,30, 31, 30, 31}; // index corresponds to month-1

	//Calculating current time and date
	if (period == 1)
		hour += 12;

	// counting total number of full days, sets correct month and day
	newMinute = runTime/1000/60 + minute; // total minutes
	newHour = (newMinute/60) + hour; // total hours
	if (newMinute > 60)
		newMinute -= ((newMinute/60)*60); // final number of minutes
	day += (newHour/24); // total days
	if (newHour > 24)
		newHour -= ((newHour/24)*24);// final number of hours

	// changing a.m./p.m.
	if (newHour >= 0 && newHour <= 11)
		period = 0;
	else if (newHour >= 12)
		period = 1;

	// convert from 24h to 12h clock
	if (newHour == 0)
		newHour = 12;
	if (newHour > 12)
		newHour -= 12;

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

		if (day <= daysInMonth[month-1] && month <= 12)
			correctDate = true;
	}


	// display stats
	displayTextLine(4, "Plant name: %s", plantName);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Total run time (ms): %d", runTime);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Water interval (ms): %d", timeWater);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Rotation interval (ms): %d", timeRotation);
	wait1Msec(WAIT_MESSAGE);

	// correct display of date
	if (month<10)
		displayTextLine(4, "%d/0%d/%d", month, day, year);
	else
		displayTextLine(4, "%d/%d/%d", month, day, year);
	wait1Msec(WAIT_MESSAGE);

	// correct display of time
	string periodDisplay = " ";
	if (period == 0)
		periodDisplay = "a.m.";
	else 
		periodDisplay = "p.m.";
	if (newMinute < 10)
		displayTextLine(4, "%d:0%d %s", newHour, newMinute, periodDisplay);
	else
		displayTextLine(4, "%d:%d %s", newHour, newMinute, periodDisplay);
	wait1Msec(WAIT_MESSAGE);
	
	if (!executed)
	{
		displayTextLine(4, "ROBOT FAILURE:");
		switch (taskFailed) //display reason
		{
			case -1:
				displayTextLine(5, "UNKNOWN REASON");
				break; //break statement approved by teaching team**
			case 0:
				displayTextLine(5, "ROTATION FAILED");
				break;
			case 1:
				displayTextLine(5, "PUMP FAILED");
				break;
			case 2:
				displayTextLine(5, "AXIS FAILED");
				break;
			default:
				displayTextLine(5, "UNKNOWN REASON");
		} 
		wait1Msec(WAIT_MESSAGE);
	}
	clearScreen();
}

/*
All daily operations (performs water/rotation cycles at the proper intervals, and listening for buttons)
*/
void activateGreenhouse(string& plantName, bool& executed, int& taskFailed, float& waterInterval, float& rotationInterval, float& day, float& month, float& year, float& hour, float& minute, float& period, float& newHour, float& newMinute)
{
	clearTimer(T2); //water cycle interval timer
	clearTimer(T3); //rotation interval timer
	
	// initialize, for the multiplexer connected to S4; must be done here (not global)
	MSMMUXinit();
	wait1Msec(50);
	
	/*
 	buttonUp: stats report
  	buttonDown: shut down
	*/

	int numRotations = 0; //no turns yet
	bool clockwise = true; //first turn clockwise
	bool userShutDown = false; //to exit activateGreenhouse without failing
	
	while(executed && !userShutDown)
	{
		displayTextLine(4, "Press UP for stats");
		displayTextLine(5, "Press DOWN to shut down");

		//listens for button presses, waits for timers
		while (!getButtonPress(buttonUp) && !getButtonPress(buttonDown) && (time1[T2] < waterInterval) && (time1[T3] < rotationInterval) && (SensorValue[S3] == 0))
		{}
	
		if (SensorValue[S3] == 1) //emergency button pressed
			executed = false;

		//GENERATE STATS (up button)
		else if (getButtonPress(buttonUp))
		{
			while(getButtonPress(buttonAny))
			{}
			wait1Msec(50); //buffer
			clearScreen();
			generateStats(plantName, waterInterval, rotationInterval, day, month, year, hour, minute, period, newHour, newMinute, executed, taskFailed);
		}
	
		//SHUT DOWN (down button)
		else if (getButtonPress(buttonDown))
		{
			while(getButtonPress(buttonAny))
			{}
			wait1Msec(50); //buffer
			userShutDown = true;
		}
	
		//WATER CYCLE (time based)
		else if (time1[T2] > waterInterval)
		{
			if (activateWaterCycle(taskFailed))
			{
				executed = resetWaterCycle(taskFailed);
				clearTimer(T2);
			}
			else
				executed = false;
		}
	
		//ROTATION (time based)
		else if (time1[T3] > rotationInterval)
		{
			executed = rotateGreenhouse(numRotations, clockwise, taskFailed);
			clearTimer(T3);
		}
	}
}

void safeShutDown(string plantName, float waterInterval, float rotationInterval, float day, float month, float year, float hour, float minute, float period, float newHour, float newMinute, int taskFailed, bool executed)
{
	motor[motorD] = 0; //stop pump
	MSMotorStop(mmotor_S1_1); //stop rotation
	clearScreen();
	generateStats(plantName, waterInterval, rotationInterval, day, month, year, hour, minute, period, newHour, newMinute, executed, taskFailed);
}

task main()
{
	/*
 	ENTER USER SETTINGS HERE:
  	plantName: desired name of your plant!
   	waterTiming: time in between water cycles (milliseconds)
    	rotationTiming: time in between rotation cycles (milliseconds)
     	day, month, year: today's date (##, ##, ####)
 	*/
	
	string plantName = " ";
	float waterTiming = 0;
	float rotationTiming = 0;
	float day = 0;
	float month = 0;
	float year = 0;
	
	/*
 	END OF USER SETTINGS
  	*/
	
	clearTimer(T1); //main timer
	configureSensors();
	MSMotorStop(mmotor_S1_1); //precaution for multiplexer motor

	bool executed = true; //false as soon as any function fails
	int taskFailed = NO_FAILURE; //indicates which task failed
	
	/*
  	settings[0]: water interval	settings[1]: rotation interval
    	settings[2]: day		settings[3]: month		settings[4]: year
    	settings[5]: start hour		settings[6]: start minute
	settings[7]: am = 1, pm = 0	settings[8]: current hour	settings[9]: current minute
    	*/
	float settings[10] = {waterTiming, rotationTiming, day, month, year, 0, 0, 0, 0, 0};

	setStartTime(settings[5], settings[6], settings[7]); //user inputs current time
	generateStats(plantName, settings[0], settings[1], settings[2], settings[3], settings[4], settings[5], settings[6], settings[7], settings[8], settings[9], executed, taskFailed);

	/*
 	First water-cycle (start-up)
 	*/
	if (activateWaterCycle(taskFailed))
		executed = resetWaterCycle(taskFailed);
	else
		executed = false;

	/*
 	Main program operations
	*/
	if (executed)
		activateGreenhouse(plantName, executed, taskFailed, settings[0], settings[1], settings[2], settings[3], settings[4], settings[5], settings[6], settings[7], settings[8], settings[9]);

	safeShutDown(plantName, settings[0], settings[1], settings[2], settings[3], settings[4], settings[5], settings[6], settings[7], settings[8], settings[9], taskFailed, executed);
	
}
