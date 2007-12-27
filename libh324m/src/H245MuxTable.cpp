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
#include "log.h"

H245MuxTable::H245MuxTable(H245Connection & con)
	: H245Negotiator(con)
{
	//Initial states
	inState = e_Idle;
	outState = e_Idle;

	//Initial secuence number
	inSec = (unsigned)-1;
	outSec = 0;
}

H245MuxTable::~H245MuxTable()
{
}

/* Outgoing MTSE SDL
 */
BOOL H245MuxTable::TransferRequest(H223MuxTable& table)
{
	Logger::Debug("H245 MultiplexEntrySend\n");

	//If not in idle
		//Reset timer

	//Increment secuence number
	outSec = (outSec + 1)%256;

	//Set new state
	outState = e_AwaitingResponse;

	H324ControlPDU pdu;

	//Build request pdu
	H245_MultiplexEntrySend & entrySend = pdu.BuildMultiplexEntrySend(outSec);

	//Create pdu
	table.BuildPDU(entrySend);

	//Set timer

	//Write pdu
	return connection.WriteControlPDU(pdu);
}


BOOL H245MuxTable::HandleAck(const H245_MultiplexEntrySendAck  & pdu)
{
	Logger::Debug("H245 MultiplexEntrySend accepted\n");

	//If already idle
	if (outState==e_Idle)
		//Do nothing
		return TRUE;

	//Check secuence number
	if (pdu.m_sequenceNumber!=outSec) 
		return TRUE;

	//Reset timer

	//Set state
	outState = e_Idle;
	
	//List of accepted entries
	H223MuxTableEntryList list;

	//For all entries
	for (int i=0; i<pdu.m_multiplexTableEntryNumber.GetSize(); i++)
		//Append to list
		list.push_back(pdu.m_multiplexTableEntryNumber[i].GetValue());
	
	//Send event
	return connection.OnEvent(Event(e_TransferConfirm,NULL,&list));
}

BOOL H245MuxTable::HandleReject(const H245_MultiplexEntrySendReject & pdu)
{
	Logger::Debug("H245 MultiplexEntrySend rejected\n");

	//If already idle
	if (outState==e_Idle)
		//Do nothing
		return TRUE;

	//Check secuence number
	if (pdu.m_sequenceNumber!=outSec) 
		return TRUE;

	//Reset timer

	//Set state
	outState = e_Idle;
	
	//List of rejected entries
	H223MuxTableEntryList list;

	//For all entries
	for (int i=0; i<pdu.m_rejectionDescriptions.GetSize(); i++)
		//Append to list
		list.push_back(pdu.m_rejectionDescriptions[i].m_multiplexTableEntryNumber.GetValue());

	//Send event
	return connection.OnEvent(Event(e_TransferReject,NULL,&list));
}

/*
 * Incoming MTSE SDL
 */
BOOL H245MuxTable::HandleRequest(const H245_MultiplexEntrySend & pdu)
{
	Logger::Debug("H245 MultiplexEntrySend request\n");

	//Create table
	H223MuxTable table(pdu);

	H324ControlPDU pdux;

	//Build request pdu
	H245_MultiplexEntrySend & entrySend = pdux.BuildMultiplexEntrySend(outSec);

	//Create pdu
	table.BuildPDU(entrySend);

	//Depending on the state
	switch(inState)
	{
		case e_Idle:
			//Get in sequence
			inSec = pdu.m_sequenceNumber;
			//Set new state
			inState = e_AwaitingResponse;
			//Send event
			return connection.OnEvent(Event(e_TransferIndication,&table,NULL));
		case e_AwaitingResponse:
			//Get in sequence
			inSec = pdu.m_sequenceNumber;
			//Set new state
			inState = e_AwaitingResponse;
			//Should indicate a reject !
			//Send event
			return connection.OnEvent(Event(e_TransferIndication,&table,NULL));
	}

	return FALSE;
	
}

BOOL H245MuxTable::TransferResponse(H223MuxTableEntryList &accept)
{
	//Check state
	if (inState==e_Idle)
		//Exit
		return FALSE;

	H324ControlPDU pdu;

	//Build accept
	H245_MultiplexEntrySendAck &ack = pdu.BuildMultiplexEntrySendAck(inSec);

	//Take out all
	ack.m_multiplexTableEntryNumber.RemoveAll();

	//Build
	for (H223MuxTableEntryList::iterator it = accept.begin(); it != accept.end(); it++)
	{
		//Create accepted entry number
		H245_MultiplexTableEntryNumber number;
		//Set the value
		number.SetValue(*it);
		//Append to array
		ack.m_multiplexTableEntryNumber.Append((PASN_Object*)number.Clone());
	}

	//Set state
	inState = e_Idle;

	//Write
	return connection.WriteControlPDU(pdu);
}

BOOL H245MuxTable::TransferReject(H223MuxTableEntryList &reject)
{
	//Check state
	if (inState==e_Idle)
		//Exit
		return FALSE;

	H324ControlPDU pdu;

	//Build reject 
	H245_MultiplexEntrySendReject &rej = pdu.BuildMultiplexEntrySendReject(inSec);

	//Take out all
	rej.m_rejectionDescriptions.RemoveAll();

	//Build
	for (H223MuxTableEntryList::iterator it = reject.begin(); it != reject.end(); it++)
	{
		//Create reject entry description
		H245_MultiplexEntryRejectionDescriptions desc;
		//Set number
		desc.m_multiplexTableEntryNumber.SetValue(*it);
		//Set cause
		desc.m_cause.SetTag(H245_MultiplexEntryRejectionDescriptions_cause::e_unspecifiedCause);
		//Añadimos
		rej.m_rejectionDescriptions.Append((PASN_Object*)desc.Clone());
	}

	//Set state
	inState = e_Idle;

	//Write
	return connection.WriteControlPDU(pdu);
}

