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
	enum Type {
		e_TransferConfirm,
		e_TransferReject,
		e_TransferIndication
	};

	struct Event: public H245Connection::Event
	{
		public:
			Event(Type t,H223MuxTable* table,H223MuxTableEntryList* e)
			{
				source = H245Connection::e_MultiplexTable;
				type = t;
				muxTable = table;
				entries = e;
			};
			Type type;
			H223MuxTable* muxTable;
			H223MuxTableEntryList* entries;
	};

public:
	H245MuxTable(H245Connection & connection);
	~H245MuxTable();

	BOOL TransferRequest(H223MuxTable &table);
	BOOL TransferResponse(H223MuxTableEntryList &accept);
	BOOL TransferReject(H223MuxTableEntryList &reject);

	BOOL HandleRequest(const H245_MultiplexEntrySend & pdu);
    BOOL HandleAck(const H245_MultiplexEntrySendAck  & pdu);
	BOOL HandleReject(const H245_MultiplexEntrySendReject & pdu);

private:
	enum States{
		e_Idle,
		e_AwaitingResponse,
	};

	States inState;
	States outState;

    unsigned      inSec;
    unsigned      outSec;
};

#endif
