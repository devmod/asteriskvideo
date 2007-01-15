#include "H223Header.h"

int H223Header::IsComplete()
{
	return (length==3);
}

int H223Header::IsValid()
{
	/*
	mc = (header.buffer[0] & 0x1E) >> 1;
	pm = (header.buffer[0] & 0x01);
	*/
	
	//Get the values
	mc   = buffer[0] & 0x0F;
	mpl  = (buffer[0] >> 4) | (buffer[1] & 0x0F);
	pm   = (buffer[1] >> 4) | (buffer[2] << 4);

	//Calculate the golay code
	return 1;

}

BYTE H223Header::Append(BYTE b)
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
			buffer[2] = b;
			length = 3;
			break;
		case 3:
			out       = buffer[0];
			buffer[0] = buffer[1];
			buffer[1] = buffer[2];
			buffer[2] = b;
			break;
	}

	//return the byte
	return out;
}

void H223Header::Clear()
{
	//Empty buffers
	buffer[0] = 0;
	buffer[1] = 0;

	//And the length
	length = 0;
}
