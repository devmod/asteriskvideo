/* H324M library
 *
 * Copyright (C) 2006 Sergio Garcia Murillo
 *
 * sergio.garcia@fontventa.com
 * http://sip.fontventa.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "FileLogger.h"
#include <stdarg.h>

FileLogger::FileLogger()
{
	num = 0;
	l1 = line1+8;
	l2 = line2;
	memset(line3,' ',linesize);
}

FileLogger::~FileLogger()
{
}

#ifdef DUMP_H223 
void FileLogger::SetMuxByte(BYTE b)
{
	//New line
	if (num % 32 == 0)
	{
		if (num>0)
		{
			char name[256];
			sprintf(name,"/tmp/h245_out_%x.log",(unsigned int)this);
			int fd = open(name,O_CREAT|O_WRONLY|O_APPEND);
			write(fd,line1,linesize);
			write(fd,"\r\n",2);
			write(fd,line3,linesize);
			write(fd,"\r\n",2);
			close(fd);
		} 
		sprintf(line1,"%.8X ",num);
		l1 = line1+8;
	}

	int numchar = sprintf(l1," %.2X",b);
	l1[numchar] = ' ';
	l1+=3;
	num++;
}

void FileLogger::SetMuxInfo(const char*info,...)
{
	va_list ap;

	//Set list
	va_start(ap,info);

	//Set line info
	int numchar = vsprintf(l2,info,ap);

	//Remove \0
	l2[numchar]=' ';

	//Move l2
	l2 += numchar;

	//Calculate the remining
	int rest = l2-line2-32*3;

	//if we have past the end of line
	if (rest>=0)
	{
		//Set to blank
		memset(line3,' ',linesize);
		//Copy to line3
		memcpy(line3+8,line2,32*3);
		//Move end of line2 to begining
		memcpy(line2,line2+32*3,rest);
		//Move to the begining
		l2 = line2+rest;
	}
}

void FileLogger::SetDemuxByte(BYTE b)
{
	//New line
	if (num % 32 == 0)
	{
		if (num>0)
		{
			char name[256];
			sprintf(name,"/tmp/h245_%x.log",(unsigned int)this);
			int fd = open(name,O_CREAT|O_WRONLY|O_APPEND);
			write(fd,line3,linesize-6);
			if (line3[linesize-5]==' ') {
				write(fd,line2+2,6);
			} else if (line3[linesize-3]!=' ') {
				write(fd,line3+linesize-6,3);
				write(fd,line2+5,3);
			} else {
				write(fd,line3+linesize-6,6);
			}
			memset(line2,' ',8);
			write(fd,"\r\n",2);
			write(fd,line1,linesize);
			write(fd,"\r\n",2);
			memcpy(line3,line2,linesize);
			close(fd);
		} else
			memset(line3,' ',linesize);
		sprintf(line1,"%.8X ",num);
		memset(line2,' ',linesize);
		l1 = line1+8;
		l2 = line2+8;

	}
	int numchar = sprintf(l1," %.2X",b);
	l1[numchar] = ' ';
	numchar = sprintf(l2,"   ");
	l2[numchar] = ' ';
	l1+=3;
	l2+=3;
	num++;
}

void FileLogger::SetDemuxInfo(int offset,const char*info,...)
{
	va_list ap;

	//Set list
	va_start(ap,info);

	//Set line info
	int numchar = vsprintf(l2+offset,info,ap);

	//Remove \0
	l2[numchar+offset]=' ';
}
#else

void FileLogger::SetMuxByte(BYTE b)
{
}

void FileLogger::SetMuxInfo(const char*info,...)
{
}

void FileLogger::SetDemuxByte(BYTE b)
{
}

void FileLogger::SetDemuxInfo(int offset,const char*info,...)
{
}
#endif
