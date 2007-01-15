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
			Event(PTimeInterval &d): delay(d)
			{
				source = H245Connection::e_RoundTripDelay;
			};
			PTimeInterval delay;
	};

public:
    H245RoundTripDelay(H245Connection & connection);
	virtual ~H245RoundTripDelay();

    BOOL Start();
    BOOL HandleRequest(const H245_RoundTripDelayRequest & pdu);
    BOOL HandleResponse(const H245_RoundTripDelayResponse & pdu);
    /*void HandleTimeout(PTimer &, INT);*/

    PTimeInterval GetRoundTripDelay() const { return roundTripTime; }
    BOOL IsRemoteOffline() const { return retryCount == 0; }

protected:
    BOOL          awaitingResponse;
    unsigned      sequenceNumber;
    PTimeInterval tripStartTime;
    PTimeInterval roundTripTime;
    unsigned      retryCount;
};

#endif
