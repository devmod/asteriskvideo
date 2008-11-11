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
#include "H324MAL2.h"
#include "crc8.h"
#include "FileLogger.h"

/****************** Receiver **************/
H223AL2Receiver::H223AL2Receiver(int segmentable,H223SDUListener* listener,int useSequenceNumbers)
{
	//Set sn parameter
	useSN = useSequenceNumbers;
	//Save listener
	sduListener = listener;
	//Set segmentable
	segmentableChannel = segmentable;
	//Create logger
	logger = new FileLogger();
}

H223AL2Receiver::~H223AL2Receiver()
{
	//Delete logger
	delete logger;
}

void H223AL2Receiver::Send(BYTE b)
{
	//Enque in sdu
	sdu.Push(b);
}

void H223AL2Receiver::SendClosingFlag()
{
	//Check empty
	if	(sdu.Length() == 0)
		return;

	//Crc
	CRC8 crc;

	//Data
	BYTE *data;
	int dataLen;

	//Check minimum size
	if (sdu.Length()<2+useSN)
		goto clean;

	//Get data
	data = sdu.GetPointer();
	dataLen = sdu.Length();

	//Set data
	crc.Add(data,dataLen-1);

	//Dump media
	logger->DumpMediaInput(data+useSN,dataLen-useSN-1);
	
	//Calc
	if (data[dataLen-1]!=crc.Calc())
		goto clean;

	//Send to listener
	sduListener->OnSDU(data+useSN,dataLen-useSN-1);

//Clean SDU and exit
clean:
	sdu.Clean();
}

int H223AL2Receiver::IsSegmentable()
{
	return segmentableChannel;
}

/****************** Sender **************/
H223AL2Sender::H223AL2Sender(int segmentable,int useSequenceNumbers)
	:jitBuf(0,0)
{
	//Set sn parameter
	useSN = useSequenceNumbers;
	sn = 0;
	//Set segmentable flag
	segmentableChannel = segmentable;
	//NO sdu
	pdu = NULL;
	//Set jitter buffer parameters
	minDelay = 0;
	minPackets = 0;
	//Create logger
	logger = new FileLogger();
}

H223AL2Sender::~H223AL2Sender()
{
	//If we have sent anything
	if(pdu)
		//Delete sdu
		delete pdu;
	//Reset queue
	Reset();
	//Delete logger
	delete logger;
}

H223MuxSDU* H223AL2Sender::GetNextPDU()
{
	//Get next element from jitter buffer
	pdu = jitBuf.GetSDU();

	//Send 
	return pdu;
}

void H223AL2Sender::OnPDUCompleted()
{
	//delete frame
	delete pdu;
}

int H223AL2Sender::SendPDU(BYTE *buffer,int len)
{
	//Crc
	CRC8 crc;

	//Build SDU
	H223MuxSDU *sdu = new H223MuxSDU();

	//If we have sn
	if (useSN)
		//Append
		sdu->Push(sn++);

	//Set buffer
	sdu->Push(buffer,len);

	//Calc crc
	crc.Add(sdu->GetPointer(),sdu->Length());

	//Append crc
	sdu->Push(crc.Calc());

	//Dump media
	logger->DumpMediaOutput(buffer,len);

	//Push sdu into jitterBuffer
	jitBuf.Push( sdu );

	//exit
	return true;
}
int H223AL2Sender::Reset()
{
	//Free jitter
	jitBuf.SetBuffer(0,0);
	//Delete the rest of the jitter buffer packets
	while(jitBuf.GetSize())
		//Delete first
		delete jitBuf.GetSDU();
	//Set jitter to previous values
	jitBuf.SetBuffer(minPackets,minDelay);
	//Exit
	return true;
}
int H223AL2Sender::IsSegmentable()
{
	//Return if the channel is segmentable or not
	return segmentableChannel;
}

void H223AL2Sender::SetJitBuffer(int packets,int delay)
{
	//Save values
	minDelay = delay;
	minPackets = packets;
	//Set the jitter buffer parameters
	jitBuf.SetBuffer(packets,delay);
}

void H223AL2Sender::Tick(DWORD len)
{
	//Set jitter tick
	jitBuf.Tick(len);
}



