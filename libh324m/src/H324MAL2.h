#ifndef _H324MAL2_H_
#define _H324MAL2_H_

#include "H223AL.h"
#include "H324pdu.h"
#include "H223MuxSDU.h"

class H223AL2Receiver :
	public H223ALReceiver
{
public:
	//Constructor
	H223AL2Receiver(int useSequenceNumbers);
	virtual ~H223AL2Receiver();

	//H223ALReceiver interface
	virtual void Send(BYTE b);
	virtual void SendClosingFlag();

	//Methods
	H223MuxSDU* GetFrame();
	int NextFrame();

private:
	int	 useSN;
	H223MuxSDU sdu;
	H223MuxSDUList frameList;
};


class H223AL2Sender :
	public H223ALSender
{
public:
	//Constuctor
	H223AL2Sender(int useSequenceNumbers);
	virtual ~H223AL2Sender();

	//Methods
	int SendPDU(BYTE *buffer,int len);

	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU();
	virtual void OnPDUCompleted();

private:
	int useSN;
	BYTE sn;
	H223MuxSDUList frameList;
};

#endif
