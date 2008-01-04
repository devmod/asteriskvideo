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
#include <string.h>
#include "H223Demuxer.h"
#include "FileLogger.h"
#define NONE  0
#define HEAD  1
#define PDU   2

H223Demuxer::H223Demuxer()
{
	//Create logger
	log = new FileLogger();
}

H223Demuxer::~H223Demuxer()
{
	//Delete logger
	delete log;
}

int H223Demuxer::SetChannel(int num,H223ALReceiver *receiver)
{
	//Check for null channel
	if (!receiver)
		//Error
		return 0;

	//Save the reciever
	al[num] = receiver;

	return 1;
}

int H223Demuxer::ReleaseChannel(int num)
{
	// Search channel
	ALReceiversMap::iterator it = al.find(num);

	// If not found
	if (it==al.end())
		//Error
		return 0;

	// Delete from map
	al.erase(it);

	// Exit
	return 1;
}

int H223Demuxer::Open(H223MuxTable *table)
{
	//Check for null table
	if (!table)
		//Error
		return 0;

	//The table
	mux = table;

	//Reset state
	state= NONE;

	//Reset flags
	flag.Clear();
	begin.Clear();

	//And header
	header.Clear();

	return true;
}

int H223Demuxer::Close()
{
	//exit
	return 1;
}

void H223Demuxer::StartPDU(H223Flag &flag)
{
	//Copy the flag
	begin = flag;
	//Reset counter
	counter = 0;
	//No channel
	channel = -1;
	//Log
	log->SetDemuxInfo(-3,"flg");
}

void H223Demuxer::EndPDU(H223Flag &flag)
{

	//Send closing flag to all non segmentable channels
	for(ALReceiversMap::iterator it = al.begin(); it != al.end(); it++)
	{
		//Get channel
		H223ALReceiver *recv = it->second;
		//If it's non seg
		if(recv && !recv->IsSegmentable())
			//Send closing flag
			recv->SendClosingFlag();
	}
	
	//If the flag is the complement and valid
	if (flag.IsValid() && flag.complement)
	{
		//Log
		log->SetDemuxInfo(-6,"dne");
		
		//if there is channel
		if ((channel!=-1) && (al[channel]!=NULL))
			//Send the closing pdu to the last channel
			al[channel]->SendClosingFlag();
	} else {
		//Log
		log->SetDemuxInfo(-6,"end");
		//if there is non-segmentable channel
		if ((channel!=-1) && (al[channel]!=NULL) && !al[channel]->IsSegmentable())
			//Send the closing pdu to the last channel
			al[channel]->SendClosingFlag();
	}
		
}

int H223Demuxer::Demultiplex(BYTE *buffer,int length)
{
	//DeMux
	for (int i=0;i<length;i++)
		Demultiplex(buffer[i]);

	//Ok
	return 1;
}


inline void H223Demuxer::Demultiplex(BYTE b)
{
	//Append to logger
	log->SetDemuxByte(b);

	//Depending on the state
	switch(state)
	{
		case NONE:
			//Append the byte to the flag
			flag.Append(b);
			
			//If we have a full header
			if (!flag.IsValid())
				return;
	
			//Start the PDU
			StartPDU(flag);

			//Clear flag
			flag.Clear();

			//Header clear
			header.Clear();

			//Change state
			state = HEAD;
			
			break;
		case HEAD:
			//Append the byte to the header
			header.Append(b);

			//If still don't we have a complete header
			if (!header.IsComplete())
				return;
	
			//Is the header correct? And it's not stuffing ?
			if (!header.IsValid() || !header.mpl)
			{
				//Reset state
				state = NONE;
				//Exit
				return;
			}

			//Log header
			log->SetDemuxInfo(-6,"mc%.1dl%.2x",header.mc,header.mpl);

			//We have a good header go for the pdu
            		state = PDU;

			break;
		case PDU:
			//Check if the buffer of the flag is full or not
			int complete = flag.IsComplete();

			//Append the byte to the flag and get previous one
			BYTE  a = flag.Append(b);

			//Send the byte to the corresponding AL
			if (complete)
				Send(a);

			//While we are in the PDU
			if (counter<header.mpl)
				//And return
				return;

			//End the pdu
			EndPDU(flag);

			//If the flag is valid
			if (flag.IsValid())
			{
				//Start the next PDU
				StartPDU(flag);

				//Clear flag
				flag.Clear();

				//Clear the header 
				header.Clear();

				//Change state
				state = HEAD;
			} else 
				//No header found
				state = NONE;
			
			break;
	}
}


void H223Demuxer::Send(BYTE b)
{
	//Log
	log->SetDemuxInfo(-9," xx");
	
	//Get the next channel from the mux table
	channel = mux->GetChannel(header.mc,counter++);

	//Check channel
	if ((channel<0) || (channel>15))
		//Exit
		return;

	//Log
	log->SetDemuxInfo(-9," n%.1d",channel);

	//Get channel
	ALReceiversMap::iterator it = al.find(channel);

	//If not found 
	if (it==al.end())
		return;

	//Log
	log->SetDemuxInfo(-9," c%.1d",channel);

	//Get channel
	H223ALReceiver *recv = it->second;

	//If it's not null
	if (recv)
		//Send byte
		recv->Send(b);
}

