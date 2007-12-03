#ifndef _TIMER_H_
#define _TIMER_H_

#include "H223Const.h"
#include <list>
using namespace std;

class Timer
{
public:
	typedef void*(*Handler)(void*);
	typedef void* Handle;
private:
	struct Data
	{
		Handler handler;
		void *	data;
		bool	set;
		DWORD	when;

		Data(Handler h,void* d);		
		void Set(DWORD t);
		void Reset();
	};
	
	typedef list<Data*> ListTimers;
public:
	Timer();
	virtual ~Timer();

	DWORD Tick(int ms);

	Handle CreateTimer(Handler handler,void *data);
	void SetTimer(Handle id,DWORD ms);
	void ResetTimer(Handle id);
	void DestroyTimer(Handle id);

private:
	DWORD time;
	DWORD next;
	ListTimers lstTimers;
};


#endif
