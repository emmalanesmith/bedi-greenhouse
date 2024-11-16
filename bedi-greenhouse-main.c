/*
Plant Bed(i) Greenhouse:
An automated plant incubator for bringing up houseplants, powered by a LEGO Mindstorms EV3 Robot.
By: Sevita Moiseev, Emma Lane-Smith, Kira Costen, Meeji Koo
Last Updated: 11/14/2024
*/

#include "PC_FileIO.c"

//Fail-safe max times
const float MAX_PUMP_TIME = 0.0; //set empirically
const float MAX_AXIS_TIME = 0.0;
const float MAX_ROTATION_TIME = 0.0;

//Rotation
const float ROTATION_DISTANCE = 0.0; //set empirically (for 90 degrees rotation)
const int ROTATION_SPEED = 25; //set empirically
const int MAX_ROTATIONS = 2; //change direction after 2 turns
const float ROTATION_CONVERSION_FACTOR = 0.0; //set empirically

//Water cycle
const int PUMP_SPEED = 25; //set empirically
const float AXIS_CONVERSION_FACTOR = 0.0; //set empirically, might need two?
const float Y_AXIS_LENGTH = 0.0; //^
const float X_AXIS_LENGTH = 0.0; //^
const float AXIS_SPEED = 25; //^

/*
MOTOR A: peristaltic pump
MOTOR B: rotation of greenhouse base
MOTOR C: x direction, 2D axis
MOTOR D: y direction, 2D axis
*/

void configureSensors()
{
	SensorType[S1] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S1] = modeEV3Color_Color;
	wait1Msec(50);
}

bool checkFillLevel()
{
	if (SensorMode[S1] == (int)colorWhite)
		return false;
	else if (!SensorMode[S1] == (int)colorWhite)
		return true;
}

void displayFillLevel()
{
	if (checkFillLevel())
		displayTextLine(5, "Water available in tank.");
	else
		displayTextLine(5, "Empty water tank. Please add water.");
}

//Returns false if fails
float startPump()
{
	float startTime = time1[T1];
	motor[motorA] = PUMP_SPEED;
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
	nMotorEncoder[motorC] = nMotorEncoder[motorD] = 0;
	
	motor[motorC] = -AXIS_SPEED;
	while((abs(nMotorEncoder[motorC])*AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH) && (time1[T1] - startTime < MAX_AXIS_TIME))
	{}
	motor[motorC] = 0;

	motor[motorD] = -AXIS_SPEED;
	while((abs(nMotorEncoder[motorD])*AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - startTime < MAX_AXIS_TIME))
	{}
	motor[motorD] = 0;

	if (time1[T1] - startTime > MAX_AXIS_TIME)
		executed = false;
	return executed;
}

/*
Reads in user array and saves settings
settings[0]: water cycle interval, settings[1]: rotation cycle interval
date[0]: day, date[1]: month, date[2]: year
*/
void readUserSettings(TFileHandle& config, string& plantName, float settings[], int date[])
{
	string header = " "; //ignore headers on config file
	readTextPC(config, header);
	readTextPC(config, plantName);
	
	readTextPC(config, header);
	readFloatPC(config, settings[0]);
	readTextPC(config, header);
	readFloatPC(config, settings[1]);
	
	readTextPC(config, header);
	readIntPC(config, date[0]);
	readTextPC(config, header);
	readIntPC(config, date[1]);
	readTextPC(config, header);
	readIntPC(config, date[2]);

	return plantName;
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

	nMotorEncoder[motorB] = 0;
	if (clockwise)
		motor[motorB] = ROTATION_SPEED;
	else
		motor[motorB] = -ROTATION_SPEED;

	while((abs(nMotorEncoder[motorB])*ROTATION_CONVERSION_FACTOR < ROTATION_DISTANCE) && (time1[T1] - startTime < MAX_ROTATION_TIME)) //fail-safe
	{}
	motor[motorB] = 0;
	
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
	float startTime = time1[T1]; // fail safe timer
	
	while (!checkFillLevel()) //no water
	{
		displayFillLevel();
	}
	startPump();

	//activate 2D axis
	if (executed)
	{
		nMotorEncoder[motorC] = nMotorEncoder[motorD] = 0;
		motor[motorC] = motor[motorD] = AXIS_SPEED;
		while((abs(nMotorEncoder[motorD])*AXIS_CONVERSION_FACTOR < Y_AXIS_LENGTH) && (time1[T1] - startTime < MAX_AXIS_TIME) && (time1[T1] - startTime < MAX_PUMP_TIME)) //fail-safe
		{
			
			while(abs(nMotorEncoder[motorC])*AXIS_CONVERSION_FACTOR < X_AXIS_LENGTH)
			{}
			motor[motorC] *= -1;
			nMotorEncoder[motorC] = 0;
		}
		motor[motorC] = motor[motorD] = motor[motorA] = 0; //stop axis and pump
		if (time1[T1] - startTime < MAX_AXIS_TIME)
			executed = resetWaterCycle();
	}
	
	if (time1[T1] - startTime > MAX_ROTATION_TIME)
		executed = false;
	return executed;
}

//meeji
void generateStats(string plantName, float settings[], int date[])
{}

//kira
void generateEndFile(TFileHandle& fout, string plantName, float settings[], int date[])
{
	/*
 	writeEndlPC(fout);
  	string s, float f, int i, etc.
  	writeTextPC(fout, s);
   	writeFloatPC(fout, f);
    	writeFloatPC(fout, "%.2f", f); //this is how they do it on the doc but i feel like it's writeTextPC not writeFloatPC
     	writeIntPC(fout, i);
	*/
}

//kira
void generateFailFile(TFileHandle& fout, string plantName, float settings[], int date[], int task)
{}

//emma
void activateGreenhouse(float settings[], string plantName, int date[])
{}

void safeShutDown()
{
	motor[motorA] = 0; //stop pump
	motor[motorB] = 0; //stop rotation
	resetWaterCycle();
	generateFailFile();
}

task main()
{
	clearTimer(T1);
	float runTime = 0.0;
	
	configureSensors();
	
	TFileHandle config;
	bool configOpen = openReadPC(config, "config.txt");
	TFileHandle fout;
	bool foutOpen = openWritePC(fout, "output.txt"); //syntax

	if (configOpen && foutOpen)
	{
		string plantName = " ";
		float settings[2] = {0.0, 0.0}; //water cycle interval, rotation cycle interval
		int date[3] = {0, 0, 0}; //day, month, year
		plantName = readUserSettings(config, settings, date);

		closeFilePC(fout);
		closeFilePC(config);
	}
	else
	{
		displayTextLine(5, "ERROR opening files.");
	}
}
