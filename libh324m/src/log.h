#ifndef _LOG_H_
#define _LOG_H_

#include "H324MConfig.h"

class Logger
{
public:
	virtual void SetMuxByte(BYTE b) = 0;
	virtual void SetMuxInfo(const char*info,...) = 0;
	virtual void SetDemuxByte(BYTE b) = 0;
	virtual void SetDemuxInfo(int offset,const char*info,...) = 0;
	virtual void DumpMediaInput(BYTE *data,DWORD len)=0;
	virtual void DumpMediaOutput(BYTE *data,DWORD len)=0;
	virtual void DumpInput(BYTE *data,DWORD len)=0;
	virtual void DumpOutput(BYTE *data,DWORD len)=0;
	virtual ~Logger() {}

	static void Debug(const char* msg,...);
	static void Warning(const char* msg,...);
	static void Log(const char* msg,...);
	static void Error(const char* msg,...);
	static void SetLevel(int level);
	static void SetCallback(int (*callback)  (const char *, va_list));
protected:
	static int level;
};

#endif
