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
#include "log.h"

H245RoundTripDelay::H245RoundTripDelay(H245Connection & con)
	: H245Negotiator(con)
{
	awaitingResponse = FALSE;
	sequenceNumber = 0;
}

H245RoundTripDelay::~H245RoundTripDelay()
{
}

BOOL H245RoundTripDelay::Start()
{
	Logger::Debug("H245 Started round trip delay\n");

	sequenceNumber = (sequenceNumber + 1)%256;
	awaitingResponse = TRUE;

	H324ControlPDU pdu;

	pdu.BuildRoundTripDelayRequest(sequenceNumber);

	if (!connection.WriteControlPDU(pdu))
		return FALSE;

	return TRUE;
}


BOOL H245RoundTripDelay::HandleRequest(const H245_RoundTripDelayRequest & pdu)
{
	Logger::Debug("H245 Started round trip delay");

	H324ControlPDU reply;

	reply.BuildRoundTripDelayResponse(pdu.m_sequenceNumber);

	return connection.WriteControlPDU(reply);
}

BOOL H245RoundTripDelay::HandleResponse(const H245_RoundTripDelayResponse & pdu)
{
	Logger::Debug("H245 Handling round trip delay\n");

	if (awaitingResponse && pdu.m_sequenceNumber == sequenceNumber) 
	{
		awaitingResponse = FALSE;

		//Set event
		connection.OnEvent(Event());
	}

	return TRUE;
}
