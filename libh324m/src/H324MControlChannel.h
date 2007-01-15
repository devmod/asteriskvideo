#ifndef _H324MControlChannel_
#define _H324MControlChannel_

#include "H324CCSRLayer.h"
#include "H245Connection.h"
#include "H245MasterSlave.h"
#include "H245TerminalCapability.h"
#include "H245RoundTrip.h"
#include "H245MuxTable.h"
#include "H245LogicalChannels.h"
#include "H245MaintenanceLoop.h"
#include "H245ChannelsFactory.h"

class H324MControlChannel : 
	public H324CCSRLayer,
	public H245Connection
{
public:
	H324MControlChannel();
	~H324MControlChannel();

	//Methods
	int CallSetup();
	int MediaSetup(H324MMediaChannel *audio,H324MMediaChannel *video);
	
	//Events
	virtual int OnCallSetup() = 0;
	virtual int OnMediaSetup() = 0;
	
public:
	//Method overrides
	virtual int WriteControlPDU(H324ControlPDU & pdu);
	virtual int OnError(ControlProtocolSource source, const void *);
	virtual int OnEvent(const H245Connection::Event &event);
		
private:
	//Event from Negotiators
	virtual int OnMasterSlaveDetermination(const H245MasterSlave::Event & event);
	virtual int OnCapabilityExchange(const H245TerminalCapability::Event & event);
	virtual int OnMultiplexTable(const H245MuxTable::Event &event);
	virtual int OnLogicalChannel(const H245LogicalChannels::Event &event);

private:
	H245MasterSlave* ms;
	H245TerminalCapability* tc;
	H245RoundTripDelay* rt;
	H245MuxTable* mt;
	H245LogicalChannels* lc;
	H245ChannelsFactory* channels;
	H245MaintenanceLoop* loop;
	H324MMediaChannel *audio;
	H324MMediaChannel *video;
	
};

#endif
