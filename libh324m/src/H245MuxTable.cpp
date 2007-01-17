/* H324M library
 *
 * Copyright (C) 2006 Sergio Garcia Murillo
 *
 * sergio.garcia@fontventa.com
 * http://sip.fontventa.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "H245MuxTable.h"

#define Debug printf

H245MuxTable::H245MuxTable(H245Connection & con)
	: H245Negotiator(con)
{
	awaitingResponse = FALSE;
	sequenceNumber = 0;
	retryCount = 1;
}

H245MuxTable::~H245MuxTable()
{
}

BOOL H245MuxTable::Send(H223MuxTable& table)
{
	Debug("H245 MultiplexEntrySend\n");

	sequenceNumber = (sequenceNumber + 1)%256;
	awaitingResponse = TRUE;

	H324ControlPDU pdu;

	//Build request pdu
	pdu.BuildMultiplexEntrySend(sequenceNumber);

	//Create pdu
	table.BuildPDU((H245_MultiplexEntrySend &)(H245_RequestMessage&)pdu);

	if (!connection.WriteControlPDU(pdu))
		return FALSE;

	return TRUE;
}


BOOL H245MuxTable::HandleRequest(const H245_MultiplexEntrySend & pdu)
{
	Debug("H245 MultiplexEntrySend request\n");

	H324ControlPDU reply;

	H223MuxTable table(pdu);

	//Set event
	if (connection.OnEvent(Event(table)))
	{
		//Build accept
		H245_MultiplexEntrySendAck &ack = reply.BuildMultiplexEntrySendAck(pdu.m_sequenceNumber);
		//Take out all
		ack.m_multiplexTableEntryNumber.RemoveAll();
		//Build
		for (int i=0;i<pdu.m_multiplexEntryDescriptors.GetSize();i++)
			ack.m_multiplexTableEntryNumber.Append((PASN_Object*)pdu.m_multiplexEntryDescriptors[i].m_multiplexTableEntryNumber.Clone());
		
	} else {
		reply.BuildMultiplexEntrySendReject(pdu.m_sequenceNumber);
	}

	return connection.WriteControlPDU(reply);
}

BOOL H245MuxTable::HandleAck(const H245_MultiplexEntrySendAck  & pdu)
{
	Debug("H245 MultiplexEntrySend accepted\n");

	if (!awaitingResponse || pdu.m_sequenceNumber != sequenceNumber) 
		return FALSE;

	awaitingResponse = FALSE;
	retryCount = 3;
	
	return TRUE;
}

BOOL H245MuxTable::HandleReject(const H245_MultiplexEntrySendReject & pdu)
{
	Debug("H245 MultiplexEntrySend rejected\n");

	if (!awaitingResponse || pdu.m_sequenceNumber != sequenceNumber) 
		return FALSE;

	awaitingResponse = FALSE;
	retryCount = 3;
	
	return TRUE;
}
/*
void H245RoundTripDelay::HandleTimeout(PTimer &, INT)
{
//	PWaitAndSignal wait(mutex);

//	PTRACE(3, "H245\tTimeout on round trip delay: seq=" << sequenceNumber
	//			 << (awaitingResponse ? " awaitingResponse" : " idle"));

	if (awaitingResponse && retryCount > 0)
		retryCount--;
	awaitingResponse = FALSE;

	connection.OnControlProtocolError(H245Connection::e_RoundTripDelay, "Timeout");
}
*/
