#ifndef _H223MUXTABLE_H_
#define _H223MUXTABLE_H_
#include "H223Const.h"
#include "H245.h"

struct H223MuxTableEntry
{
	H223MuxTableEntry(const char* f,const char *r);
	~H223MuxTableEntry();

	BYTE* fixed;
	int	  fixedLen;
	BYTE* repeat;
	int   repeatLen;
};


class H223MuxTable
{
public:
	H223MuxTable();
	H223MuxTable(const H245_MultiplexEntrySend & pdu);
	~H223MuxTable();
	int IsSet(int mc);
	int SetEntry(int mc,const char* f,const char *r);
	int GetChannel(int mc,int count);
	void BuildPDU(H245_MultiplexEntrySend & pdu);
	void SetEntry(int mc,H245_MultiplexElement);
private:
	H223MuxTableEntry*	entries[16];
};

#endif

