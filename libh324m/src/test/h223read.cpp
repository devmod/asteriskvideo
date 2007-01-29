#include "../H324MSession.h"


int main(int argc,char **argv)
{
	H324MSession session;

	// Check number of parameters
	if (argc<2)
	{
		printf("usage: h223read <filename>\n");
		return 1;
	}

	//Open file
	int f = open(argv[1],O_RDONLY);

	//Check
	if (f==-1)
	{
		printf("unable to open [%s]\n",argv[1]);
		return 2;
	}

	//Init session
	session.Init();
	
	//Read
	unsigned char buffer[160];
	int len;

	//Until end of file
	while ((len=read(f,buffer,160))>0)
	{
		//Read
		session.Read(buffer,160);
		//Write
		session.Write(buffer,160);
	}

	//End session
	session.End();

	return 0;
	
}
