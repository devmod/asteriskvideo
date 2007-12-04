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
#include "H245TerminalCapability.h"
#include "log.h"

H245TerminalCapability::H245TerminalCapability(H245Connection & con)
	: H245Negotiator(con)

{
	//Reset secuence numbers
	inSequenceNumber = (DWORD) -1;
	outSequenceNumber = 0;

	//Idle state
	inState = e_Idle;
	outState = e_Idle;
}

H245TerminalCapability::~H245TerminalCapability() {
}

/** Outgoing CESE SDL
*/

BOOL H245TerminalCapability::TransferRequest(H245Capabilities* capabilities)
{
	//Logger::Debug
	Logger::Debug("H245 TerminalCapabilitySet TransferRequest\n");

	//We are already in progress 
	if (outState != e_Idle) 
		return TRUE;
	
	//Incremente out secuance number
	outSequenceNumber = (outSequenceNumber+1)%256;

	//Set new State
	outState = e_AwaitingResponse;

	//Build PDU
	H324ControlPDU pdu;

	//Set capabilites
	capabilities->BuildPDU(pdu.BuildTerminalCapabilitySet(outSequenceNumber));

	//Write pdu
	return connection.WriteControlPDU(pdu);
}

BOOL H245TerminalCapability::HandleAck(const H245_TerminalCapabilitySetAck & pdu)
{
	Logger::Debug("H245 Received TerminalCapabilitySetAck\n");
	
	//Check sequence number
	if (pdu.m_sequenceNumber != outSequenceNumber)
		//Exit
		return TRUE;

	//If we are idle
	if (outState == e_Idle)
		//Exit
		return FALSE;

	//Reset state
 	outState = e_Idle;

	//Event
	return connection.OnEvent(Event(e_TransferConfirm,NULL));
}

BOOL H245TerminalCapability::HandleReject(const H245_TerminalCapabilitySetReject & pdu)
{
	Logger::Debug("H245 Received TerminalCapabilitySetReject\n");
	
	//Check sequence number
	if (pdu.m_sequenceNumber != outSequenceNumber)
		//Exit
		return TRUE;

	//If we are idle
	if (outState == e_Idle)
		//Exit
		return FALSE;

	//Reset state
 	outState = e_Idle;

	//Event
	return connection.OnEvent(Event(e_RejectIndication,NULL));;
}


/** Incomming CESE SDL
*/
BOOL H245TerminalCapability::HandleIncoming(const H245_TerminalCapabilitySet & pdu)
{
	Logger::Debug("H245 Received TerminalCapabilitySet\n");
	
	//Check secuence number
	if (pdu.m_sequenceNumber == inSequenceNumber) 
		return TRUE;	

	//Get incoming sequence number
	inSequenceNumber = pdu.m_sequenceNumber;

	//Set new state
	inState = e_AwaitingResponse;

	//Build capabilities object
	H245Capabilities remoteCapabilities(pdu);

	//Send indication
	return connection.OnEvent(Event(e_TransferIndication,&remoteCapabilities));
	
}

BOOL H245TerminalCapability::TransferResponse(int accept)
{
	//If not waiting for response
	if (inState != e_AwaitingResponse)
		return FALSE;

	//Reply
	H324ControlPDU reply;

	//Accept
	if (accept)
		reply.BuildTerminalCapabilitySetAck(inSequenceNumber);
	else
		reply.BuildTerminalCapabilitySetReject(inSequenceNumber,H245_TerminalCapabilitySetReject_cause::e_unspecified);
	
	//State
	inState = e_Idle;
	
	//Exit
	return connection.WriteControlPDU(reply);
}

BOOL H245TerminalCapability::HandleRelease(const H245_TerminalCapabilitySetRelease & /*pdu*/)
{
	//If idle
	if (outState == e_Idle)
        return FALSE;

	//Reset state
	outState = e_Idle;

	return connection.OnEvent(Event(e_RejectIndication,NULL));
}
