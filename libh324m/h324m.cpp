extern "C" 
{
#include "src/bits.h"
}

#include "src/H324MSession.h"

extern "C" 
{

void * H324MSessionCreate()
 	{ return (void *)new H324MSession(); }	
void H324MSessionDestroy(void * id)
 	{ delete ((H324MSession*)id); }	

int  H324MSessionInit(void * id)
 	{ return ((H324MSession*)id)->Init(); }
int  H324MSessionEnd(void * id)
 	{ return ((H324MSession*)id)->End(); }

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

}
