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
#include "H223Muxer.h"
extern "C"
{
#include "golay.h"
}

#define Debug printf

H223Muxer::H223Muxer()
{
}

H223Muxer::~H223Muxer()
{
}

int H223Muxer::Open(H223MuxTable *muxTable)
{
	//Save the mux table
	table = muxTable;

	//first state
	state = NONE;
	pm = 0;

	return true;
}


int H223Muxer::Close()
{
	return 1;
}


int H223Muxer::SetChannel(int channel,H223ALSender *sender)
{
	//If the channel already has a sender
	if (senders.find(channel)!=senders.end())
		return 0;

	//Add it to the list
	senders[channel] = sender;

	//Good
	return 1;
}
/**********************************
* GetBestMC
*	Search the best mc entry and calculate de mpl of the pdu
*   Currently best result is the one that
***********************************/
int H223Muxer::GetBestMC(int max)
{
	H223MuxSDUMap::iterator itSDUS;
	ALSendersMap::iterator itSenders;
	//Reset
	mc = -1;
	mpl = 0;
	pm = 0;

	//best ratio
	float best = 0;

	//Check for new request
	for (ALSendersMap::iterator itSenders=senders.begin();itSenders!= senders.end();itSenders++)
	{
		//Get channel number
		int channel = itSenders->first;

		//Does it has a pdu? and its not null??
		if(itSenders->second && sdus.find(channel)==sdus.end())
		{
			//Request new
			H223MuxSDU *sdu = itSenders->second->GetNextPDU();

			//If it's not null
			if (sdu!=NULL)
				//Append
				sdus[channel] = sdu;
		}
	}
	
	//Map and iterators
	WORD len[256];
	WORD sduLen[256];

	//Reset length
	memset(sduLen,0,256*sizeof(WORD));

	//For each sdu
	for (itSDUS=sdus.begin();itSDUS!=sdus.end();itSDUS++)
		sduLen[itSDUS->first] = itSDUS->second->Length();
	
	//For each table
	for (int i=0;i<16;i++)
	{
		int j = 0;
		int end = 0;

		//Reset lengths
		memset(len,0,256*sizeof(WORD));

		//While not done
		while((j<max) && (!end))
		{
			//Get next channel for table
			int c = table->GetChannel(i,j++);

			//If we don't have a mux for that channel
			if (c==-1 || sduLen[c]==0)
				break;

			//Increase counter
			len[c]++;

			//Do we have more on this channel?
			if(len[c] == sduLen[c])
			{
				//Calculate ratio
				float ratio = 0;
				//For each sdu
				for (int k=0;k<256;k++)
					if (len[k]>0)
						ratio +=  (float)sduLen[k]/len[k];

				//If the ratio is better
				if (ratio>best)
				{
					//Save values
					mc = c;
					mpl = j;
					pm = 1;
					best = ratio;
					//Next entry
					end = 1;
				}
			}
		}
		//If max reached and not found an mc yet
		if ((!end) && (j>0))
		{
		}
	}

	//If we found something
	return mc!=-1;
}

BYTE H223Muxer::Multiplex()
{
	//Multiplex
	while (1)
	{
		//Depending on the state
		switch(state)
		{
			case NONE:
				//If we have to finish last packet
				if (!pm)
				{
					//Create the flag
					buffer[0] = 0xE1;
					buffer[1] = 0x4D;
				} else {
					//Create the flag
					buffer[0] = ~0xE1;
					buffer[1] = ~0x4D;
				}

				//Get the best mc & mpl from the table
				if (GetBestMC(160))
				{
					//Calculate p bits
					WORD data = (mc & 0x0F) | mpl << 4;
					//Get the codeword
					long code = golay_encode(data);

					//Create the header
					buffer[2] = ((BYTE *)&code)[0];//(mc & 0x0F) | (mpl &0x0F) << 4;
					buffer[3] = ((BYTE *)&code)[1];//(mpl &0xF0) >> 4;
					buffer[4] = ((BYTE *)&code)[2];//0x00;
				} else {
					//Create the header
					buffer[2] = 0x00;
					buffer[3] = 0x00;
					buffer[4] = 0x00;
				}
				//Set pointers
				i = 0;
				j = 0;
				size = 5;
				//Send pdu
				state = PDU;
				break;
			case PDU:
				//If we still haven't sent the flag & header
				if (i<size)
					//Return header byte
					return buffer[i++];

				//If we haven't finished
				if (j<mpl)
				{
					//Next channel byte
					channel = table->GetChannel(mc,j++);
					//Send the byte
					return sdus[channel]->Pop();
				}
				//If we hav finished the sdu
				if (pm)
				{
					//Remove the last sdu from the channel
					sdus.erase(channel);
					//Set the event
					senders[channel]->OnPDUCompleted();
				}

				//No state
				state = NONE;
				break;
			default:
				return 0;
		}
	}

	return 0;
}
