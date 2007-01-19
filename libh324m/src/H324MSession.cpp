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
H324MSession::H324MSession()
{
	//Create the control channel
	controlChannel = new H324MControlChannel(&channels);
	//Create audio channel
	audio = channels.CreateChannel(H324MMediaChannel::Audio);
	//Create video channel
	video = channels.CreateChannel(H324MMediaChannel::Video);
}

H324MSession::~H324MSession()
{
	//Delete control channels
	delete controlChannel;
}

int H324MSession::Init()
{
	//Set demuxer channels
	demuxer.SetChannel(0,controlChannel);
	demuxer.SetChannel(audio,channels.GetReceiver(audio));
	demuxer.SetChannel(video,channels.GetReceiver(video));

	//Set muxer channels
	muxer.SetChannel(0,controlChannel);
	muxer.SetChannel(audio,channels.GetSender(audio));
	muxer.SetChannel(video,channels.GetSender(video));
	
	//Open demuxer
	demuxer.Open(channels.GetLocalTable());

	//Open muxer
	muxer.Open(channels.GetRemoteTable());

	//Call Setup
	return controlChannel->CallSetup();
}

int H324MSession::End()
{
	//Disconnect channels
	return controlChannel->Disconnect();;
}

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
