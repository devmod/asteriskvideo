#ifndef _H324MAL3_H_
#define _H324MAL3_H_

#include "H223AL.h"
#include "H324pdu.h"
#include "H223MuxSDU.h"

class H223AL3Receiver :
	public H223ALReceiver
{
public:
	//H223ALReceiver interface
	virtual void Send(BYTE b);
	virtual void SendClosingFlag();
};


class H223AL3Sender :
	public H223ALSender
{
	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU();
	virtual void OnPDUCompleted();
};

#endif
