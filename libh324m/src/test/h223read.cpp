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
	Frame *frame;

	//Until end of file
	while ((len=read(f,buffer,10))>0)
	{
		//Read
		session.Read(buffer,10);
		//Loopback
		while((frame=session.GetFrame())!=NULL)
		{
			//If it's video
			if (frame->type==e_Video)
				//Loop
				session.SendFrame(frame);
			//Delete frame
			delete frame;
		}
		//Write
		session.Write(buffer,10);
	}

	//End session
	session.End();

	return 0;
	
}
