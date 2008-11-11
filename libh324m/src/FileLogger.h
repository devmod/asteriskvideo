#ifndef _FILELOGGER_H_
#define _FILELOGGER_H_

#include "log.h"
#define linesize 104

class FileLogger : 
	public Logger
{
public:
	//Constructor
	FileLogger();
	virtual ~FileLogger();

	//Logger interface
	virtual void SetMuxByte(BYTE b);
	virtual void SetMuxInfo(const char*info,...);
	virtual void SetDemuxByte(BYTE b);
	virtual void SetDemuxInfo(int offset,const char*info,...);
	virtual void DumpMediaInput(BYTE *data,DWORD len);
	virtual void DumpMediaOutput(BYTE *data,DWORD len);
	virtual void DumpInput(BYTE *data,DWORD len);
	virtual void DumpOutput(BYTE *data,DWORD len);
private:
	char line1[linesize];
	char line2[linesize];
	char line3[linesize];
	char *l1;
	char *l2;
	int num;
};
#endif
