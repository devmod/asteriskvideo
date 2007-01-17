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
#include "H324MSession.h"

#define Debug printf 
int H324MSession::Init(void)
{
	//Only entry 0 at the beginning is available
	tableDemux.SetEntry(0,"","0");
	
	//Init the demuxer
	demuxer.Open(&tableDemux);

	//Add the reciever
	demuxer.SetChannel(0,&controlChannel);

	//Set mcs
	tableMux.SetEntry(0,"","0");

	//Init the muxer
	muxer.Open(&tableMux);
	
	//Set the control channel
	muxer.SetChannel(0,&controlChannel);

	//Initial state
	state = e_Setup;

	//Set  logical channel
	videoChannel.localChannel = 1;

	//Set  logical channel
	audioChannel.localChannel = 2;
	
	//Init video
	videoChannel.Init();

	//Init audio
	audioChannel.Init();

	//Call Setup
	return true;
}
int H324MSession::End()
{
	return true;
}
/*
int H324MSession::OnCallSetup()
{
	H245Capabilities c;

	//Check capabilities
	if (c.audioWithAL1)
		audioChannel.SetReceiverLayer(1);
	else if (c.audioWithAL2) 
		audioChannel.SetReceiverLayer(2);
	else if (c.audioWithAL3) 
		audioChannel.SetReceiverLayer(3);
	else 
		//No audio (should end??)
		audioChannel.SetReceiverLayer(2);//Debug("No audio suported\n");

	//Set sender layer
	audioChannel.SetSenderLayer(2);

	//Check capabilities
	if (c.videoWithAL1)
		videoChannel.SetReceiverLayer(1);
	else if (c.audioWithAL2) 
		videoChannel.SetReceiverLayer(2);
	else if (c.audioWithAL3) 
		videoChannel.SetReceiverLayer(3);
	else 
		//No audio (should end??)
		audioChannel.SetReceiverLayer(2);//Debug("No video suported\n");

	//Video
	videoChannel.SetReceiverLayer(2);
	videoChannel.SetSenderLayer(2);

	//Add to muxer & demuxer
	tableDemux.SetEntry(1,"","2");
	demuxer.SetChannel(2,videoChannel.GetReceiver());
	tableMux.SetEntry(2,"","2");
	muxer.SetChannel(videoChannel.localChannel,videoChannel.GetSender());

	//Save table
	state=e_SetupMedia;

	//Setup Media
	return true;
}

int H324MSession::OnMediaSetup()
{
	//Set demuxing channels
	demuxer.SetChannel(audioChannel.remoteChannel,audioChannel.GetReceiver());
	demuxer.SetChannel(videoChannel.remoteChannel,videoChannel.GetReceiver());
	//Set muxer channel
	muxer.SetChannel(audioChannel.localChannel,audioChannel.GetSender());
	muxer.SetChannel(videoChannel.localChannel,videoChannel.GetSender());

	//We are open!!
	state = e_Stablished;

	//Exit
	return 1;
}*/

int H324MSession::Read(BYTE *buffer,int length)
{
	//DeMux
	for (int i=0;i<length;i++)
		demuxer.Demultiplex(buffer[i]);
	//Ok
	return 1;
}
int H324MSession::Write(BYTE *buffer,int length)
{
	//Mux
	for (int i=0;i<length;i++)
		buffer[i] = muxer.Multiplex();
	//Ok
	return 1;
}

/*
int H324MControlChannel::CallSetup()
{
	//Send our request
	tc->TransferRequest();
	//ms->Request();
	//Exit
	return TRUE;
}

int H324MControlChannel::OnMasterSlaveDetermination(const H245MasterSlave::Event & event)
{
	//Depending on the type
	switch(event.confirm)
	{
		case H245MasterSlave::e_Confirm:
			//Send event
			OnCallSetup();	
			return TRUE;
		case H245MasterSlave::e_Indication:
			return TRUE;
	}

	//Exit
	return FALSE;
}
int H324MControlChannel::OnCapabilityExchange(const H245TerminalCapability::Event & event)
{
	//Depending on the type
	switch(event.type)
	{
		case H245TerminalCapability::e_TransferConfirm:
			return TRUE;
		case H245TerminalCapability::e_TransferIndication:
			//Accept
			tc->TransferResponse();
			//Start master Slave
			ms->Request();
			//Exit
			return TRUE;
	}

	//Exit
	return FALSE;
}


int H324MControlChannel::MediaSetup(H324MMediaChannel *a,H324MMediaChannel *v)
{
	//Save channels
	audio = a;
	video = v;

	//Create channel
	//channels->CreateChannel(audio->localChannel,audio,0);

	//Create channel
	//channels->CreateChannel(video->localChannel,video,0);

	//Opne audio
	//lc->EstablishRequest(video->localChannel);

	

	return true; 
}

int H324MControlChannel::OnMultiplexTable(const H245MuxTable::Event &event)
{
	return true;
}

int H324MControlChannel::OnLogicalChannel(const H245LogicalChannels::Event &event)
{
	Debug("-OnLogicalChannel\n");
	switch(event.type)
	{
		case H245LogicalChannels::e_EstablishIndication:
			lc->EstablishResponse(event.channel,true);
			Debug("OpenLogicalChannel\n");
			//lc->EstablishRequest(video->localChannel);
			return true;
		case H245LogicalChannels::e_EstablishConfirm:
		{
			//Send table
			H223MuxTable table;
			table.SetEntry(1,"","1");
			//Send
			mt->Send(table);
			return true;
		}
		case H245LogicalChannels::e_ReleaseIndication:
		case H245LogicalChannels::e_ReleaseConfirm:
		case H245LogicalChannels::e_ErrorIndication:
			return true;
	}

	//Exit
	return true;
}
*/
