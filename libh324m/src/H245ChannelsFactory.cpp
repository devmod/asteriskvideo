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
#include "H245ChannelsFactory.h"

H245ChannelsFactory::H245ChannelsFactory()
{
	//Set local capabilities only with layer 2
	local.audioWithAL2 = true;
	local.videoWithAL2 = true;

	//No media channels
	numChannels = 0;
}

H245ChannelsFactory::~H245ChannelsFactory()
{
}

int H245ChannelsFactory::Init(H223ALSender* controlSender,H223ALReceiver* controlReceiver,H245ChannelsFactoryListener *listener)
{
	//Save listener
	this->listener = listener;

	//Set local table with control channel
	localTable.SetEntry(0,"","0");
	
	//Set remote table with control channel
	remoteTable.SetEntry(0,"","0");

	//Set control channel
	demuxer.SetChannel(0,controlReceiver);
	muxer.SetChannel(0,controlSender);

	//Open demuxer
	demuxer.Open(&remoteTable);

	//Open muxer
	muxer.Open(&localTable);

	//OK
	return 1;
}

int H245ChannelsFactory::Reset()
{
	//Loop throught channels
	for (ChannelMap::iterator it = channels.begin(); it != channels.end(); it++)
	{
		//Get channel
		H324MMediaChannel *channel = it->second;
		//Reset it
		channel->Reset();
	}
	//Exit
	return 1;
}

int H245ChannelsFactory::End()
{
	return 1;
}

int H245ChannelsFactory::Demultiplex(BYTE *buffer,int length)
{
	//DeMux
	return demuxer.Demultiplex(buffer,length);
}

int H245ChannelsFactory::Multiplex(BYTE *buffer,int length)
{
	//Mux
	int ret = muxer.Multiplex(buffer,length);
	//For all media channels
	for(ChannelMap::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		//Get channel
		H324MMediaChannel *chan = it->second;
		//Check for null
		if (chan)
			//Set time ticks
			chan->Tick(length);
	}
	return ret;
}

int H245ChannelsFactory::CreateChannel(MediaType type)
{
	H324MMediaChannel* chan;

	//Dependin on channel type
	switch(type)
	{
		case e_Audio:
			//New audio channel
			chan = new H324MAudioChannel(25,160);
			break;
		case e_Video:
			//New audio channel
			chan = new H324MVideoChannel();
			break;
		default:
			return -1;
	}

	//Append to map
	channels[++numChannels] = chan;

	//Set  logical channel
	chan->localChannel = numChannels;

	//Create rep part of entry
	char rep[4];
	sprintf(rep,"%d",numChannels);
	//Add to local table
	localTable.SetEntry(numChannels,"",rep);

	//return channel id
	return numChannels;
}

H223ALSender* H245ChannelsFactory::GetSender(int id)
{
	//If it exist
	if (channels.find(id)==channels.end())
		return NULL;
	//Return sender
	return channels[id]->GetSender();
}

H223ALReceiver* H245ChannelsFactory::GetReceiver(int id)
{
	//If it exist
	if (channels.find(id)==channels.end())
		return NULL;
	//Return receiver
	return channels[id]->GetReceiver();
}

H223MuxTable* H245ChannelsFactory::GetLocalTable()
{
	return &localTable;
}

H223MuxTable* H245ChannelsFactory::GetRemoteTable()
{
	return &remoteTable;
}

int H245ChannelsFactory::SetRemoteTable(H223MuxTable* table)
{
	return true;
}


H245Capabilities* H245ChannelsFactory::GetLocalCapabilities()
{
	return &local;
}
H245Capabilities* H245ChannelsFactory::GetRemoteCapabilities()
{
	return &remote;
}

int H245ChannelsFactory::SetRemoteCapabilities(H245Capabilities* remoteCapabilities)
{
	//Save capabilites
	remote.audioWithAL1 = remoteCapabilities->audioWithAL1;
	remote.audioWithAL2 = remoteCapabilities->audioWithAL2;
	remote.audioWithAL3 = remoteCapabilities->audioWithAL3;
	remote.audioWithAL1 = remoteCapabilities->audioWithAL1;
	remote.audioWithAL2 = remoteCapabilities->audioWithAL2;
	remote.audioWithAL3 = remoteCapabilities->audioWithAL2;
	remote.h263Cap	= remoteCapabilities->h263Cap;
	remote.amrCap	= remoteCapabilities->amrCap;
	remote.g723Cap	= remoteCapabilities->g723Cap;
	remote.inputCap	= remoteCapabilities->inputCap;

	//Set layers for channels
	for(ChannelMap::iterator it = channels.begin(); it != channels.end(); it++)
	{
		//Get channel
		//H324MMediaChannel *chan = it->second;
		
		//Here we should set an H245Channel to a H324MChannel
	}
	
	return 1;
}


int H245ChannelsFactory::OnEstablishIndication(int number, H245Channel *channel)
{
	Logger::Debug("-OnEstablishIndication [%d]\n",number);

	int local = -1;

	//Search local channel for same media type
	ChannelMap::iterator it = channels.begin();

	//While not found
	while (it!=channels.end())
	{
		if (it->second->type == channel->GetType() )
		{
			//We found it 
			local = it->first;
			//Exit
			break;
		}
		//Next channel
		it++;
	}

	//If not found
	if (local==-1)
		//Reject channel
		return 0;

	//Get channel
	H324MMediaChannel * chan = it->second;

	//Asign remote channel
	chan->remoteChannel = number;

	//Set receiving layer
	chan->SetReceiverLayer(channel->GetAdaptationLayer(),channel->IsSegmentable());

	//If the listener was setup
	if(listener)
		//Send event
		listener->OnChannelStablished(chan->localChannel,chan->type);

	//Append to demuxer && accept
	return demuxer.SetChannel(number,chan->GetReceiver());
}

int H245ChannelsFactory::OnEstablishConfirm(int number)
{
	Logger::Debug("-OnEstablishConfirm [%d]\n",number);

	//Search channel 
	ChannelMap::iterator it = channels.find(number);

	//If not found
	if (it==channels.end())
		//exit
		return 0;

	//Get channel
	H324MMediaChannel * chan = it->second;

	//Set sender layer
	//This should be set upon an incomming h245channel from lc
	if (chan->type == e_Audio)
		chan->SetSenderLayer(e_al2WithoutSequenceNumbers,false);
	else
		chan->SetSenderLayer(e_al2WithoutSequenceNumbers,true);

	//If the listener was setup
	if(listener)
		//Send event
		listener->OnChannelStablished(chan->localChannel,chan->type);

	//Set muxer
	return muxer.SetChannel(number,chan->GetSender());
}

int H245ChannelsFactory::OnMuxTableIndication(H223MuxTable &table, H223MuxTableEntryList &list)
{
	//Append entries to table
    return remoteTable.AppendEntries(table,list);
}

int H245ChannelsFactory::OnMuxTableConfirm(H223MuxTableEntryList &list)
{
	return 1;
}

Frame* H245ChannelsFactory::GetFrame()
{
	//Loop throught channels
	for (ChannelMap::iterator it = channels.begin(); it != channels.end(); it++)
	{
		//Get channel
		H324MMediaChannel *channel = it->second;
		
		//If have remote channel
		if (channel->remoteChannel>0)
		{
			//Frame
			Frame* frame = channel->GetFrame();
			
			//If not null
			if (frame)
				//Return frame
				return frame;
		}
	}
	//exit
	return NULL;
}

int H245ChannelsFactory::SendFrame(Frame* frame)
{
	//Loop throught channels
	for (ChannelMap::iterator it = channels.begin(); it != channels.end(); it++)
	{
		//Get channel
		H324MMediaChannel *channel = it->second;
		
		//If have local channel && same type
		if ((channel->type== frame->type) && (channel->localChannel>0))
			//Send frame
			channel->SendFrame(frame);
	}
	//Not send
	return 0;
}

int H245ChannelsFactory::GetRemoteChannel(MediaType type)
{
	//Loop throught channels
	for (ChannelMap::iterator it = channels.begin(); it != channels.end(); it++)
	{
		//Get channel
		H324MMediaChannel *channel = it->second;

		//If have remote channel && same type
		if (channel->type==type)
			//Return number
			return channel->remoteChannel;
	}
	//No channel found
	return 0;
}
