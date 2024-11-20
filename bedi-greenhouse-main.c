/*
Plant Bed(i) Greenhouse:
An automated plant incubator for bringing up houseplants, powered by a LEGO Mindstorms EV3 Robot.
By: Sevita Moiseev, Emma Lane-Smith, Kira Costen, Meeji Koo
Last Updated: 11/18/2024
*/

#include "PC_FileIO.c"
#include "mindsensors‐motormux.h"

//Fail-safe max times (found empirically)
const float MAX_PUMP_TIME = 19500; //axis time + 1 sec
const float MAX_X_AXIS_TIME = 18500; //16410 runtime
const float MAX_Y_AXIS_TIME = 10500; //8700 runtime
const float MAX_ROTATION_TIME = 20000;

//Rotation constants (found empirically)
const float ROTATION_DISTANCE = 31.5;
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
const float BUFFER_LENGTH = 3.5; //due to change in direction
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
SENSOR 2: colour sensor
*/
void configureSensors()
{
	SensorType[S1] = sensorI2CCustom;
	wait1Msec(50);
	SensorType[S2] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S2] = modeEV3Color_Color;
	wait1Msec(50);
}

bool checkFillLevel()
{
	if (SensorValue[S2] == (int)colorWhite)
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
		displayTextLine(5, "Empty water tank.");
		displayTextLine(6, "Please add water.");
	}
}

//Returns time when pump started
float startPump()
{
	float startTime = time1[T1];
	motor[motorD] = PUMP_SPEED;
	return startTime;
}

/*
Resets 2D axis to starting position
First x-axis, then y-axis
Returns false if fails
taskFailed refers to which task failed if any
*/
bool resetWaterCycle(int& taskFailed)
{
	bool executed = true;
	float startTime = time1[T1];
	nMotorEncoder[motorB] = 0; //error when combined in one line
	nMotorEncoder[motorA] = 0;
	
	motor[motorB] = -X_AXIS_SPEED; //x-axis motors
	motor[motorA] = -X_AXIS_SPEED;
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < (X_AXIS_LENGTH+BUFFER_LENGTH)) && (time1[T1] - startTime < MAX_X_AXIS_TIME))
	{}
	motor[motorB] = 0;
	motor[motorA] = 0;
	if (time1[T1] - startTime > MAX_X_AXIS_TIME)
	{
		taskFailed = AXIS_FAILED;
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
taskFailed refers to which task failed if any
*/
bool rotateGreenhouse(int& numRotations, bool& clockwise, int& taskFailed)
{
	bool executed = true;
	//bool buffer = false; //need buffer for direction change
	float startTime = time1[T1];
	if (numRotations == MAX_ROTATIONS)
	{
		clockwise = !clockwise; //change direction
		numRotations = 0;
		//buffer = true;
	}
	else
		numRotations++;
	
	
	if (clockwise)
		MSMMotor(mmotor_S1_1, -ROTATION_SPEED);
	else
		MSMMotor(mmotor_S1_1, ROTATION_SPEED);

	//mechanical tests
	/*if (buffer)
		wait1Msec(1000);
	wait1Msec(10000); //test
	while(abs(MSMMotorEncoder(mmotor_S1_1))*ROTATION_CONVERSION_FACTOR < ROTATION_DISTANCE) //fail-safe TEST
	{}
	*/
	MSMMotorEncoderReset(mmotor_S1_1);
	while((abs(MSMMotorEncoder(mmotor_S1_1))*ROTATION_CONVERSION_FACTOR < ROTATION_DISTANCE) && (time1[T1] - startTime < MAX_ROTATION_TIME)) //fail-safe
	{}
	MSMotorStop(mmotor_S1_1);
	
	if (time1[T1] - startTime > MAX_ROTATION_TIME)
	{
		taskFailed = ROTATION_FAILED;
		executed = false;
	}
	return executed;
}

/*
Checks if water is available
Starts the pump and the 2D axis motors
Stops pump and returns motors to origin
Returns false if fails
taskFailed refers to which task failed if any
*/
bool activateWaterCycle(int& taskFailed)
{
	bool executed = true;
	while (!checkFillLevel()) //no water
	{
		displayFillLevel();
	}
	float startTime = time1[T1]; // fail safe timer
	startPump();

	//activate 2D axis
	nMotorEncoder[motorA] = 0; //error when combined in one line
	nMotorEncoder[motorB] = 0;
	nMotorEncoder[motorC] = 0;
	motor[motorB] = X_AXIS_SPEED;
	motor[motorA] = X_AXIS_SPEED;
	motor[motorC] = Y_AXIS_SPEED;
	
	float xStartTime = time1[T1]; //fail safe
	while((abs(nMotorEncoder[motorA])*X_AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH) && (time1[T1] - xStartTime < MAX_X_AXIS_TIME) && (time1[T1] - startTime < MAX_PUMP_TIME))
	{
		// y-axis iterates multiple times while x-axis makes its first iteration
		float yStartTime = time1[T1];
		while((abs(nMotorEncoder[motorC])*Y_AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - yStartTime < MAX_Y_AXIS_TIME))
		{}
		motor[motorC] *= -1;
		nMotorEncoder[motorC] = 0;
		if (time1[T1] - yStartTime > MAX_Y_AXIS_TIME)
		{
			taskFailed = AXIS_FAILED;
			executed = false;
		}
	}
	motor[motorC] = 0; //stop axis
	motor[motorA] = 0;
	motor[motorB] = 0;
	motor[motorD] = 0; //stop pump
	
	if (time1[T1] - xStartTime > MAX_X_AXIS_TIME)
	{
		taskFailed = AXIS_FAILED;
		executed = false;
	}
	else if (time1[T1] - startTime > MAX_PUMP_TIME)
	{
		taskFailed = PUMP_FAILED;
		executed = false;
	}
	
	return executed;
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
		if (minute< 10) displayTextLine(4, "%.0f:0%.0f %s", hour, minute, period);
		else displayTextLine(4, "%.0f:%.0f %s", hour, minute, period);

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
		if (minute< 10) displayTextLine(4, "%.0f:0%.0f %s", hour, minute, periodDisplay);
		else displayTextLine(4, "%.0f:%.0f %s", hour, minute, periodDisplay);

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
	displayTextLine(4, "Total run time in milliseconds: %.0f", runTime);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Number of water cycles: %.0f", numWaterCycles);
	wait1Msec(WAIT_MESSAGE);
	displayTextLine(4, "Number of rotations: %.0f", numRotations);
	wait1Msec(WAIT_MESSAGE);

	// correct display of date
	if (month<10) displayTextLine(4, "%.0f/0%.0f/%.0f", month, day, year);
	else displayTextLine(4, "%.0f/%.0f/%.0f", month, day, year);
	wait1Msec(WAIT_MESSAGE);

	// correct display of time
	string periodDisplay = " ";
	if (period == 0) periodDisplay = "a.m.";
	else periodDisplay = "p.m.";
	if (newMinute < 10) displayTextLine(4, "%.0f:0%.0f %s", newHour, newMinute, periodDisplay);
	else displayTextLine(4, "%.0f:%.0f %s", newHour, periodDisplay);
	wait1Msec(WAIT_MESSAGE);

}

/*
Generates end file based on stats from generateStats(string, float*)
*/
void generateEndFile(TFileHandle& fout, string plantName, float* settings)
{
	//float* settingsTemp = settings;
	generateStats(plantName, settings);
  	
	writeTextPC(fout, "PLANT NAME:");
	writeEndlPC(fout);
	writeTextPC(fout, plantName);
 	writeEndlPC(fout);
	writeEndlPC(fout);

   	writeTextPC(fout, "ROTATION CYCLE INTERVAL (milliseconds):");
	writeEndlPC(fout);
    	writeFloatPC(fout, *settings); //MIGHT BUG DUE TO GENERATE STATS
  	writeEndlPC(fout);  
	writeEndlPC(fout);
	
    	writeTextPC(fout, "ROTATION CYCLE INTERVAL (milliseconds):");
	writeEndlPC(fout);
    	writeFloatPC(fout, *(settings++));
  	writeEndlPC(fout);  
	writeEndlPC(fout);
	
   	writeTextPC(fout, "DAY (##)");
	writeEndlPC(fout);
	if (*(settings++) < 10.0) //should be settings[2]
	{
		writeTextPC(fout, "0");
	   	writeFloatPC(fout, "%.0f", *settings); //MIGHT BUG, also could be writeTextPC
	}
	else 
	{
		writeFloatPC(fout, "%.0f", *settings);
	}
  	writeEndlPC(fout);  
	writeEndlPC(fout);
	
   	writeTextPC(fout, "MONTH (##)");
	writeEndlPC(fout);
    	writeFloatPC(fout, "%.0f", *(settings++));
  	writeEndlPC(fout);  
	writeEndlPC(fout);
	
    	writeTextPC(fout, "YEAR (####)");
	writeEndlPC(fout);
    	writeFloatPC(fout, "%.0f", *(settings++));
	writeEndlPC(fout);  
	writeEndlPC(fout);
	
	settings += 3;
	writeTextPC(fout, "END TIME:");
	writeEndlPC(fout);
	writeFloatPC(fout,  "%.0f", *settings); //MIGHT BUG
	writeTextPC(fout, ":");
	writeFloatPC(fout, "%.0f", *(settings++));

	settings -= 2;
	if (*settings == 0)
	{
		writeTextPC(fout, "AM");
	}
	else 
	{
		writeTextPC(fout, "PM");
	}		

}

/*
Generates fail file based on generateEndFile(TFileHandle&, string, float*)
Adds message about why the robot failed
*/
void generateFailFile(TFileHandle& fout, string plantName, float* settings, int taskFailed)
{
	generateEndFile(fout, plantName, settings);
	writeEndlPC(fout);

	switch(taskFailed)
	{
		case 0:
			writeTextPC(fout, "ROTATION FAILED");
			break;
		case 1:
			writeTextPC(fout, "WATER PUMP FAILED");
			break;
		case 2:
			writeTextPC(fout, "2D AXIS FAILED");
			break;
		default:
			writeTextPC(fout, "UNKNOWN FAILURE");
	}

	/*
	if (taskFailed == ROTATION_FAILED)
	{
		writeTextPC(fout, "ROTATION FAILED");
	}
	else if (taskFailed == PUMP_FAILED)
	{
		writeTextPC(fout, "WATER PUMP FAILED");
	}
	else if (taskFailed == AXIS_FAILED)
	{
		writeTextPC(fout, "2D AXIS FAILED");
	}
	else
	{
		writeTextPC(fout, "UNKNOWN FAILURE");
	}*/
}

//emma
void activateGreenhouse(float* settings, string plantName, bool& executed, int& taskFailed)
{
	// initialize, for the multiplexer connected to S2; must be done here (not global)
	MSMMUXinit();
	wait1Msec(50);
	
	/*
 	buttonUp: stats report
  	buttonDown: shut down
	*/

	float* tempSettings = settings; //could break.
	float* water = settings;
	float* rotation = settings++;

	int numRotations = 0; //no turns yet
	bool clockwise = true; //first turn clockwise
	bool userShutDown = false; //to exit activateGreenhouse without failing

	while(executed && !userShutDown)
	{
		while (!getButtonPress(buttonUp) && !getButtonPress(buttonDown) && !getButtonPress(buttonRight) && (time1[T2] < *water) && (time1[T3] < *rotation))
		{}
	
		//GEN STATS
		if (getButtonPress(buttonUp))
		{
			while(getButtonPress(buttonAny))
			{}
			wait1Msec(50); //buffer
			generateStats(plantName, tempSettings);
		}
	
		//SHUT DOWN
		else if (getButtonPress(buttonDown)) //shut down
		{
			while(getButtonPress(buttonAny))
			{}
			wait1Msec(50); //buffer
			userShutDown = true;
		}
	
		//SET TIME
		else if (getButtonPress(buttonRight))
		{
			while(getButtonPress(buttonAny))
			{}
			wait1Msec(50);
			setStartTime(tempSettings);
		}
	
		//WATER CYCLE
		else if (time1[T2] < *water)
		{
			clearTimer(T2);
			if (activateWaterCycle(taskFailed))
				executed = resetWaterCycle(taskFailed);
			else
				executed = false;
		}
	
		//ROTATION
		else if (time1[T3] < *rotation)
		{
			clearTimer(T3);
			executed = rotateGreenhouse(numRotations, clockwise, taskFailed);
		}
	}
}

void safeShutDown(int taskFailed)
{
	motor[motorD] = 0; //stop pump
	MSMotorStop(mmotor_S1_1); //stop rotation
	resetWaterCycle(taskFailed);
}

task main()
{
	clearTimer(T1); //main timer
	clearTimer(T2); //water cycle interval timer
	clearTimer(T3); //rotation interval timer
	configureSensors();
	
	TFileHandle config;
	bool configOpen = openReadPC(config, "config.txt");
	TFileHandle fout;
	bool foutOpen = openWritePC(fout, "output.txt"); //syntax

	if (configOpen && foutOpen)
	{
		/*
  		START-UP PROCEDURE
    		*/

		bool executed = true; //false as soon as any function fails
		int taskFailed = NO_FAILURE; //indicates which task failed
		string plantName = " ";
		
		/*
  		settings[0]: water interval	settings[1]: rotation interval
      		settings[2]: day		settings[3]: month		settings[4]: year
    		settings[5]: start hour		settings[6]: start minute
		settings[7]: am = 1, pm = 0	settings[8]: current hour	settings[9]: current minute
    		*/
		float settings[10] = {0.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0};
		
		readUserSettings(config, plantName, settings);
		generateStats(plantName, settings); //again idk if pointers will allow this

		if (activateWaterCycle(taskFailed)) //first water-cycle
			executed = resetWaterCycle(taskFailed);
		else
			executed = false;
		
		/*
		MAIN OPERATION
  		*/
		if (executed)
			activateGreenhouse(settings, plantName, executed, taskFailed);

		/*
		SHUT-DOWN PROCEDURE
		*/
		safeShutDown(taskFailed);
		if (executed)
			generateEndFile(fout, plantName, settings);
		else
			generateFailFile(fout, plantName, settings, taskFailed);
		closeFilePC(fout);
		closeFilePC(config);
	}
	else
	{
		displayTextLine(5, "ERROR opening files.");
	}
}
