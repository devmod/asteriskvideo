#ifndef _H223Demuxer_H_
#define _H223Demuxer_H_

#include "H223Const.h"
#include "H223AL.h"
#include "H223MuxTable.h"
#include "H223Flag.h"
#include "H223Header.h"

#include <map>
using namespace std;

class H223Demuxer
{
public:
	//Constructors
	H223Demuxer();
	~H223Demuxer();
	
	int Open(H223MuxTable *table);
	int SetChannel(int num,H223ALReceiver *receiver);
	void Demultiplex(BYTE b);
	int Close();

private:
	void StartPDU(H223Flag &flag);
	void EndPDU(H223Flag &flag);
	void Send(BYTE b);
	int  DecodeHeader(H223Header &header);

private:
	H223MuxTable		*mux;
	H223Flag			begin;
	H223Flag			flag;
	H223Header			header;

	//Al channel users
	map<int,H223ALReceiver*>	al;
	
	int state;
	int counter;
	int channel;
};

#endif

