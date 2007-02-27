#ifndef _H324MAL3_H_
#define _H324MAL3_H_

#include "H223AL.h"
#include "H324pdu.h"
#include "H223MuxSDU.h"

class H223AL3Receiver :
	public H223ALReceiver
{
public:
	//Constructors
	H223AL3Receiver(int segmentable,H223SDUListener* listener);
	virtual ~H223AL3Receiver();
	//H223ALReceiver interface
	virtual void Send(BYTE b);
	virtual void SendClosingFlag();
	virtual int IsSegmentable();
private:
	int segmentableChannel;
	H223SDUListener* sduListener;
	H223MuxSDU sdu;
};


class H223AL3Sender :
	public H223ALSender
{
public:
	//Constuctor
	H223AL3Sender(int segmentable);
	virtual ~H223AL3Sender();

	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU();
	virtual void OnPDUCompleted();
	virtual int IsSegmentable();

private:
	int segmentableChannel;
	H223MuxSDUList frameList;
};

#endif

