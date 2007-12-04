#include <stdarg.h>
#include <stdio.h>
#include "log.h"

int Logger::level = 0;

void Logger::SetLevel(int level)
{
	Logger::level = level;
}

void Logger::Debug(const char* msg,...)
{
	if(level>4)
	{
		va_list ap;
		//Set list
		va_start(ap,msg);
		//Output
		vprintf(msg,ap);
		//End list
		va_end(ap);
	}
}

void Logger::Warning(const char* msg,...)
{
	if(level>3)
	{
		va_list ap;
		//Set list
		va_start(ap,msg);
		//Output
		vprintf(msg,ap);
		//End list
		va_end(ap);
	}
}

void Logger::Log(const char* msg,...)
{
	if(level>2)
	{
		va_list ap;
		//Set list
		va_start(ap,msg);
		//Output
		vprintf(msg,ap);
		//End list
		va_end(ap);
	}
}

void Logger::Error(const char* msg,...)
{
	if(level>1)
	{
		va_list ap;
		//Set list
		va_start(ap,msg);
		//Output
		vprintf(msg,ap);
		//End list
		va_end(ap);
	}
}
