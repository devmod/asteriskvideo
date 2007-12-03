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
#include "H324MAL1.h"


/****************** Receiver **************/
H223AL1Receiver::H223AL1Receiver(int segmentable,H223SDUListener* listener)
{
	//Save listener
	sduListener = listener;
	//Set segmentable
	segmentableChannel = segmentable;
}

H223AL1Receiver::~H223AL1Receiver()
{
}

void H223AL1Receiver::Send(BYTE b)
{
}

void H223AL1Receiver::SendClosingFlag()
{
}

int H223AL1Receiver::IsSegmentable()
{
	return segmentableChannel;
}
/****************** Sender **************/
H223AL1Sender::H223AL1Sender(int segmentable)
{
	//Set segmentable flag
	segmentableChannel = segmentable;
}

H223AL1Sender::~H223AL1Sender()
{
	//Clean sdus pending
	while(frameList.size()>0)
	{
		//Delete front
		delete frameList.front();
		//Remove
		frameList.pop_front();
	}
}

H223MuxSDU* H223AL1Sender::GetNextPDU()
{
	return NULL;
}

void H223AL1Sender::OnPDUCompleted()
{
}

int H223AL1Sender::IsSegmentable()
{
	return segmentableChannel;
}

