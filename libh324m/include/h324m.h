#ifndef _H324M_H_
#define _H324M_H_

#ifdef __cplusplus
extern "C"
{
#endif

void* H324MSessionCreate();
void  H324MSessionDestroy(void * id);

int  H324MSessionInit(void * id);
int  H324MSessionEnd(void * id);

int  H324MSessionRead(void * id,unsigned char *buffer,int len);
int  H324MSessionWrite(void * id,unsigned char *buffer,int len);

#ifdef __cplusplus    
}
#endif

#endif
