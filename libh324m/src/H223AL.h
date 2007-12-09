#ifndef _H223AL_H_
#define _H223AL_H_

#include "H324MConfig.h"
#include "H223MuxSDU.h"

class H223ALReceiver
{
public:
	//H223ALReceiver interface
	virtual void Send(BYTE b)=0;
	virtual void SendClosingFlag()=0;
	virtual int IsSegmentable() = 0;
	virtual ~H223ALReceiver() {}
};

class H223ALSender
{
public:
	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU()=0;
	virtual void OnPDUCompleted()=0;
	virtual int IsSegmentable() = 0;
	virtual ~H223ALSender() {}
};

class H223SDUListener
{
public:
	//H223SDUListener
	virtual void OnSDU(BYTE* data,DWORD length) = 0;
	virtual ~H223SDUListener() {}
};
#endif

