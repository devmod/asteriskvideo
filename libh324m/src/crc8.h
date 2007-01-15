#ifndef _CRC8_H_
#define _CRC8_H_

#include "H223Const.h"

class CRC8
{
public:
	CRC8();
	void Add(BYTE *buffer,int len);
	void Add(BYTE b);
	BYTE Calc();
private:
	BYTE crc;
};
#endif
