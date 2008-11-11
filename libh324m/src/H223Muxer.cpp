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
#include "FileLogger.h"

extern "C"
{
#include "golay.h"
}

H223Muxer::H223Muxer()
{
	//Create logger
	log = new FileLogger();
}

H223Muxer::~H223Muxer()
{
	//Delete logger
	delete log;
}

int H223Muxer::Open(H223MuxTable *muxTable)
{
	//Check for null table
	if (!muxTable)
		//Error
		return 0;

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


int H223Muxer::SetChannel(int num,H223ALSender *sender)
{
	//Check for null channel
	if (!sender)
		//Error
		return 0;

	//If the channel already has a sender
	if (senders.find(num)!=senders.end())
		return 0;

	//Add it to the list
	senders[num] = sender;

	//Good
	return 1;
}

int H223Muxer::ReleaseChannel(int num)
{
	// Find channel
	ALSendersMap::iterator it = senders.find(num);

	// If not found
	if (it==senders.end())
		//Error
		return 0;

	// Remove channel from map
	senders.erase(it);

	// Exit
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
		while(j<max)
		{
			//Get next channel for table
			int c = table->GetChannel(i,j);

			//If we don't have a mux for that channel
			if (c==-1 || sduLen[c]==0)
				break;

			 //If channel is non-segmentble and it was already ended
			if (!senders[c]->IsSegmentable() && (len[c] == sduLen[c]))
				//Exit
				break;
			
			//Increase counter
			len[c]++;
			j++;
			
			//If channel is segmentble and it has ended
			if (senders[c]->IsSegmentable() && (len[c] == sduLen[c]))
			{
				//Finish sdu
				end = 1;
				//Exit
				break;
			}

		}

		//Calculate ratio
		float ratio = 0;

		//For each sdu
		for (int k=0;k<256;k++)
			if (len[k]>0)
				ratio +=  (float)len[k]/sduLen[k];

		//If the ratio is better
		if (ratio>best)
		{
			//Save values
			mc = i;
			mpl = j;
			pm = end;
			best = ratio;
		}
	}

	//If we found something
	return mc!=-1;
}

int H223Muxer::Multiplex(BYTE *buffer,int length)
{
	//Mux
	for (int i=0;i<length;i++)
		buffer[i] = Multiplex();

	//Ok
	return 1;
}

inline BYTE H223Muxer::Multiplex()
{
	//Multiplex
	while (1)
	{
		//Depending on the state
		switch(state)
		{
			case NONE:
				//Reset channel
				channel = -1;
				//If we have to finish last packet
				if (!pm)
				{
					//Create the flag
					buffer[0] = 0xE1;
					buffer[1] = 0x4D;

					//Log
					log->SetMuxInfo("endflg");
				} else {
					//Create the flag
					buffer[0] = (BYTE)~0xE1;
					buffer[1] = (BYTE)~0x4D;
					//Log
					log->SetMuxInfo("dneflg");
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
					//Log
					log->SetMuxInfo("   mc%.1d %.2x",mc,mpl);
				} else {
					//Create the header
					buffer[2] = 0x00;
					buffer[3] = 0x00;
					buffer[4] = 0x00;
					//Log
					log->SetMuxInfo("         ");
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
				{
					//Log
					log->SetMuxByte(buffer[i]);
					//Return header byte
					return buffer[i++];
				}

				//If we haven't finished
				if (j<mpl)
				{
					//Next channel byte
					channel = table->GetChannel(mc,j++);
					//Get byte
					BYTE b = sdus[channel]->Pop();
					//Log
					log->SetMuxByte(b);
					log->SetMuxInfo(" c%.1d",channel);
					//Send the byte
					return b;
				}
				//Remove all empty sdus
				{
					H223MuxSDUMap::iterator it = sdus.begin();
					while(it!=sdus.end())
					{
						//Get channel and sdu
						int number		= it->first;
						H223MuxSDU* sdu = it->second;
						//If it's empty
						if ((sdu!=NULL) && (sdu->Length()==0))
						{
							//Erase
							sdus.erase(it++);
							//Set event
							senders[number]->OnPDUCompleted();
						} else
							++it;
					}
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
