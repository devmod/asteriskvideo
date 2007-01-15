#include "H324MMediaChannel.h"


int H324MMediaChannel::Init()
{
	return 1;
}
int H324MMediaChannel::End()
{
	//Clean
	if (sender)
		delete sender;
	if (receiver)
		delete receiver;

	return 1;
}


H223ALReceiver*  H324MMediaChannel::GetReceiver()
{
	return receiver;
}
H223ALSender* H324MMediaChannel::GetSender()
{
	return sender;
}

int H324MMediaChannel::SetSenderLayer(int layer)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case 1:
			sender = new H223AL1Sender();
			break;
		case 2:
			sender = new H223AL3Sender();
			break;
		case 3:
			sender = new H223AL3Sender();
			break;
		default:
			return 0;
	}

	return 1;
}

int H324MMediaChannel::SetReceiverLayer(int layer)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case 1:
			receiver = new H223AL1Receiver();
			break;
		case 2:
			receiver = new H223AL2Receiver(1);
			break;
		case 3:
			receiver = new H223AL3Receiver();
			break;
		default:
			return 0;
	}

	return 1;
}

