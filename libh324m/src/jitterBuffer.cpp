/***************************************************************************
 *   Copyright (C) 2007 by francesco   *
 *   fremmi@ciccio   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "jitterBuffer.h"

jitterBuffer::jitterBuffer(int pack, int delay)
{
	minPackets = pack;
	minDelay = delay;
	wait = true;
	ticks = 0;
	nextPacket = 0;
	size = 0;
	buffer = last = 0;
}

jitterBuffer::~ jitterBuffer()
{
	
}

void jitterBuffer::Tick( DWORD len )
{
	ticks += len;
}

void jitterBuffer::Push( H223MuxSDU *sdu )
{
	//Create a new Envelop
	struct env *newEl = (struct env *)malloc(sizeof(struct env));
	//Inizialize element
	newEl->sdu = sdu;
	newEl->next = 0;

	if(!size)
	{
		buffer = last = newEl;
	}
	else
	{
		last->next = newEl;
		last = last->next;
	}
	//Increase size;
	size++;
	//Unlock buffer if minPackets are reached
	if(wait && size>=minPackets)
		wait = false;
}

H223MuxSDU *jitterBuffer::GetSDU(void)
{
	//If buffer is locked wait for minPackets size
	if(wait)
		return 0;

	//Look if we have to pop-up an sdu
	if(minDelay && nextPacket>ticks)
	{
		//Don't send yet
		return 0;
	}

	if(!size)
		return 0;

	//Get sdu
	H223MuxSDU *sdu = buffer->sdu;
	
	//Pop front list
	struct env *tmp = buffer;

	buffer = buffer->next;

	free(tmp);
 
	//Descrease size
	size--;

	//Calculate next send
	if(minDelay)
		nextPacket = ticks + minDelay;

	//If size now is 0, lock buffer till is reached minPacket size
	if(size == 0)
		wait = true;

	return sdu;
}

int jitterBuffer::getSize(void)
{
	return size;
}

void jitterBuffer::SetBuffer( int packets, int delay )
{
	minDelay = delay;
	minPackets = packets;
}