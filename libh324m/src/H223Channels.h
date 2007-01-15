#ifndef _H223CHANNELS_H_
#define _H223CHANNELS_H_
#include "H223Const.h"

class H223InputChannel
{
public:
	virtual int ReadIndication() = 0;
	virtual BYTE Read() = 0;
	virtual int Read(BYTE *buffer,int len) = 0;
	virtual int Eof() = 0;
};


class H223OutputChannel
{
public:
	virtual int WriteIndication() = 0;
	virtual BYTE Write(BYTE buffer) = 0;
	virtual BYTE Write(BYTE *buffer,int len) = 0;
	virtual int Eof() = 0;
};


class H223FileChannel:
	public H223InputChannel,
	public H223OutputChannel
{
public:
	int Open(const char *);
	int Close();
	virtual int ReadIndication();
	virtual BYTE Read();
	virtual int Read(BYTE *buffer,int len);
	virtual int Eof();
	virtual int WriteIndication();
	virtual BYTE Write(BYTE buffer);
	virtual BYTE Write(BYTE *buffer,int len);
public:
	HANDLE fd;
};

#endif