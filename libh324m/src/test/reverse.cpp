#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../../bits.c"

int main(int argc, char** argv) 
{
	//Check params
	if (argc<3)
	{
		printf("usage: reverse <input-file> <output-file>\n");
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

	//Read
	unsigned char buffer[1024];
	int len;

	//Until end of file
	while((len=read(fdIn,buffer,1024))>0)
	{
		//Reverse bits
               	TIFFReverseBits(buffer,len);
		//Write
		write(fdOut,buffer,len);	
	}	
	
	//close files
	close(fdOut);
	close(fdIn);

	//Exit
	return(0);
} 
