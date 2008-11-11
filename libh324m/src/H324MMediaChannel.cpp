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
#include "H324MMediaChannel.h"
#include "log.h"


H324MMediaChannel::H324MMediaChannel(int jitter, int delay)
{
	state = e_AwaitingEstablishment;
	localChannel = 0;
	remoteChannel = 0;
	isBidirectional = 0;
	sender = NULL;
	receiver = NULL;
	jitterPackets = jitter;
	jitterActive = false;
	minDelay = delay;
	nextPacket = 0;
	ticks = 0;
}

H324MMediaChannel::~H324MMediaChannel()
{
}

int H324MMediaChannel::Init()
{
	return 1;
}
int H324MMediaChannel::End()
{
	//Clean
	if (sender)
		delete sender;
	if (receiver)
		delete receiver;

	return 1;
}


H223ALReceiver*  H324MMediaChannel::GetReceiver()
{
	return receiver;
}
H223ALSender* H324MMediaChannel::GetSender()
{
	return sender;
}

int H324MMediaChannel::SetSenderLayer(AdaptationLayer layer, int segmentable)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case e_al1Framed:
		case e_al1NotFramed:
			// AL 1
			sender = new H223AL1Sender(segmentable);
			break;
		case e_al2WithoutSequenceNumbers:
			// AL 2
			sender = new H223AL2Sender(segmentable,false);
			//Set jitterBuffer
			((H223AL2Sender *)sender)->SetJitBuffer(jitterPackets, minDelay);
			break;
		case e_al2WithSequenceNumbers:
			// AL 2
			sender = new H223AL2Sender(segmentable,true);
			break;
		case e_al3:
			// AL3
			sender = new H223AL3Sender(segmentable);
			break;
		default:
			//Not handled
			return 0;
	}

	return 1;
}

int H324MMediaChannel::SetReceiverLayer(AdaptationLayer layer, int segmentable)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case e_al1Framed:
		case e_al1NotFramed:
			// AL 1
			receiver = new H223AL1Receiver(segmentable,this);
			break;
		case e_al2WithoutSequenceNumbers:
			// AL 2
			receiver = new H223AL2Receiver(segmentable,this,false);
			break;
		case e_al2WithSequenceNumbers:
			// AL 2
			receiver = new H223AL2Receiver(segmentable,this,true);
			break;
		case e_al3:
			// AL3
			receiver = new H223AL3Receiver(segmentable,this);
			break;
		default:
			//Not handled
			return 0;
	}
	//Exit
	return 1;
}
void H324MMediaChannel::Tick(DWORD value)
{
	//Increase counter
	ticks += value;
	//If got sender
	if(sender)
		((H223AL2Sender*)sender)->Tick( value);
}

void H324MMediaChannel::Reset()
{
	//If got sender
	if(sender)
		//Reset send queue
		((H223AL2Sender*)sender)->Reset();
}

void H324MMediaChannel::OnSDU(BYTE* data,DWORD length)
{
	MediaCodec codec;
	//Depending on the type
	if (type == e_Audio)
		codec = e_AMR;
	else
		codec = e_H263;
	//Enque new frame
	frameList.push_back(new Frame(type,codec,data,length));
}

Frame* H324MMediaChannel::GetFrame()
{
	//Check size
	if (frameList.size()==0)
	{
		//No packet
		return NULL;
	}
	//Check if sending sending or have enougth packets
	//Get frame
	Frame *frame = frameList.front();
	//Remove
	frameList.pop_front();
	//Return frame
	return frame;
}

int H324MMediaChannel::SendFrame(Frame *frame)
{
	//Debug
	Logger::Debug("-Sending Frame [%d,%d]\n",frame->type,frame->dataLength);

	//Check sender
	if (!sender)
		//Exit
		return 0;

	//Initial sdu length
	DWORD len = 0;
    	DWORD pos = 0;

	//Sen up to max size
	while (pos<frame->dataLength)
	{
		//Calculate length
		if (pos+160>frame->dataLength)
			//Send until the end
			len = frame->dataLength-pos;
		else
			//Send 160
			len = 160;

		//Debug
		Logger::Debug("-Sending PDU [%d,%d,%d]\n",pos,len,frame->dataLength);
		//Send
		((H223AL2Sender*)sender)->SendPDU(frame->data+pos,len);
		//Increase len
		pos += len;
	}

	//Return size
	return pos;
}

H324MAudioChannel::H324MAudioChannel(int jitter,int delay) : H324MMediaChannel(jitter,delay)
{
	//Set audio type
	type = e_Audio;
}

H324MVideoChannel::H324MVideoChannel() : H324MMediaChannel(0,0)
{
	//Set video type
	type = e_Video;
}

