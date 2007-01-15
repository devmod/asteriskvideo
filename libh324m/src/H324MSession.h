#ifndef _H324MSESSION_H_
#define _H324MSESSION_H_

#include "H223Demuxer.h"
#include "H223Muxer.h"
#include "H324MControlChannel.h"
#include "H324MMediaChannel.h"
#include "H245MasterSlave.h"
#include "H245TerminalCapability.h"
#include "H245RoundTrip.h"
#include "H245MuxTable.h"
#include "H245LogicalChannels.h"
#include "H245MaintenanceLoop.h"
#include "H245ChannelsFactory.h"

class H324MSession :
	public H324MControlChannel
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
	int ReadControlFrane(BYTE *input,int length);
	int Write(BYTE *input,int length);
	
protected:
	//Overrides
	virtual int OnCallSetup();
	virtual int OnMediaSetup();
	
private:
	H223Muxer			muxer;
	H223Demuxer			demuxer;
	H223MuxTable		tableMux;
	H223MuxTable		tableDemux;
	CallState			state;
	H324MAudioChannel 	audioChannel;
	H324MVideoChannel 	videoChannel;
};

#endif
