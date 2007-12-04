#ifndef __H245_ROUNDTRIP_
#define __H245_ROUNDTRIP_

#include "H245Negotiator.h"

class H245RoundTripDelay:
	public H245Negotiator 
{
public:
	/** Events
	*/
	struct Event: public H245Connection::Event
	{
		public:
			Event()
			{
				source = H245Connection::e_RoundTripDelay;
			};
	};

public:
	H245RoundTripDelay(H245Connection & connection);
	virtual ~H245RoundTripDelay();

	BOOL Start();
	BOOL HandleRequest(const H245_RoundTripDelayRequest & pdu);
	BOOL HandleResponse(const H245_RoundTripDelayResponse & pdu);

protected:
	BOOL          awaitingResponse;
	unsigned      sequenceNumber;
};

#endif
