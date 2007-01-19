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
	//Set local capabilities
	local.audioWithAL2 = true;
	local.videoWithAL2 = true;
	local.videoWithAL3 = true;

	//No media channels
	numChannels = 0;

	//Set local table with control channel
	localTable.SetEntry(0,"","0");
	
	//Set remote table with control channel
	remoteTable.SetEntry(0,"","0");
}

H245ChannelsFactory::~H245ChannelsFactory()
{
}

int H245ChannelsFactory::CreateChannel(H324MMediaChannel::e_Type type)
{
	H324MMediaChannel* chan;

	//Dependin on channel type
	switch(type)
	{
		case H324MMediaChannel::Audio:
			//New audio channel
			chan = new H324MAudioChannel();
			break;
		case H324MMediaChannel::Video:
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
		H324MMediaChannel *chan = it->second;

		//IF is audio
		if (chan->type == H324MMediaChannel::Audio)
		{
			//Check capabilities
			if (remote.audioWithAL1)
				chan->SetReceiverLayer(1);
			else if (remote.audioWithAL2) 
				chan->SetReceiverLayer(2);
			else if (remote.audioWithAL3) 
				chan->SetReceiverLayer(3);
			else 
				Debug("No audio suported\n");
			//Set sender layer
			chan->SetSenderLayer(2);

		} else {
			//Check capabilities
			if (remote.videoWithAL1)
				chan->SetReceiverLayer(1);
			else if (remote.audioWithAL2) 
				chan->SetReceiverLayer(2);
			else if (remote.audioWithAL3) 
				chan->SetReceiverLayer(3);
			else 
				//No audio (should end??)
				Debug("No video suported\n");

			//Video
			chan->SetSenderLayer(2);
		}
	}
	
	return 1;
}

int H245ChannelsFactory::BuildChannelPDU(H245_OpenLogicalChannel & open,int number)
{
	
	//Check channel exits
	if (channels.find(number)==channels.end())
		//Exit
		return FALSE;
	
	Debug("-Building PDU for channel [%d,%d]\n",number,channels[number]->type);

	//Depending on the channel type
	if (channels[number]->type==H324MMediaChannel::Video)
	{
		//Set video type
		open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_videoData);
		//Set capabilities
		(H245_VideoCapability &) open.m_forwardLogicalChannelParameters.m_dataType = (H245_VideoCapability&)local.h263Cap.m_capability;
		//Mux Capabilities
		open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h223LogicalChannelParameters);
		//Set h223
		H245_H223LogicalChannelParameters & h223 = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
		h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithoutSequenceNumbers);
		/*H245_H223LogicalChannelParameters_adaptationLayerType_al3 &al3 = h223.m_adaptationLayerType;
		al3.m_controlFieldOctets = 1;
		al3.m_sendBufferSize = 4231;
		h223.m_segmentableFlag = true;*/
	
		return 1;	

		//Set capabilities
		open.m_reverseLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_videoData);
		open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
		(H245_VideoCapability &) open.m_reverseLogicalChannelParameters.m_dataType = (H245_VideoCapability&)local.h263Cap.m_capability;

		//Mux Capabilities
		open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h223LogicalChannelParameters);
		open.m_reverseLogicalChannelParameters.IncludeOptionalField(H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
		//Set h223
		H245_H223LogicalChannelParameters & rh223 = open.m_reverseLogicalChannelParameters.m_multiplexParameters;
		rh223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al3);
		H245_H223LogicalChannelParameters_adaptationLayerType_al3 &ral3 = rh223.m_adaptationLayerType;
		ral3.m_controlFieldOctets = 1;
		ral3.m_sendBufferSize = 4231;
		rh223.m_segmentableFlag = true;

	} else {
		//Set audio type
		open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_audioData);
		//Set capabilities
		(H245_AudioCapability &) open.m_forwardLogicalChannelParameters.m_dataType = (H245_AudioCapability&)local.g723Cap.m_capability;
		//Mux Capabilities
		open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h223LogicalChannelParameters);
		//Set h223
		H245_H223LogicalChannelParameters & h223 = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
		h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithSequenceNumbers);
		h223.m_segmentableFlag = false;
	}

	return 1;
}
