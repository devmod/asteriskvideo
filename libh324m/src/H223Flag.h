#ifndef _H223FLAG_H_
#define _H223FLAG_H_
#include "H324MConfig.h"

class H223Flag
{
public:
	BYTE Append(BYTE b);
	int  IsComplete();
	int  IsValid();
	void Clear();
	int		complement;
private:
	int		level;
	BYTE	buffer[2];
	int		length;
	
};

#endif

