#ifndef __H245_TERMINALCAPABILITY_
#define __H245_TERMINALCAPABILITY_

#include "H245Negotiator.h"
#include "H245Capabilities.h"

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
		Event(Type i,H245Capabilities*  c) 
		{
			source = H245Connection::e_CapabilityExchange;
			type = i;
			capabilities = c;
		};

		Type type;
		H245Capabilities* capabilities;
	};

public:
	//Constructor
	H245TerminalCapability(H245Connection &con);
	virtual ~H245TerminalCapability();
	
	//Methods
	BOOL TransferRequest(H245Capabilities* capabilities);
	BOOL TransferResponse(int accept);

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
	DWORD	inSequenceNumber;
	DWORD	outSequenceNumber;
	States	inState;
	States	outState;
};


#endif
