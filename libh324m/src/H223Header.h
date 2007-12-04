#ifndef _H223HEADER_H_
#define _H223HEADER_H_
#include "H324MConfig.h"

class H223Header
{
public:
	BYTE Append(BYTE b);
	int  IsComplete();
	int  IsValid();
	void Clear();
public:
	BYTE	mc;
	BYTE	pm;
	BYTE	mpl;
private:
	int		level;
	BYTE	buffer[4];
	int		length;
};

#endif

