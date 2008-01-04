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
		printf("usage: amr2if <input-file> <output-file>\n");
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

	unsigned char buffer[1024];

	//Read amr header
	read(fdIn,buffer,6);

	//frame header
	unsigned char header;

	//Read all frames
	while(read(fdIn,&header,1))
	{
		//Reverse
		TIFFReverseBits(&header,1);

		//Get frame mode
		unsigned char mode = (header >> 3) & 0x0F; 

		//Get frame size
		short size = blockSize[mode];

		printf("-Frame [%.2x,%.2x,%d]\n",header,mode,size);

		//Read frame size
		if (read(fdIn,buffer,size)!=size)
			//Exit
			break;

		//Reverse
		TIFFReverseBits(buffer,size);

		//Pad the last
		buffer[size-1] = 0;

		//For each byte
		for (int i=size-1; i>0; i--)
			//Move bits
			buffer[i] = (buffer[i] << 4) | (buffer[i-1] >>  4);

		//Set first byte
		buffer[0] = (buffer[0] << 4) | mode;
			
		//Save frame
		write(fdOut,buffer,size);	
	}	
	
	//close files
	close(fdOut);
	close(fdIn);

	//Exit
	return(0);
} 
