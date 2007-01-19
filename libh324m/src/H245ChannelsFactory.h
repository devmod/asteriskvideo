#ifndef _H245_CHANNELSFACTORY_H_
#define _H245_CHANNELSFACTORY_H_

#include "H245Capabilities.h"
#include "H324MMediaChannel.h"
#include <map>

class H245ChannelsFactory
{
public:
	H245ChannelsFactory();
	~H245ChannelsFactory();

	void BuildPDU(H245_TerminalCapabilitySet & pdu);
	int OnRemoteCapabilities(const H245_TerminalCapabilitySet & pdu);

	int BuildChannelPDU(H245_OpenLogicalChannel & open,int number);
	int CreateChannel(H324MMediaChannel::e_Type type);

	H223ALSender* GetSender(int channel);
	H223ALReceiver* GetReceiver(int channel);

private:
	typedef std::map<int,H324MMediaChannel*> ChannelMap;

	H245Capabilities local;
	H245Capabilities remote;
	ChannelMap channels;
	int numChannels;
};


#endif
