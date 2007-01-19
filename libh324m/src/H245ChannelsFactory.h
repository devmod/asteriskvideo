#ifndef _H245_CHANNELSFACTORY_H_
#define _H245_CHANNELSFACTORY_H_

#include "H223MuxTable.h"
#include "H223AL.h"
#include "H245Capabilities.h"
#include "H324MMediaChannel.h"
#include <map>

class H245ChannelsFactory
{
public:
	H245ChannelsFactory();
	~H245ChannelsFactory();

	int BuildChannelPDU(H245_OpenLogicalChannel & open,int number);
	int CreateChannel(H324MMediaChannel::e_Type type);

	H223ALSender*	GetSender(int channel);
	H223ALReceiver* GetReceiver(int channel);

	H223MuxTable* GetLocalTable();
	H223MuxTable* GetRemoteTable();
	int SetRemoteTable(H223MuxTable* table);

	H245Capabilities* GetLocalCapabilities();
	H245Capabilities* GetRemoteCapabilities();
	int SetRemoteCapabilities(H245Capabilities* remoteCapabilities);

private:
	typedef std::map<int,H324MMediaChannel*> ChannelMap;

	H245Capabilities	local;
	H245Capabilities	remote;
	H223MuxTable		localTable;
	H223MuxTable		remoteTable;
	ChannelMap			channels;
	int					numChannels;
};


#endif
