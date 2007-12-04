#ifndef __H245_MODE_REQUEST__H
#define __H245_MODE_REQUEST__H

#include "H245Negotiator.h"

class H245ModeRequest :
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
		Event(Type i, const H245_ArrayOf_ModeDescription*  c) 
		{
			source = H245Connection::e_ModeRequest;
			type = i;
			requestedModes = c;
		};

		Type type;
		const H245_ArrayOf_ModeDescription* requestedModes;
	};

public:
	//Constructor
	H245ModeRequest(H245Connection &con);
	virtual ~H245ModeRequest();
	
	//Methods
	BOOL TransferResponse(int accept, int causeOrResponse);

	//Message Handlers
	BOOL HandleIncoming(const H245_RequestMode& pdu);
	BOOL HandleAck(const H245_RequestModeAck & pdu);
	BOOL HandleReject(const H245_RequestModeReject & pdu);
	BOOL HandleRelease(const H245_RequestModeRelease & pdu);

private:
	/** Process state
	*/
	enum States {
		e_Idle,
		e_AwaitingResponse
	};
	
	DWORD	inSequenceNumber;
	DWORD	outSequenceNumber;
	States	inState;
	States	outState;
};


#endif
