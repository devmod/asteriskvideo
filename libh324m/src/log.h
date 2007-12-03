#ifndef _LOG_H_
#define _LOG_H_

#include "H223Const.h"

class Logger
{
public:
	virtual void SetMuxByte(BYTE b) = 0;
	virtual void SetMuxInfo(const char*info,...) = 0;
	virtual void SetDemuxByte(BYTE b) = 0;
	virtual void SetDemuxInfo(int offset,const char*info,...) = 0;
};

#endif
