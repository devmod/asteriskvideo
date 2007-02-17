#ifndef _H324MSESSION_H_
#define _H324MSESSION_H_

#include "H245ChannelsFactory.h"
#include "H324MControlChannel.h"

class H324MSession 
{
public:
	//Enums
	enum CallState
	{
		e_None,
		e_Setup,
		e_SetupMedia,
		e_Stablished,
		e_Hangup
	};

	H324MSession();
	virtual ~H324MSession();

	//Init functions
	int Init();
	Frame* GetFrame();
	int SendFrame(Frame *frame);
	int End();
	
	//Mux & demux
	int Read(BYTE *input,int length);
	int Write(BYTE *input,int length);
	
private:

	CallState			state;
	H245ChannelsFactory channels;
	H324MControlChannel *controlChannel;
	int	audio;
	int	video;
	
};

#endif
