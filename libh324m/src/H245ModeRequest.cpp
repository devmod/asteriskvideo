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
#include "H245ModeRequest.h"
#include "log.h"

H245ModeRequest::H245ModeRequest(H245Connection & con)
	: H245Negotiator(con)

{
	//Reset secuence numbers
	inSequenceNumber = (DWORD) -1;
	outSequenceNumber = 0;

	//Idle state
	inState = e_Idle;
	outState = e_Idle;
}

H245ModeRequest::~H245ModeRequest() {
}


/** Incomming RMESE SDL
*/
BOOL H245ModeRequest::HandleIncoming(const H245_RequestMode& pdu)
{
	Logger::Debug("H245 Received ModeRequest\n");
	
	//Check secuence number
	if (pdu.m_sequenceNumber == inSequenceNumber) 
		return TRUE;	

	//Get incoming sequence number
	inSequenceNumber = pdu.m_sequenceNumber;

	//Set new state
	inState = e_AwaitingResponse;

	//Send indication
	return connection.OnEvent(Event(e_TransferIndication,&pdu.m_requestedModes));
	
}

BOOL H245ModeRequest::HandleAck(const H245_RequestModeAck & pdu)
{
	Logger::Debug("H245 Received H245_RequestModeAck\n");
	
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
BOOL H245ModeRequest::HandleReject(const H245_RequestModeReject & pdu)
{
	Logger::Debug("H245 Received ModeRequestSetReject\n");
	
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
	return connection.OnEvent(Event(e_RejectIndication,NULL));
}

BOOL H245ModeRequest::HandleRelease(const H245_RequestModeRelease & pdu)
{
	//If idle
	if (outState == e_Idle)
		//Exit
        	return FALSE;

	//Reset state
	outState = e_Idle;

	//Event
	return connection.OnEvent(Event(e_RejectIndication,NULL));
}

BOOL H245ModeRequest::TransferResponse( int accept, int causeOrResponse)
{
	//Reply
	H324ControlPDU reply;

	//Accept
	if (accept) 
		reply.BuildRequestModeAck(inSequenceNumber, causeOrResponse);
	else 
		reply.BuildRequestModeReject(inSequenceNumber, causeOrResponse);
	
	//State
	inState = e_Idle;
	
	//Exit
	return connection.WriteControlPDU(reply);
}
