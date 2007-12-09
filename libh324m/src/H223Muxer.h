#ifndef _H223MUXER_H_
#define _H223MUXER_H_

#include "H324MConfig.h"
#include "H223MuxTable.h"
#include "H223MuxSDU.h"
#include "H223AL.h"
#include "log.h"

class H223Muxer
{
private:
	typedef map<int,H223ALSender*> ALSendersMap;
	typedef enum{NONE,PDU} State;
public:
	//Constructors
	H223Muxer();
	~H223Muxer();

	int Open(H223MuxTable *table);
	int SetChannel(int num,H223ALSender *sender);
	int ReleaseChannel(int num);
	int  Multiplex(BYTE *buffer,int length);
	BYTE Multiplex();
	int Close();

private:
	int GetBestMC(int max);

private:
	H223MuxTable* table;
	H223MuxSDUMap sdus;
	ALSendersMap  senders;
	State state;

	char buffer[5];
	int mc;
	int mpl;
	int pm;
	int i;
	int j;
	int size;
	int len;
	int channel;

	Logger *log;

};

#endif

