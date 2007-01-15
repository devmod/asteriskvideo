#include "H223MuxSDU.h"

H223MuxSDU::H223MuxSDU()
{
	//Set default size
	size = 256;
	
	//Set pointers
	ini = 0;
	end = 0;

	//Allocate memory
	buffer = (BYTE*)malloc(size);
}

H223MuxSDU::~H223MuxSDU()
{
	//Free memory
	free(buffer);
}

H223MuxSDU::H223MuxSDU(BYTE *b,int len)
{
	//Set the size
	size = len+256;

	//Allocate memory
	buffer = (BYTE*)malloc(size);

	//Copy the buffer
	memcpy(buffer,b,len);

	//Set end
	end = len;
	ini = 0;
}

void H223MuxSDU::Push(BYTE b)
{
	//Check if there is enougth room
	if (end+1>size)
	{
		//Increment size in 256
		size += 256;

		//incremente size
		BYTE *aux = (BYTE*)malloc(size);

		//Copy the buffer
		memcpy(aux,buffer,end);

		//Free old buffer
		free(buffer);

		//Set new buffer
		buffer = aux;
	}

	//Set the byte
	buffer[end++]=b;
}

void H223MuxSDU::Push(BYTE *b,int len)
{
	//Check if there is enougth room
	if (end+len>size)
	{
		//Increment size
		size += len+256;

		//incremente size
		BYTE *aux = (BYTE*)malloc(size);

		//Copy the buffer
		memcpy(aux,buffer,end);

		//Free old buffer
		free(buffer);

		//Set new buffer
		buffer = aux;
	}

	//Insert
	memcpy(buffer+end,b,len);

	//Increase end
	end+=len;
}

BYTE H223MuxSDU::Pop()
{
	if(ini>=end)
		*((BYTE*)0)=0;
	//Exit
	return buffer[ini++];
}

int	 H223MuxSDU::Length()
{
	return end-ini;
}

void H223MuxSDU::Begin()
{
	ini = 0;
}

void H223MuxSDU::Clean()
{
	ini = 0;
	end = 0;
}
