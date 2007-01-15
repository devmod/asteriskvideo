#include "H223Flag.h"

BYTE H223Flag::Append(BYTE b)
{
	BYTE out = 0;

	//Enque the byte into the header
	switch(length)
	{
		case 0:
			buffer[0] = b;
			length = 1;
			break;
		case 1:
			buffer[1] = b;
			length = 2;
			break;
		case 2:
			out       = buffer[0];
			buffer[0] = buffer[1];
			buffer[1] = b;
			break;
	}

	//return the byte
	return out;
}

int H223Flag::IsComplete()
{
	//Check length
	return (length==2);
}

int H223Flag::IsValid()
{
	//Check length
	if (length!=2)
		return 0;

	//Check for flag
	if (buffer[0]==0xE1 && buffer[1]==0x4D)
	{
		//Not complement
		complement = 0;
		//Exit
		return 1;
	}

	//Check for negative flag
	if (buffer[0]==(BYTE)(~0xE1) && buffer[1]==(BYTE)(~0x4D))
	{
		//complement
		complement = 1;
		//Exit
		return 1;
	}

	//No flag
	return 0;
}

void H223Flag::Clear()
{
	//Empty buffers
	buffer[0] = 0;
	buffer[1] = 0;

	//And the length
	length = 0;
}

