#ifndef _CRC16_H_
#define _CRC16_H_

#include "H324MConfig.h"

class CRC16
{
public:
	CRC16();
	void Add(BYTE *buffer,int len);
	void Add(BYTE b);
	WORD Calc();
private:
	WORD crc;
};
#endif
