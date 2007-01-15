#include "Timer.h"
Timer::Data::Data(Handler h,void* d)
	: handler(h), data(d),set(0),when(0)
{
}

void Timer::Data::Set(DWORD t)
{
	//Set flag
	set = 1;
	//Update when
	when = t;
}

void Timer::Data::Reset()
{
	//ReSet flag
	set = 0;
	//Update when
	when = 0;
}

Timer::Timer()
{
	//Set up initial timer
	time = 0;
}

Timer::~Timer()
{
}

DWORD Timer::Tick(int ms)
{
	//Increase time
	time += ms;

	//Check timers
	if (time>=next)
	{
		//Set next send time
		next = (DWORD)-1;

		//For each timer
		for(ListTimers::iterator it=lstTimers.begin(); it!=lstTimers.end(); it++)
		{
			//Get data pointer
			Data *d = (*it);

			//If it's not set
			if (!d->set)
				continue;

			//If it's time for trigger
			if (d->when<=next)
			{
				//Launch trigger
				d->handler(d->data);
				//Unset
				d->set = 0;
			} else if (d->when<next) {
				//Calc new min time
				next = d->when;
			}
		}
		
	}
	//Return current time
	return time;
}

Timer::Handle Timer::CreateTimer(Handler handler,void *data)
{
	//Create timer
	return (Handle) new Data(handler,data);
}

void Timer::SetTimer(Handle id,DWORD ms)
{
	//Set new timer
	((Data*)id)->Set(time+ms);

	//Reset next time for timer
	next = (DWORD)-1;

	//For each timer
	for(ListTimers::iterator it=lstTimers.begin(); it!=lstTimers.end(); it++)
		//If its new min
		if ((*it)->when<next)
			//Calc new min time
			next = (*it)->when;
}

void Timer::ResetTimer(Handle id)
{
	//Reset
	((Data *)id)->Reset();
	//For each timer
	for(ListTimers::iterator it=lstTimers.begin(); it!=lstTimers.end(); it++)
		//If its new min
		if ((*it)->when<next)
			//Calc new min time
			next = (*it)->when;
}

void Timer::DestroyTimer(Handle id)
{
	delete (Data *)id;
}
