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
#include "H245LogicalChannelRate.h"
#include "log.h"


H245LogicalChannelRate::H245LogicalChannelRate(H245Connection & con)
	: H245Negotiator(con)
{
	//Reset secuence numbers
	inSequenceNumber = (DWORD) -1;
	outSequenceNumber = 0;

	//Idle state
	inState = e_Idle;
	outState = e_Idle;
}

H245LogicalChannelRate::~H245LogicalChannelRate() 
{
}


/** Incomming RMESE SDL
*/
BOOL H245LogicalChannelRate::HandleIncoming(const H245_LogicalChannelRateRequest& pdu)
{
	Logger::Debug("H245 Received LogicalChannelRate\n");
	
	//Check secuence number
	if (pdu.m_sequenceNumber == inSequenceNumber) 
		return TRUE;	

	//Get incoming sequence number
	inSequenceNumber = pdu.m_sequenceNumber;

	//Set new state
	inState = e_AwaitingResponse;

	//Send indication
	return connection.OnEvent( Event(e_TransferIndication, (unsigned int)pdu.m_logicalChannelNumber, (unsigned int)pdu.m_maximumBitRate ));
}

BOOL H245LogicalChannelRate::HandleAck(const H245_LogicalChannelRateAcknowledge& pdu)
{
	Logger::Debug("H245 Received H245_LogicalChannelRateAck\n");
	
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
	return connection.OnEvent( Event(e_TransferConfirm, pdu.m_logicalChannelNumber, pdu.m_maximumBitRate));
}
BOOL H245LogicalChannelRate::HandleReject(const H245_LogicalChannelRateReject & pdu)
{
	Logger::Debug("H245 Received LogicalChannelRateSetReject\n");
	
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
	return connection.OnEvent( Event(e_RejectIndication, pdu.m_logicalChannelNumber, 0));
}

BOOL H245LogicalChannelRate::HandleRelease(const H245_LogicalChannelRateRelease & pdu)
{
	//If idle
	if (outState == e_Idle)
        return FALSE;

	//Reset state
	outState = e_Idle;

	return connection.OnEvent(Event(e_RejectIndication, 0, 0));
}

BOOL H245LogicalChannelRate::TransferRequest( int targetChannel, int bitRate)
{
    	Logger::Debug("H245 H245LogicalChannelRate TransferRequest\n");

	//We are already in progress 
	if (outState != e_Idle) 
		return TRUE;
	
	//Increment out sequence number
	outSequenceNumber = (outSequenceNumber+1)%256;

	//Set new State
	outState = e_AwaitingResponse;

	//Build PDU
	H324ControlPDU pdu;

	pdu.BuildLogicalChannelRequest(outSequenceNumber, 
		targetChannel, bitRate);

	return connection.WriteControlPDU(pdu);
}

BOOL H245LogicalChannelRate::TransferResponse( int accept, int logicalChannel, int bitRate, int cause)
{
	//Reply
	H324ControlPDU reply;

	//Accept
	if (accept)
		reply.BuildLogicalChannelRateAck(
			inSequenceNumber,
			logicalChannel, 
			bitRate
		);
	else
		reply.BuildLogicalChannelRateReject(
			(unsigned int)inSequenceNumber, 
			logicalChannel,
			bitRate,
			(H245_LogicalChannelRateRejectReason::Choices)cause
		);
	
	//State
	inState = e_Idle;
	
	//Exit
	return connection.WriteControlPDU(reply);
}
