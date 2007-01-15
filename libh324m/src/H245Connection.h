#ifndef _H245_CONNECTION_
#define _H245_CONNECTION_
#include "H324pdu.h"
#include "Timer.h"

class H245Connection :
		public Timer
{
public:
	//Enums
	enum ControlProtocolSource 
	{
		e_MasterSlaveDetermination,
		e_CapabilityExchange,
		e_LogicalChannel,
		e_ModeRequest,
		e_RoundTripDelay,
		e_MultiplexTable
	};
	
	//Events
	struct Event
	{
		ControlProtocolSource source;
	};

	virtual int WriteControlPDU(H324ControlPDU & pdu) = 0;
	virtual int OnError(ControlProtocolSource source, const void *) = 0;
	virtual int OnEvent(const Event& event) = 0;
	
	//
	virtual int OnH245Request(H245_RequestMessage& req);
	virtual int OnH245Response(H245_ResponseMessage& rep);
	virtual int OnH245Command(H245_CommandMessage& cmd);
	virtual int OnH245Indication(H245_IndicationMessage& ind);

	//Event from Negotiators
	virtual int OnMasterSlaveDetermination(const H245MasterSlave::Event & event);
	virtual int OnCapabilityExchange(const H245TerminalCapability::Event & event);
	virtual int OnMultiplexTable(const H245MuxTable::Event &event);
	virtual int OnLogicalChannel(const H245LogicalChannels::Event &event);


};



#endif
