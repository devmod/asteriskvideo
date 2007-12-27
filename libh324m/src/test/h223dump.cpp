#include "../H223Demuxer.h"
#include "../H324CCSRLayer.h"

class DumpChannel :
	public H324CCSRLayer
{
public:
	//Method overrides from ccsrl
	virtual int OnControlPDU(H324ControlPDU &pdu)
	{
		cout << "Received \n" << pdu << "\n";
		return 1;
	}
};


int main(int argc,char **argv)
{
	// Check number of parameters
	if (argc<2)
	{
		printf("usage: h223dump <filename>\n");
		return 1;
	}

	//Set login high
	Logger::SetLevel(9);

	//Open file
	int f = open(argv[1],O_RDONLY);

	//Check
	if (f==-1)
	{
		printf("unable to open [%s]\n",argv[1]);
		return 2;
	}

	H223MuxTable localTable;
	DumpChannel dumpChannel;
	H223Demuxer demuxer;
	
	//Set entry
	localTable.SetEntry(0,"","0");
	
	//Set demuxer channels
	demuxer.SetChannel(0,&dumpChannel);

	//Open demuxer
	demuxer.Open(&localTable);

	//Read
	unsigned char buffer[160];
	int len;

	//Until end of file
	while ((len=read(f,buffer,160))>0)
		//Demux
		demuxer.Demultiplex(buffer,len);

	//Close demuxer
	demuxer.Close();

	//Close file
	close(f);

	return 0;
	
}
