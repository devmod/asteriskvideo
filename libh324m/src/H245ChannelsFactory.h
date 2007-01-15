#ifndef _H245_CHANNELSFACTORY_H_
#define _H245_CHANNELSFACTORY_H_

#include "H245Capabilities.h"
#include "H324MMediaChannel.h"
#include <map>
using namespace std;

class H245ChannelsFactory
{
public:
	H245ChannelsFactory();
	~H245ChannelsFactory();

	void BuildPDU(H245_TerminalCapabilitySet & pdu);
	int OnRemoteCapabilities(const H245_TerminalCapabilitySet & pdu);


	int BuildChannelPDU(H245_OpenLogicalChannel & open,int number);

private:
	H245Capabilities local;
	H245Capabilities remote;
};


#endif
