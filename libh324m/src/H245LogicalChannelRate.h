#ifndef __H245_LOGICAL_CHANNEL_RATE__H
#define __H245_LOGICAL_CHANNEL_RATE__H

#include "H245Negotiator.h"

class H245LogicalChannelRate :
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
		Event(Type i, unsigned int logicalChannel, unsigned int bitRate)
		{
			source 	= H245Connection::e_LogicalChannelRate;
			type 	= i;
			targetChannel    = logicalChannel;
			requestedBitRate = bitRate;
		};

		Type type;
		unsigned int targetChannel;
		unsigned int requestedBitRate;// units of 100bps
	};

public:
	//Constructor
	H245LogicalChannelRate(H245Connection &con);
	~H245LogicalChannelRate();
	
	//Methods
	BOOL TransferRequest(int targetChannel, int bitRate);
	BOOL TransferResponse( int accept, int logicalChannel, int bitRate, int cause);

	//Message Handlers
	BOOL HandleIncoming(const H245_LogicalChannelRateRequest& pdu);
	BOOL HandleAck(const H245_LogicalChannelRateAcknowledge& pdu);
	BOOL HandleReject(const H245_LogicalChannelRateReject& pdu);
	BOOL HandleRelease(const H245_LogicalChannelRateRelease& pdu);

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
