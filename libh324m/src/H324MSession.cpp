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
#include "FileLogger.h"

H324MSession::H324MSession()
{
	//Set state
	state = e_None;
	//Create the control channel
	controlChannel = new H324MControlChannel(&channels);
	//Create audio channel
	audio = channels.CreateChannel(e_Audio);
	//Create video channel
	video = channels.CreateChannel(e_Video);
	//Init channels
	channels.Init(controlChannel,controlChannel,this);
	//Create logger
	logger = new FileLogger();
}

H324MSession::~H324MSession()
{
	//End channels
	channels.End();
	//Delete control channel
	delete controlChannel;
	//Delete logger
	delete logger;
}

int H324MSession::Init()
{
	//Set state
	state = e_Setup;
	//Call Setup
	return controlChannel->CallSetup();
}

int H324MSession::End()
{
	//Set state
	state = e_Hangup;
	//Disconnect channels
	return controlChannel->Disconnect();;
}

int H324MSession::Read(BYTE *buffer,int length)
{
	//Dump data
	logger->DumpInput(buffer,length);

	//Demultiplex
	return channels.Demultiplex(buffer,length);
}

int H324MSession::Write(BYTE *buffer,int length)
{
	int ret;

	//Multiplex
	ret = channels.Multiplex(buffer,length);

	//Dump data
	logger->DumpOutput(buffer,length);

	return ret;
}
Frame* H324MSession::GetFrame()
{
	//Send Frame
	return channels.GetFrame();
}

int H324MSession::SendFrame(Frame *frame)
{
	//Return Frame
	return channels.SendFrame(frame);
}

char* H324MSession::GetUserInput()
{
	//Get input
	return controlChannel->GetUserInput();
}
int	H324MSession::SendUserInput(const char *input)
{
	//Send input
	return controlChannel->SendUserInput(input);
}

int H324MSession::SendVideoFastUpdatePicture()
{
	//Get remote video channel
	int video = channels.GetRemoteChannel(e_Video);
	//If we have one
	if (video)
		//Send command
		return controlChannel->SendVideoFastUpdatePicture(video);
	//Error
	return 0;
}

H324MSession::CallState H324MSession::GetState()
{
	return state;
}

int H324MSession::OnChannelStablished(int channel, MediaType type)
{
	//Set state
	if (state == e_Setup)
		//Wait for next 
		state = e_SetupMedia;
	else
		//Both stablised
		state = e_Stablished;

	return true;
}

int H324MSession::OnChannelReleased(int channel, MediaType type)
{
	//Not implemented yet
	return true;
}

int H324MSession::ResetMediaQueue()
{
	//Call the media channels reset
	return channels.Reset();
}
