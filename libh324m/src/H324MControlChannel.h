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
	virtual ~H324MControlChannel();
	
public:
	//Method overrides from ccsrl
	virtual int OnControlPDU(H324ControlPDU &pdu);

	//Method overrrides from h245connection
	virtual int WriteControlPDU(H324ControlPDU & pdu);
	virtual int OnError(ControlProtocolSource source, const void *);
	virtual int OnEvent(const H245Connection::Event &event);
	virtual int OnH245Request(H245_RequestMessage& req);
	virtual int OnH245Response(H245_ResponseMessage& rep);
	virtual int OnH245Command(H245_CommandMessage& cmd);
	virtual int OnH245Indication(H245_IndicationMessage& ind);

private:
	H245MasterSlave* ms;
	H245TerminalCapability* tc;
	H245RoundTripDelay* rt;
	H245MuxTable* mt;
	H245LogicalChannels* lc;
	H245ChannelsFactory* channels;
	H245MaintenanceLoop* loop;
};

#endif
