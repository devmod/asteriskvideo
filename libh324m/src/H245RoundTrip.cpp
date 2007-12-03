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
#include "H245RoundTrip.h"

#define Debug printf

H245RoundTripDelay::H245RoundTripDelay(H245Connection & con)
	: H245Negotiator(con)
{
	awaitingResponse = FALSE;
	sequenceNumber = 0;
	retryCount = 1;
}

H245RoundTripDelay::~H245RoundTripDelay()
{
}

BOOL H245RoundTripDelay::Start()
{
	Debug("H245 Started round trip delay\n");

	sequenceNumber = (sequenceNumber + 1)%256;
	awaitingResponse = TRUE;

	H324ControlPDU pdu;

	pdu.BuildRoundTripDelayRequest(sequenceNumber);
	if (!connection.WriteControlPDU(pdu))
		return FALSE;

	tripStartTime = PTimer::Tick();

	return TRUE;
}


BOOL H245RoundTripDelay::HandleRequest(const H245_RoundTripDelayRequest & pdu)
{
	Debug("H245 Started round trip delay");

	H324ControlPDU reply;
	reply.BuildRoundTripDelayResponse(pdu.m_sequenceNumber);
	return connection.WriteControlPDU(reply);
}

BOOL H245RoundTripDelay::HandleResponse(const H245_RoundTripDelayResponse & pdu)
{
	PTimeInterval tripEndTime = PTimer::Tick();

	Debug("H245 Handling round trip delay\n");

	if (awaitingResponse && pdu.m_sequenceNumber == sequenceNumber) 
	{
		awaitingResponse = FALSE;
		roundTripTime = tripEndTime - tripStartTime;
		retryCount = 3;

		//Set event
		connection.OnEvent(Event(roundTripTime));
	}

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
