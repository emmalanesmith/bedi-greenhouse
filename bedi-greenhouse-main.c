/*
Plant Bed(i) Greenhouse:
An automated plant incubator for bringing up houseplants, powered by a LEGO Mindstorms EV3 Robot.
By: Sevita Moiseev, Emma Lane-Smith, Kira Costen, Meeji Koo
Last Updated: 11/14/2024
*/

#include "PC_FileIO.c"

//Fail-safe method identifiers
const int PUMP = 0;
const int ROTATION = 1;
const int WATER_CYCLE = 2;

//Fail-safe max times
const float MAX_PUMP_TIME = 0.0; //set empirically
const float MAX_AXIS_TIME = 0.0;
const float MAX_ROTATION_TIME = 0.0;

//Rotation & speeds
const float ROTATION_DISTANCE = 0.0; //set empirically (for 90 degrees rotation)
const int ROTATION_SPEED = 25; //set empirically
const int PUMP_SPEED = 25; //set empirically
const int MAX_ROTATIONS = 2; //change direction after 2 turns
const float CONVERSION_FACTOR = 0.0; //set empirically

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

float startPump()
{
	float startTime = time1[T1]; //for the fail safe timer
	motor[motorA] = PUMP_SPEED;
	return startTime;
}

float stopPump()
{
	float startTime = time1[T1];
	motor[motorA] = 0;
	return startTime;
}

void resetWaterCycle()
{}

void stopRotation()
{}

string readUserSettings(TFileHandle& config, float settings[])
{}

/*
Powers the motors to turn 90 degrees (at ROTATION_SPEED)
Switches directions after 180 degrees (MAX_ROTATIONS)
int& numRotations: number of rotations thus far
bool& clockwise: true for CW, false for CCW
*/
float rotateGreenhouse(int& numRotations, bool& clockwise)
{
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

	while(abs(nMotorEncoder[motorB])*CONVERSION_FACTOR < ROTATION_DISTANCE)
	{}

	motor[motorB] = 0;
	return startTime;
}

/*
Checks if water is available
Starts the pump and the 2D axis motors
Stops pump and returns motors to origin
*/
float activateWaterCycle()
{
	float startTime = time1[T1]; // fail safe timer

	while (!checkFillLevel()) //no water
	{
		displayFillLevel();
	}
	startPump();
	wait1Msec(10000); // insert time (empirically)
	//activate 2D axis motors
	stopPump();
	resetWaterCycle();

	return startTime;
}

void generateStats(string plantName, float settings[])
{}

void generateEndFile(TFileHandle& fout, string plantName, float settings[])
{}

void generateFailFile(TFileHandle& fout, string plantName, float settings[], int task)
{}

bool checkFail(int task, float startTime)
{}

void activateGreenhouse(float settings[], string plantName)
{}

void safeShutDown()
{
	stopPump();
	stopRotation();
	resetWaterCycle();
	generateFailFile();
}

task main()
{
	configureSensors();
	
	TFileHandle config;
	bool fileOkay = openReadPC(config, "config.txt");
	TFileHandle fout;
	bool fileGood = openWritePC(fout, "output.txt"); //syntax

	closeFilePC(fout);
	closeFilePC(config);
}
