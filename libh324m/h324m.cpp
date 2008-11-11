extern "C" 
{
#include "bits.c"
}

#include "src/H324MSession.h"

extern "C" 
{

void H324MLoggerSetLevel(int level)
{
	Logger::SetLevel(level);	
}

void H324mLoggerSetCallback(int (*callback)  (const char *, va_list))
{
	Logger::SetCallback(callback);
}

void * H324MSessionCreate()
{
	return (void *)new H324MSession(); 
}	

void H324MSessionDestroy(void * id)
{
	delete ((H324MSession*)id); 
}	

int  H324MSessionInit(void * id)
{
	return ((H324MSession*)id)->Init(); 
}

int  H324MSessionResetMediaQueue(void * id)
{ 
	return ((H324MSession*)id)->ResetMediaQueue(); 
}

int  H324MSessionEnd(void * id)
{ 
	return ((H324MSession*)id)->End(); 
}

int  H324MSessionRead(void * id,unsigned char *buffer,int len)
{ 
	TIFFReverseBits(buffer,len);
	return ((H324MSession*)id)->Read(buffer,len); 
}

int  H324MSessionWrite(void * id,unsigned char *buffer,int len)
{ 	
	int ret = ((H324MSession*)id)->Write(buffer,len); 
	TIFFReverseBits(buffer,len);
	return ret;
}

void * H324MSessionGetFrame(void * id)
{ 	
	return (void *)((H324MSession*)id)->GetFrame();
}

int  H324MSessionSendFrame(void * id,void *frame)
{ 	
	return ((H324MSession*)id)->SendFrame((Frame*)frame);
}

char * H324MSessionGetUserInput(void * id)
{ 	
	return ((H324MSession*)id)->GetUserInput();
}

int  H324MSessionSendUserInput(void * id,char *input)
{ 	
	return ((H324MSession*)id)->SendUserInput((char*)input);
}

int  H324MSessionSendVideoFastUpdatePicture(void * id)
{ 	
	return ((H324MSession*)id)->SendVideoFastUpdatePicture();
}

int  H324MSessionGetState(void * id)
{ 	
	return ((H324MSession*)id)->GetState();
}

void * FrameCreate(int type, int codec, unsigned char * data, int len)
{
	return (void*)new Frame((MediaType)type,(MediaCodec)codec,data,len);
}

int FrameGetType(void* frame)
{
	return ((Frame*)frame)->type;
}

int FrameGetCodec(void* frame)
{
	return ((Frame*)frame)->codec;
}

unsigned char * FrameGetData(void* frame)
{
	return ((Frame*)frame)->data;
}

unsigned int FrameGetLength(void *frame)
{
	return ((Frame*)frame)->dataLength;
}

void FrameDestroy(void *frame)
{
	delete (Frame*)frame;
}

}
