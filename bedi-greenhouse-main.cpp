// Non-Trivial Functions **********************

const double CONVERSION_FACTOR = 2.75;
const double NINTEY_DEGREES_DISTANCE = 10; // change after measuring

// Rotate green house
double rotateGreenhouse(int& numRotations)
{
	double startTime = time1[T1]; // fail safe timer
	int counter = 0;
	bool direction; // true for ccw - false for cw

	// turns ccw
	while (direction)
	{
		motor[motorB] = 25; // check wiring to ensure correct motor
		while (nMotorEncoder[motorB] * CONVERSION_FACTOR < NINETY_DEGREES_DISTANCE)
		{}
		motor[motorB] = 25; // check wiring to ensure correct motor
		counter++;

		// switches direction once it rotates the max numRotations
		if (counter == numRotations)
		{
			direction = false;
			counter = 0;
		}
	}

	// turns cw
	while (!direction)
	{
		motor[motorB] = 25; // check wiring to ensure correct motor
		while (nMotorEncoder[motorB] * CONVERSION_FACTOR < NINETY_DEGREES_DISTANCE)
		{}
		motor[motorB] = 25; // check wiring to ensure correct motor
		counter++;

		// switches direction once it rotates the max numRotations
		if (counter == numRotations)
		{
			direction = true;
			counter = 0;
		}
	}
}

// Activate Water Cycle
double activateWaterCycle()
{
	double startTime = time1[T1]; // fail safe timer

	while (checkFillLevel())
	{}
	startPump();
	wait1Msec(); // insert time (empirically)
	stopPump();
	resetWaterCycle();

	return time1;
}

// Trivial Functions **************************

// Check fill level
bool checkFillLevel()
{
	if (SensorMode[S1] == white) //check wiring to ensure correct sensor port // check what code for 'white' is
	{
		return 0;
	}
	else if (!SensorMode[S1] == white)
	{
		return 1;
	}
}

// Displace fill level
void displayFillLevel(string fillLevel)
{
	if (checkFillLevel == 1)
	{
		displayString (5, "Fill level: %s", "Full");
	}
	else
	{
		displayString (5, "Fill level: %s", "Empty");
	}
}

// Start pump
double startPump()
{
	time1 = 0; //fail safe timer
	motor[motorA] = 25; //check wiring to ensure correct motor

	return time1;
}

// Stop pump
void stopPump()
{
	time1 = 0; //fail safe timer
	motor[motorA] = 0; //check wiring to ensure correct motor

	return time1;
}

// Ignore for now
task main()
{
}
