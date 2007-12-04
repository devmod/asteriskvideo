#ifndef _H223MUXTABLE_H_
#define _H223MUXTABLE_H_
#include "H245.h"

#include <list>

typedef std::list<int> H223MuxTableEntryList;

struct H223MuxTableEntry
{
	H223MuxTableEntry();
	H223MuxTableEntry(const char* f,const char *r);
	H223MuxTableEntry(H223MuxTableEntry* entry);
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
	int SetEntry(int mc,H223MuxTableEntry *entry);
	int GetChannel(int mc,int count);
	void BuildPDU(H245_MultiplexEntrySend & pdu);
	int AppendEntries(H223MuxTable &table,H223MuxTableEntryList &list);
protected:
	H223MuxTableEntry*	entries[16];
};

#endif

