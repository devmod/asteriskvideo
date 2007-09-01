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
#include <list>

class H324MControlChannel : 
	public H324CCSRLayer,
	public H245Connection
{
public:
	enum States {
		e_None = 0,
		e_MasterSlaveConfirmed = 1,
		e_CapabilitiesExchanged = 2
	};

public:
	H324MControlChannel(H245ChannelsFactory* channels);
	virtual ~H324MControlChannel();

	int CallSetup();
	int MediaSetup();
	int Disconnect();

public:
	//User input
	char*	GetUserInput();
	int		SendUserInput(const char* input);
	int		SendVideoFastUpdatePicture(int channel);

	//Method overrides from ccsrl
	virtual int OnControlPDU(H324ControlPDU &pdu);

	//Method overrrides from h245connection
	virtual int WriteControlPDU(H324ControlPDU & pdu);
	virtual int OnError(ControlProtocolSource source, const void *);
	virtual int OnEvent(const H245Connection::Event &event);

private:
	int OnH245Request(H245_RequestMessage& req);
	int OnH245Response(H245_ResponseMessage& rep);
	int OnH245Command(H245_CommandMessage& cmd);
	int OnH245Indication(H245_IndicationMessage& ind);

	int OnMasterSlaveDetermination(const H245MasterSlave::Event & event);
	int OnCapabilityExchange(const H245TerminalCapability::Event & event);
	int OnMultiplexTable(const H245MuxTable::Event &event);
	int OnLogicalChannel(const H245LogicalChannels::Event &event);

	int OnUserInput(const char* input);

private:
	H245MasterSlave* ms;
	H245TerminalCapability* tc;
	H245RoundTripDelay* rt;
	H245MuxTable* mt;
	H245LogicalChannels* lc;
	H245ChannelsFactory* cf;
	H245MaintenanceLoop* loop;
	std::list<char *> inputList;

	int state;
	int master;
};

#endif
