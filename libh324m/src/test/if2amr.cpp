#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../../bits.c"

static short blockSize[16] = { 12, 13, 15, 17, 19, 20, 26, 31,  5, -1, -1, -1, -1, -1, -1, -1};

int main(int argc, char** argv) 
{
	//Check params
	if (argc<3)
	{
		printf("usage: if2amr <input-file> <output-file>\n");
		return 1;
	}

	//Open output
	int fdOut = open(argv[2],O_CREAT|O_WRONLY);

	//If not opened
	if (fdOut==-1)
	{
		printf("unable to create [%s]\n",argv[2]);
		return 2;
	}

	//Open input
	int fdIn = open(argv[1],O_RDONLY);

	//If not opened
	if (fdIn==-1)
	{
		printf("unable to open [%s]\n",argv[1]);
		return 2;
	}

	//Write amr header
	write(fdOut,"#!AMR\n",6);

	//frame header
	unsigned char header;

	//Read all frames
	while(read(fdIn,&header,1))
	{
		unsigned char buffer[1024];

		//Get frame mode
		unsigned char mode = header & 0x0F;

		//Get frame size
		short size = blockSize[mode];

		if(size == -1)
			continue;

		printf("-Frame [%.2x,%.2x,%d]\n",header,mode,size);

		//Read frame size
		if (read(fdIn,buffer+1,size-1)!=size-1)
			//Exit
			break;

		//Set header
		buffer[0] = (mode << 3) | 0x04;

		//For each byte
		for (int i=size; i>1; i--)
			//Move bits
			buffer[i] = (buffer[i] << 4) | (buffer[i-1] >>  4);

		//Set first byte
		buffer[1] = (buffer[1] << 4) | (header >> 4);

		//Reverse
		TIFFReverseBits(buffer+1,size);
			
		//Save frame
		write(fdOut,buffer,size+1);	
	}	
	
	//close files
	close(fdOut);
	close(fdIn);

	//Exit
	return(0);
} 
