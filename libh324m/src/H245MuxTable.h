#ifndef _H245MUXTABLE_H_
#define _H245MUXTABLE_H_

#include "H245Negotiator.h"
#include "H223MuxTable.h"

class H245MuxTable:
	public H245Negotiator 
{
public:
	/** Events
	*/
	struct Event: public H245Connection::Event
	{
		public:
			Event(H223MuxTable& table) : muxTable(table)
			{
				source = H245Connection::e_MultiplexTable;
			};
			H223MuxTable& muxTable;
	};

public:
	H245MuxTable(H245Connection & connection);
	~H245MuxTable();

	BOOL Send(H223MuxTable &table);
    BOOL HandleRequest(const H245_MultiplexEntrySend & pdu);
    BOOL HandleAck(const H245_MultiplexEntrySendAck  & pdu);
	BOOL HandleReject(const H245_MultiplexEntrySendReject & pdu);

private:
	BOOL          awaitingResponse;
    unsigned      sequenceNumber;
    unsigned      retryCount;
};

#endif
