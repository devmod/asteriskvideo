/***************************************************************************
 *   Copyright (C) 2007 by Francesco Emmi   *
 *   francesco.emmi@a-tono.com   *
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
	//Initialize ticks
	ticks	= 0;
	nextPacket = 0;
	//Initialize list
	size	= 0;
	buffer	= 0;
	last	= 0;
	//Set jitter parameters
	SetBuffer(pack,delay);
}

jitterBuffer::~ jitterBuffer()
{
	//Empty queue
	while(size)
	{
		//Get next
		struct env *tmp = buffer;
		//Move to the next
		buffer = buffer->next;
		//Delete element
		free(tmp);
		//Descrease size
		size--;
	}
}

void jitterBuffer::Tick( DWORD len )
{
	//Increase counter
	ticks += len;
}

void jitterBuffer::Push( H223MuxSDU *sdu )
{
	//Create a new Envelop
	struct env *newEl = (struct env *)malloc(sizeof(struct env));

	//Inizialize element
	newEl->sdu = sdu;
	newEl->next = 0;

	//Insert in the list
	if(!size)
	{
		//The first element
		buffer = newEl;
		last = newEl;
	} else {
		//Append to the last one
		last->next = newEl;
		last = last->next;
	}

	//Increase size;
	size++;

	//Check if there are the minimum packets in the queue
	if(wait && size>=minPackets)
		//No more waiting
		wait = false;
}

H223MuxSDU *jitterBuffer::GetSDU()
{
	//If buffer is locked wait for minPackets size
	if(wait)
		//Don't send
		return 0;

	//Loff if we have waited the minimun delay between packets yet
	if(minDelay && nextPacket>ticks)
		//Don't send yet
		return 0;

	//Check size
	if(!size)
		//Don't send
		return 0;

	//Get sdu
	H223MuxSDU *sdu = buffer->sdu;
	
	//Pop front list
	struct env *tmp = buffer;

	//Move to the next
	buffer = buffer->next;

	//Delete element
	free(tmp);
 
	//Descrease size
	size--;

	//If there is delay set
	if(minDelay)
		//Calculate next send time
		//If this is the first packet or buffer has been just unlocked
		if(!nextPacket)
			nextPacket = ticks + minDelay;
		else
			//If buffer is in unlock state
			nextPacket += minDelay;

	//If size now is 0, lock buffer till is reached minPacket size
	if(size == 0)
	{
		wait = true;
		nextPacket = 0;
	}

	//Return the sdu
	return sdu;
}

int jitterBuffer::GetSize()
{
	//Return number of packets in jitter
	return size;
}

void jitterBuffer::SetBuffer(int packets,int delay )
{
	//Set minimun delay and minimun packets in jitter
	minDelay = delay;
	minPackets = packets;
	//We need to wait if there are not enougth packets in the list
	wait = (minPackets>size);
}

