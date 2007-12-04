#ifndef _H324MSESSION_H_
#define _H324MSESSION_H_

#include "H245ChannelsFactory.h"
#include "H324MControlChannel.h"
#include "log.h"

class H324MSession 
	: public H245ChannelsFactoryListener
{
public:
	//Enums
	enum CallState
	{
		e_None			= 0,
		e_Setup			= 1,
		e_SetupMedia	= 2,
		e_Stablished	= 3,
		e_Hangup		= 4 
	};

	H324MSession();
	virtual ~H324MSession();

	//Init functions
	int Init();
	int End();

	//Media frame functions
	Frame*	GetFrame();
	int		SendFrame(Frame *frame);

	//User input functions
	char*	GetUserInput();
	int		SendUserInput(const char *input);

	//Cmds & indications
	int		SendVideoFastUpdatePicture();
	int		ResetMediaQueue();
	CallState	GetState();

	//H245ChannelsFactoryListener
	virtual int OnChannelStablished(int channel, MediaType type);
	virtual int OnChannelReleased(int channel, MediaType type);
	
	//Mux & demux
	int Read(BYTE *input,int length);
	int Write(BYTE *input,int length);
	
private:
	CallState			state;
	H245ChannelsFactory channels;
	H324MControlChannel *controlChannel;
	Logger *logger;
	int	audio;
	int	video;
	
};

#endif
