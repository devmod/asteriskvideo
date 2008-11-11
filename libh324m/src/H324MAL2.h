#ifndef _H324MAL2_H_
#define _H324MAL2_H_

#include "H223AL.h"
#include "H324pdu.h"
#include "H223MuxSDU.h"
#include "jitterBuffer.h"
#include "log.h"

class H223AL2Receiver :
	public H223ALReceiver
{
public:
	//Constructor
	H223AL2Receiver(int segmentable,H223SDUListener* listener,int useSequenceNumbers);
	virtual ~H223AL2Receiver();

	//H223ALReceiver interface
	virtual void Send(BYTE b);
	virtual void SendClosingFlag();
	virtual int IsSegmentable();

private:
	int	useSN;
	int segmentableChannel;
	H223SDUListener* sduListener;
	H223MuxSDU sdu;
	Logger *logger;	
};


class H223AL2Sender :
	public H223ALSender
{
public:
	//Constuctor
	H223AL2Sender(int segmentable,int useSequenceNumbers);
	virtual ~H223AL2Sender();

	//Methods
	int SendPDU(BYTE *buffer,int len);
	void SetJitBuffer(int packets, int delay);
	void Tick(DWORD len);
	int Reset();

	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU();
	virtual void OnPDUCompleted();
	virtual int IsSegmentable();
private:
	int useSN;
	int segmentableChannel;
	BYTE sn;
	H223MuxSDU* pdu;
	jitterBuffer jitBuf;
	int minPackets;
	int minDelay;
	Logger *logger;	
};

#endif
