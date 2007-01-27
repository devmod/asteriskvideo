#ifndef _H245_LOGICALCHANNELS_H_
#define _H245_LOGICALCHANNELS_H_
#include <map>
#include "H245Negotiator.h"
#include "H245Channel.h"

class H245LogicalChannels : public H245Negotiator
{
public:
		
	/** Events
	*/
	enum Type {
		e_EstablishConfirm,
		e_EstablishIndication,
		e_ReleaseIndication,
		e_ReleaseConfirm,
		e_ErrorIndication
	};

	struct Event: public H245Connection::Event
	{
		Event(Type t,int n,H245Channel* c = NULL) 
		{
			source = H245Connection::e_LogicalChannel;
			type = t;
			number = n;
			channel = c;
		};
		Type type;
		int	number;
		H245Channel* channel;
	};

public:
	H245LogicalChannels(H245Connection &con);
	virtual ~H245LogicalChannels();

    int EstablishRequest(int channelNumber,H245Channel & channel);
	int EstablishResponse(int channelNumber);
	int EstablishReject(int channelNumber,unsigned cause = 0);
	int ReleaseRequest(int channelNumber);

    virtual BOOL HandleOpen(H245_OpenLogicalChannel & pdu);
    virtual BOOL HandleOpenAck(const H245_OpenLogicalChannelAck & pdu);
    virtual BOOL HandleOpenConfirm(const H245_OpenLogicalChannelConfirm & pdu);
    virtual BOOL HandleReject(const H245_OpenLogicalChannelReject & pdu);
    virtual BOOL HandleClose(const H245_CloseLogicalChannel & pdu);
    virtual BOOL HandleCloseAck(const H245_CloseLogicalChannelAck & pdu);
	/*
    virtual BOOL HandleRequestClose(const H245_RequestChannelClose & pdu);
    virtual BOOL HandleRequestCloseAck(const H245_RequestChannelCloseAck & pdu);
    virtual BOOL HandleRequestCloseReject(const H245_RequestChannelCloseReject & pdu);
    virtual BOOL HandleRequestCloseRelease(const H245_RequestChannelCloseRelease & pdu);
	*/
private:
	/** Channels
	*/
	enum States{
		e_AwaitingEstablishment,
		e_Established,
		e_AwaitingRelease,
		e_Released
	};

	typedef std::map<int,States> StateMap;

private:
	StateMap out;
	StateMap in;
};


#endif
