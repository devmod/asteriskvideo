#ifndef _H223Demuxer_H_
#define _H223Demuxer_H_

#include "H324MConfig.h"
#include "H223AL.h"
#include "H223MuxTable.h"
#include "H223Flag.h"
#include "H223Header.h"
#include "log.h"

#include <map>

class H223Demuxer
{
private:
	typedef map<int,H223ALReceiver*> ALReceiversMap;

public:
	//Constructors
	H223Demuxer();
	~H223Demuxer();
	
	int Open(H223MuxTable *table);
	int SetChannel(int num,H223ALReceiver *receiver);
	int ReleaseChannel(int num);
	void Demultiplex(BYTE b);
	int  Demultiplex(BYTE *buffer,int length);
	int Close();

private:
	void StartPDU(H223Flag &flag);
	void EndPDU(H223Flag &flag);
	void Send(BYTE b);
	int  DecodeHeader(H223Header &header);

private:
	H223MuxTable		*mux;
	H223Flag		begin;
	H223Flag		flag;
	H223Header		header;
	ALReceiversMap		al;
	
	int state;
	int counter;
	int channel;

	Logger *log;
};

#endif

