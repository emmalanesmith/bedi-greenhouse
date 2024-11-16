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
const float PUMP_TIME = 0.0; //set emprirically
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

//emma to do 11/16/2024
void resetWaterCycle()
{}

/*
Reads in user array and saves settings
settings[0]: water cycle interval, settings[1]: rotation cycle interval
date[0]: day, date[1]: month, date[2]: year
*/
string readUserSettings(TFileHandle& config, float settings[], int date[])
{
	string plantName = " ", header = " "; //ignore headers on config file
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

//emma to do 11/16/2024
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
	wait1Msec(PUMP_TIME); // insert time (empirically)
	//activate 2D axis motors
	stopPump();
	resetWaterCycle();

	return startTime;
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

//emma to do 11/16/2024
bool checkFail(int task, float startTime)
{}

//emma to do 11/16/2024
void activateGreenhouse(float settings[], string plantName, int date[])
{}

void safeShutDown()
{
	stopPump();
	stopRotation();
	resetWaterCycle();
	generateFailFile();
}

//emma update after finishing functions 11/16/2024
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
