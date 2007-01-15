#ifndef __H245_TERMINALCAPABILITY_
#define __H245_TERMINALCAPABILITY_

#include "H245Negotiator.h"
#include "H245ChannelsFactory.h"

class H245TerminalCapability:
	public H245Negotiator 
{
public:
	/** Events
	*/
	enum Type {
		e_TransferIndication,
		e_TransferConfirm,
		e_RejectIndication
	};

	struct Event: public H245Connection::Event
	{
		Event(Type i) 
		{
			source = H245Connection::e_CapabilityExchange;
			type = i;
		};
		Type type;
	};

public:
	//Constructor
	H245TerminalCapability(H245Connection &con,H245ChannelsFactory & factory);
	virtual ~H245TerminalCapability();
	
	//Methods
	BOOL TransferRequest();
	BOOL TransferResponse();
	BOOL RejectRequest();

	//Message Handlers
	BOOL HandleIncoming(const H245_TerminalCapabilitySet & pdu);
	BOOL HandleAck(const H245_TerminalCapabilitySetAck & pdu);
	BOOL HandleReject(const H245_TerminalCapabilitySetReject & pdu);
	BOOL HandleRelease(const H245_TerminalCapabilitySetRelease & pdu);

private:
	/** Process state
	*/
	enum States {
		e_Idle,
		e_AwaitingResponse
	};
	
	BOOL	receivedCapabilites;
	WORD	inSequenceNumber;
	WORD	outSequenceNumber;
	States	inState;
	States	outState;
	H245ChannelsFactory & channels;
};


#endif
