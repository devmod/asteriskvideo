#ifndef _H223MUXSDU_H_
#define _H223MUXSDU_H_

#include "H223Const.h"

#include <list>
#include <map>
using namespace std;

class H223MuxSDU
{
public:
	H223MuxSDU();
	H223MuxSDU(BYTE *b,int len);
	~H223MuxSDU();
	
	int	 Push(BYTE b);
	int  Push(BYTE *b,int len);
	BYTE Pop();
	BYTE *GetPointer() {return buffer;}
	int	 Length();

	void Begin();
	void Clean();

private:
	BYTE *buffer;
	int ini;
	int end;
	int size;
};

typedef map<int,H223MuxSDU*> H223MuxSDUMap;
typedef list<H223MuxSDU*> H223MuxSDUList;

#endif

