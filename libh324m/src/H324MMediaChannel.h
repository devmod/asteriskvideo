#ifndef _H324MMEDIACHANNEL_H_
#define _H324MMEDIACHANNEL_H_

#include "H324MAL1.h"
#include "H324MAL2.h"
#include "H324MAL3.h"

class H324MMediaChannel
{
public:
	enum e_Type {
		Audio,
		Video
	};

	enum State {
      e_Released,
      e_AwaitingEstablishment,
      e_Established,
      e_AwaitingRelease,
      e_AwaitingConfirmation,
      e_AwaitingResponse,
      e_NumStates
    };

public:
	H324MMediaChannel()
	{
		state = e_AwaitingEstablishment;
		localChannel = 0;
		remoteChannel = 0;
		isBidirectional = 0;
		sender = NULL;
		receiver = NULL;
	}

	int Init();
	int End();

	H223ALSender*  GetSender();
	H223ALReceiver* GetReceiver();

	int SetSenderLayer(int number);
	int SetReceiverLayer(int number);

	int localChannel;
	int remoteChannel;
	int isBidirectional;

public:
	e_Type type;
	State state;

private:
	H223ALReceiver *receiver;
	H223ALSender *sender;
};

class H324MAudioChannel : 
	public H324MMediaChannel
{
public:
	H324MAudioChannel()
	{
		type = Audio;
	};
};

class H324MVideoChannel :
	public H324MMediaChannel
{
public:
	H324MVideoChannel() 
	{
		type = Video;
	};
};

#endif
