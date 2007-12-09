#ifndef _H245_CONNECTION_
#define _H245_CONNECTION_
#include "H324pdu.h"

class H245Connection 
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
		e_MultiplexTable,
		e_LogicalChannelRate
	};
	
	//Events
	struct Event
	{
		ControlProtocolSource source;
	};

	virtual int WriteControlPDU(H324ControlPDU & pdu) = 0;
	virtual int OnError(ControlProtocolSource source, const void *) = 0;
	virtual int OnEvent(const Event& event) = 0;
	/*
	virtual int OnH245Request(H245_RequestMessage& req) = 0;
	virtual int OnH245Response(H245_ResponseMessage& rep) = 0;
	virtual int OnH245Command(H245_CommandMessage& cmd) = 0;
	virtual int OnH245Indication(H245_IndicationMessage& ind) = 0;
	*/
	virtual ~H245Connection() {}
};



#endif
