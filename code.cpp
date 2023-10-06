/*

EV3 Printer
Group 8-4
Duncan Vanpagee
Izaac Laundry-Mottiar
Aidan Fletcher
Jonathan Zhou

Version 6.0

Expectations:
This is the full code for the EV3 Printer
The user selects an image from the 3 downloaded files
The printer will replicate the image and shut down upon completion

Assumptions:
There are no obstacles that prevent the print from succeeding.
The user follows the instructions properly.
The marker and paper are prepared properly.

Note:
The front of the printer is the side you can see the marker front/load the paper.

Motor Configuration:
MotorA - left vertical motor
MotorB - top X directional motor
MotorC - paper movement
MotorD - right vertical motor

Sensor1 - touch sensor for reset
Sensor2 -
Sensor3 - colour sensor for paper loading
Sensor4 -
*/

#include "PC_FileIO.c"

//initialize global constants
const float PXL_SIZE = 0.1;  	  	// the size of a pixel
const int DEG_DOWN = 43;    		  // distance that the pen travles down
const int COLS = 126;						  // number of columns in the image
const int COLOUR_REFLECTED = 0;		// the value reflected by color sensor when there is paper above it
const int MARKER_X_SPEED = 30;    // 35 for speed prints ; 10 for slow prints

//Function prototypes
void configureSensors();
void markerX(float dist_cm, int mPower);
void markDepth(int deg, int mPower);
void movePaper(float dist_cm, int mPower);
void resetTop(int upDownSpeed);
void touchReset(int mPower);
int printRow(int * printLn, int &degDown);
int startPrint(int printChoice);
int findNext(int*line, int currentIndex=0);
int menu();
int pauseButton(int currentDeg);
bool eStop();
void eject();

task main()
{
	//initialize
	configureSensors();
	clearTimer(T2);

	//start print
	startPrint(menu());
	displayString(8, "print time: %d seconds",time1(T2)/1000);

	//finish print
	resetTop(10);
	touchReset(-25);
	eject();

}

//Standard sensor configuration
void configureSensors()
{
	SensorType[S1] = sensorEV3_Touch;
	SensorType[S3] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S3] = modeEV3Color_Color;
	wait1Msec(50);
}

/*
int menu()
this function runs through the startup process of loading paper and other UI
this function returns a print choice, which the next function uses to print the chosen image
*/

int menu() //ILM
{
	//Start the print
	displayString(3, "Press the middle button");
	while(!getButtonPress(buttonEnter))
	{}

	//Load the paper
	eraseDisplay();
	while(!getButtonPress(buttonUp))
	{
		displayString(3, "Is the paper loaded?");
		displayString(5, "Up for yes, down for no.");
		displayString(9, "Is the paper loaded to far?");
		displayString(11, "If so, press the left button.");

		if(getButtonPress(buttonDown))
		{
			eraseDisplay();

			displayString(3, "Carefully push paper in.");
			displayString(5, "Once the paper is in");
			displayString(7, "push the right button.");

			while(!getButtonPress(buttonRight))
			{}

			eraseDisplay();

			//move the paper until it reaches the colour sensor
			motor[motorC] = 5;
			while(getColorReflected(S3) == COLOUR_REFLECTED)
			{}
			motor[motorC] = 0;
			wait1Msec(500);
			nMotorEncoder[motorC] = 0;
			motor[motorC] = -5;

			//move the paper to the optimal starting location
			const float ROTATE_WHEEL = 2.54;
			const float ENC_LIMIT = ROTATE_WHEEL*180/(2.5*PI);
			while(abs(nMotorEncoder(motorC)) < ENC_LIMIT)
			{}

			motor[motorC] = 0;
		}
		else if(getButtonPress(buttonLeft))
		{
			eraseDisplay();
			motor[motorC] = -5;

			while(getColorReflected(S3) == COLOUR_REFLECTED)
			{}
			motor[motorC] = 0;

			const float ROTATE_WHEEL = 2.54;
			const float ENC_LIMIT = ROTATE_WHEEL*180/(2.5*PI);

			while(abs(nMotorEncoder(motorC)) < ENC_LIMIT)
			{}

			motor[motorC] = 0;
		}

	}
	while(getButtonPress(buttonAny))
	{}
	//wait for all buttons to be released

	eraseDisplay();

	//Print Selection
	displayString(3, "Press the right button");
	displayString(4, "to print heart.");
	displayString(6, "Press the down button");
	displayString(7, "to print smiley face.");
	displayString(9, "Press the left button");
	displayString(10, "to print a dog.");
	int printChoice = 0;
	while(printChoice == 0)
	{
		if(getButtonPress(buttonRight))
		{
			while(getButtonPress(buttonRight)){}
			printChoice = 1;
		}
		if(getButtonPress(buttonDown))
		{
			while(getButtonPress(buttonDown)){}
			printChoice = 2;
		}

		if(getButtonPress(buttonLeft))
		{
			while(getButtonPress(buttonLeft)){}
			printChoice = 3;
		}
	}
	//reset everything
	resetTop(10);
	wait1Msec(1000);
	//zero encoder
	nMotorEncoder[motorD] = 0;
	nMotorEncoder[motorA] = 0;
	touchReset(-25);

	//returns the printChoice
	return printChoice;
}

/*
int startPrint(int printChoice)

This function starts the print given the printChoice
There are three print choices: A dog, a heart, and a smiley face

This function then reads each row of the file
The function creates an array for each row and sends the array to printRow

This function returns 0 if the emergency stop is pressed
*/
int startPrint(int printChoice) // JZ
{
	int rows = 0, cols = 0; // rows and cols of the image.

	string printFile = "";
	if(printChoice == 1)
	{
		printFile = "Heart.txt";
	}
	else if(printChoice == 2)
	{
		printFile = "s_smile.txt";
	}
	else
	{
		printFile = "SmallDog.txt";
	}


	//open the image file
	TFileHandle fin;
	//the first 2 inputs of the file are the row and column size
	//these lines set the rows and cols values to the size of the image
	readIntPC(fin,cols);
	readIntPC(fin,rows);

	//Print the size of the rows and cols
	eraseDisplay();
	displayString(3,"Printing: %s",printFile);

	//CurrentRow an array to represent the current row
	int currentRow[126]; // assumes the array size is 126 right now.
	// for some reason we can't initilaize the array by a variable.
	//loop through the file

	//this variable adjusts the distance that the marker travels
	int degreeDown = DEG_DOWN;


	for(int x = 0; x < rows; x++)
	{
		for(int y = 0; y < cols; y++)
		{
			//read in the entire row and put it into currentRow
			readIntPC(fin,currentRow[y]);
		}
		//pass the row into printRow();
		if(printRow(currentRow, degreeDown) == -1)// the estop has been pressed
		{
			return 0;
		}
	}
	return 1;
}


/*
int findNext(int*line, int currentIndex)

	this function is given an array of the row and an index
	and returns an integer which gives the printer its next instruction

	returns 0 if the current index is a dot
	returns -1 if there are no more pixels in the row
	else
	returns the number of white spaces before the next dot
*/
int findNext(int*line, int currentIndex) // AF
{
	const int lineSize = 126;
	int numberWhite = 0;

	if(line[currentIndex] == 1)//If we are on a dot, return 0.
	{
		return 0;
	}
	else
	{
		//Seach through the array, 0 is white, 1 is a dot.
		for(int index = currentIndex; index < lineSize; index++)
		{
	  	int pixel = -1;

	  	pixel = line[index];

	  	if(pixel == 0)
	  	{
	  		numberWhite++;
	  	}
	  	if(pixel == 1)//When a dot is found, break the loop.
	  	{
	  		index = lineSize;
	  	}
		}
		int sum = 0;
		for(int i = currentIndex; i < lineSize; i++)
		{
		 sum += line[i];
		}
		if(sum == 0) //If there are no pixels until the end of the line.
		{
			return -1;
		}
		return numberWhite;
	}
}

/*
int pauseButton(int currentDeg)
A pausebutton that allows the user to adjust the distance the marker travels
returns an integer
*/
int pauseButton(int currentDeg) // JZ
{
	int degDownAdjust = 0;
	bool paused = false;
	if (getButtonPress(buttonEnter))
	{
		paused = true;
	}
	while(getButtonPress(buttonEnter)) // wait for the button to be released
	{}
	while(paused)
	{
		displayString(5, "PAUSED");
		displayString(7,"DEG_DOWN: %d", currentDeg + degDownAdjust);
		if(getButtonPress(buttonUp))
		{
			while(getButtonPress(buttonUp)){}
			degDownAdjust++;
		}

		else if(getButtonPress(buttonDown))
		{
			while(getButtonPress(buttonDown)){}
			degDownAdjust--;
		}
		if(getButtonPress(buttonEnter))
		{
			paused = false;
		}
	}
	displayString(5,"       ");
	return degDownAdjust;
}

/*
bool eStop()
This function is constantly looped in the printRow function and checks if buttonLeft is PRESSED
returns true if buttonleft is pressed and stops all motors
*/
bool eStop() // JZ
{
	if(getButtonPress(buttonLeft))
	{
		eraseDisplay();
		displayString(7,"ESTOP Detected! Stopping Print");
		motor[motorA] = motor[motorB] = motor[motorC] = motor[motorD] = 0;
		wait1Msec(2000);
		return true;
	}
	return false;
}


/*
 void markerX(float dist_cm, int mPower);
 	this function moves the maker across the rack.
	positive motor power goes left, and negative goes right
*/
void markerX (float dist_cm, int mPower)//DVP

{
	int encLim = dist_cm * 180 / (PI * 1) ;//1" is wheel diameter
	nMotorEncoder[motorB] = 0; //possible compounding error
	motor[motorB] = mPower;
	while (abs(nMotorEncoder[motorB]) < encLim)
	{}
	motor[motorB] = 0;
}

/*
void markDepth (int deg, int mPower)
this function moves the marker to and from the paper.
*/
void markDepth (int deg, int mPower)//DVP
{

	motor[motorD] = motor[motorA] = -mPower; //negative is towards the paper (down)
	while(abs(nMotorEncoder[motorD]) < deg || abs(nMotorEncoder[motorA]) < deg) // both motors reach deg
	{}

	motor[motorD] = motor[motorA] =  (1.5 * mPower);// positive is away from paper (up)
	while(nMotorEncoder[motorD] < 0 || nMotorEncoder[motorA] < 0) // both motors reach original psoition
	{}
	motor[motorD] = motor[motorA] = 0;

	return;
}

/*
void movePaper(float dist_cm, int mPower)
This function moves the paper using the paper motor
*/
void movePaper (float dist_cm, int mPower)//DVP
{
	float encLim = dist_cm * 180 / (PI * 1.2) ; //convert distance to encoder value
	nMotorEncoder[motorC] = 0;
	motor[motorC] = mPower;
	while (nMotorEncoder[motorC] < encLim)
	{}
	motor[motorC] = 0;
}

/*
void resetTop(int upDownSpeed)
	Resets the vertical
	This will slowly run the vertical gear racks upwards, until they reach their physical stop point.
  The code will look for a certain amount of change in motor encoders over a time interval to determine if
  it is still moving or has stopped.
	Then it will move it to the correct height to begin printing.
  The vertical sliders use two motors to move, which allows for the rack to technically be off by some teeth on each side (see images of robot).
  This function reset should still work even if the two sides are offset when starting.
	Recommended motor speed is +8
*/
void resetTop(int upDownSpeed)//AF
{

//The following constants were determined through testing
	const int degDown =185;// The amount the vertical slider moves down after the reset
	const int timeInterval=150; //Reset time interval in MS
	const int changeTarget=1; //The min amount of change in degrees that means the slider is still moving.

	motor[motorD] = motor[motorA] = abs(upDownSpeed);// positive is away from paper (up)
	//Will be driving very slowly until the pins are touched.
	do
	{
		//The two vertical slider motors are A and D.
		nMotorEncoder[motorD]=0;
		nMotorEncoder[motorA]=0;
		wait1Msec(timeInterval);
	}while(nMotorEncoder[motorD] >= changeTarget && nMotorEncoder[motorD] >= changeTarget);

	motor[motorD] = motor[motorA] = 0;

	//Move down to the printing height above the paper.

	nMotorEncoder[motorD]=0;

	motor[motorD] = motor[motorA] = -upDownSpeed; //negative is towards the paper (down)
	while(nMotorEncoder[motorD] > -degDown)
	{}
	motor[motorD] = motor[motorA] = 0;
	return;
}


/*
void touchReset(int mPower)
This function resest the print head horizontally using the touch sensor
*/
void touchReset(int mPower) //DVP
{
	motor[motorB] = mPower;
	while(!SensorValue(S1))
	{}
	motor[motorB] = 0;
	markerX(PXL_SIZE*12,-mPower);
}

/*
int printRow(int *println, int &degDown)
This function prints a row of the image given an array of the row
*/
int printRow(int * printLn, int &degDown) //DVP
{
	//DONELINE is the break value.
	const int DONELINE = 200;
	//check if the entire line is blank. If so, then do nothing/ go to next row.
	int checkIfEntireLineIsBlank = findNext(printLn,0);
	if( checkIfEntireLineIsBlank == -1)
	{}
	else //the entire line is not blank
	{
		for(int cols = 0; cols < COLS; cols++)
		{
			displayString(7,"DEG_DOWN: %d", degDown);
			int white = 0;
			white = findNext(printLn,cols); // set read-in value to colour

			if(white == 0) // the pixel is NOT white
			{
				markerX(PXL_SIZE,35);
				markDepth(degDown,35);

			}else

			if (white == -1) // there are no more dots left in the row
			{
				cols = DONELINE; //break out of the function
			}
			else // move across white space until pixel detected
			{
				markerX((PXL_SIZE*white),MARKER_X_SPEED);
				cols += (white - 1);
			}
			degDown +=	pauseButton(degDown); // the pauseButton returns the value of the offset
			if(eStop())
			{
				return -1; // stop!
			}
		}
		//reset the print head horizontall
		touchReset(-35);
		}
		//feeds the paper down
		movePaper(PXL_SIZE/1.75,20);
		return 0;
}
/*
void eject()
This function is used at the end of prints to eject the paper.
*/
void eject() // DVP
{
	//eject paper
	motor[motorC] = -100;
	while(getColorReflected(S3) != COLOUR_REFLECTED)
	{}
	wait1Msec(1000);
	motor[motorC] = 0;
	wait1Msec(10000);
}

