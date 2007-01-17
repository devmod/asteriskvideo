#ifndef _H324MSESSION_H_
#define _H324MSESSION_H_

#include "H223Demuxer.h"
#include "H223Muxer.h"
#include "H324MControlChannel.h"
#include "H324MMediaChannel.h"

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

	//Init functions
	int Init();
	int End();
	
	//Mux & demux
	int Read(BYTE *input,int length);
	int Write(BYTE *input,int length);
	
private:
	H223Muxer			muxer;
	H223Demuxer			demuxer;
	H223MuxTable		tableMux;
	H223MuxTable		tableDemux;
	CallState			state;
	H324MAudioChannel 	audioChannel;
	H324MVideoChannel 	videoChannel;
	H324MControlChannel controlChannel;
};

#endif
