#include "H324MSession.h"

#define Debug printf 
int H324MSession::Init(void)
{
	//Only entry 0 at the beginning is available
	tableDemux.SetEntry(0,"","0");
	
	//Init the demuxer
	demuxer.Open(&tableDemux);

	//Add the reciever
	demuxer.SetChannel(0,(H324MControlChannel*)this);

	//Set mcs
	tableMux.SetEntry(0,"","0");

	//Init the muxer
	muxer.Open(&tableMux);
	
	//Set the control channel
	muxer.SetChannel(0,(H324MControlChannel*)this);

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
	return CallSetup();
}
int H324MSession::End()
{
	return true;
}

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
	return MediaSetup(&audioChannel,&videoChannel);
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
