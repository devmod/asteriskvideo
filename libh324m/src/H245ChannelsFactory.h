#ifndef _H245_CHANNELSFACTORY_H_
#define _H245_CHANNELSFACTORY_H_

#include "H223MuxTable.h"
#include "H223AL.h"
#include "H245Capabilities.h"
#include "H324MMediaChannel.h"
#include "H223Demuxer.h"
#include "H223Muxer.h"
#include "H245Channel.h"
#include "Media.h"
#include <map>

class H245ChannelsFactoryListener
{
public:
	virtual int OnChannelStablished(int channel, MediaType type) = 0;
	virtual int OnChannelReleased(int channel, MediaType type) = 0;
	virtual ~H245ChannelsFactoryListener() {}
};

class H245ChannelsFactory
{
public:
	H245ChannelsFactory();
	~H245ChannelsFactory();

	int CreateChannel(MediaType type);

	H223ALSender*	GetSender(int channel);
	H223ALReceiver* GetReceiver(int channel);

	H223MuxTable* GetLocalTable();
	H223MuxTable* GetRemoteTable();
	int SetRemoteTable(H223MuxTable* table);

	H245Capabilities* GetLocalCapabilities();
	H245Capabilities* GetRemoteCapabilities();
	int SetRemoteCapabilities(H245Capabilities* remoteCapabilities);

	int Init(H223ALSender* controlSender,H223ALReceiver* controlReceiver, H245ChannelsFactoryListener *listener);
	int Reset();
	int End();

	int Demultiplex(BYTE *buffer,int length);
	int Multiplex(BYTE *buffer,int length);

	int OnEstablishIndication(int number, H245Channel *channel);
	int OnEstablishConfirm(int number);

	int OnMuxTableIndication(H223MuxTable &table, H223MuxTableEntryList &list);
	int OnMuxTableConfirm(H223MuxTableEntryList &list);

	int GetRemoteChannel(MediaType type);

	Frame* GetFrame();
	int SendFrame(Frame *frame);

private:
	typedef std::map<int,H324MMediaChannel*> ChannelMap;

	H245Capabilities	local;
	H245Capabilities	remote;
	H223MuxTable		localTable;
	H223MuxTable		remoteTable;
	H223Muxer			muxer;
	H223Demuxer			demuxer;
	ChannelMap			channels;
	int					numChannels;
	H245ChannelsFactoryListener *listener;
};


#endif
