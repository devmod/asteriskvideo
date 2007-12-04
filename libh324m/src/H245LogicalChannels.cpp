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
#include "H245LogicalChannels.h"
#include "log.h"

H245LogicalChannels::H245LogicalChannels(H245Connection & con)
	: H245Negotiator(con)
{
}

H245LogicalChannels::~H245LogicalChannels()
{
}

/** Outgoing LCSE SDL
*/
int H245LogicalChannels::EstablishRequest(int channelNumber,H245Channel & channel)
{
	Logger::Debug("H245 H245LogicalChannels Establish Request [%d]\n", channelNumber);

	//TODO: Store h245channel on list

	//Pdu
	H324ControlPDU pdu;

	//See if there is a channel already
	if (out.find(channelNumber)==out.end())
		//Create channel & set state
		out[channelNumber] = e_Released;
	
	//Check state
	switch(out[channelNumber])
	{
		case e_Released:
		{
			//H324ControlPDU pdu;
			H245_OpenLogicalChannel & open = pdu.BuildOpenLogicalChannel(channelNumber);
			//Set capabilities
			if (!channel.BuildChannelPDU(open))
				//End
				return FALSE;
			//Set state
			out[channelNumber] = e_AwaitingEstablishment;
			//Send pdu
  			return connection.WriteControlPDU(pdu);
		}
		case e_AwaitingEstablishment:
			//Uh?
			return FALSE;
		case e_Established:
			//Uh?
			return FALSE;
		case e_AwaitingRelease:
		{
			//H324ControlPDU pdu;
			H245_OpenLogicalChannel & open = pdu.BuildOpenLogicalChannel(channelNumber);
			//Set capabilities
			if (!channel.BuildChannelPDU(open))
				//End
				return FALSE;
			//Set state
			out[channelNumber] = e_AwaitingEstablishment;
			//Send pdu
  			return connection.WriteControlPDU(pdu);
		}
	}

	//Exit
	return FALSE;
}

int H245LogicalChannels::ReleaseRequest(int channelNumber)
{
	Logger::Debug("H245 H245LogicalChannels Release Request [%d]\n", channelNumber);

	//Pdu
	H324ControlPDU pdu;

	//See if channel exist
	if (out.find(channelNumber)==out.end())
		//Exit
		return FALSE;
	
	//Check state
	switch(out[channelNumber])
	{
		case e_Released:
			//Uh?
			return FALSE;
		case e_AwaitingEstablishment:
		{
			//Change state
			out[channelNumber] = e_AwaitingRelease;
			//H324ControlPDU pdu;
			//H245_CloseLogicalChannel & open = pdu.BuildCloseLogicalChannel(channelNumber);
			//Send pdu
  			return connection.WriteControlPDU(pdu);
		}
		case e_Established:
		{
			//Change state
			out[channelNumber] = e_AwaitingRelease;
			//H324ControlPDU pdu;
			//H245_CloseLogicalChannel & open = pdu.BuildCloseLogicalChannel(channelNumber);
			//Send pdu
  			return connection.WriteControlPDU(pdu);
		}
		case e_AwaitingRelease:
			//Uh?
			return FALSE;
	}

	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleOpenAck(const H245_OpenLogicalChannelAck & pdu)
{
	//TODO: return h245channel on event

	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelAck received [%d]\n", channelNumber);

	//See if channel exist
	if (out.find(channelNumber)==out.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (out[channelNumber])
	{
		case e_Released:
			//Error indication
			return connection.OnEvent(Event(e_ErrorIndication,channelNumber));
		case e_AwaitingEstablishment:
			//Set state
			out[channelNumber] = e_Established;
			//Send event
			return connection.OnEvent(Event(e_EstablishConfirm,channelNumber));
		case e_Established:
			//Good
			return TRUE;
		case e_AwaitingRelease:
			//Good
			return TRUE;
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleReject(const H245_OpenLogicalChannelReject & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (out.find(channelNumber)==out.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (out[channelNumber])
	{
		case e_Released:
			//Send error & continue 
			return connection.OnEvent(Event(e_ErrorIndication,channelNumber));
		case e_AwaitingEstablishment:
			//Set state
			out[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
		case e_Established:
			//Send error & continue 
			connection.OnEvent(Event(e_ErrorIndication,channelNumber));
			//Set state
			out[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
		case e_AwaitingRelease:
			//Set state
			out[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseConfirm,channelNumber));
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleCloseAck(const H245_CloseLogicalChannelAck & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (out.find(channelNumber)==out.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (out[channelNumber])
	{
		case e_Released:
			//Good
			return TRUE;
		case e_AwaitingEstablishment:
			//Nothing
			return TRUE;
		case e_Established:
			//Send event
			connection.OnEvent(Event(e_ErrorIndication,channelNumber));
			//Set state
			out[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
		case e_AwaitingRelease:
			//Set state
			out[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseConfirm,channelNumber));
	}
	
	//Exit
	return FALSE;
}

/** Incomming LCSE SDL
*/
int H245LogicalChannels::EstablishResponse(int channelNumber)
{
	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;

	//If not in good state
	if(in[channelNumber]!=e_AwaitingEstablishment)
		//Exit
		return FALSE;

	//PDU
	H324ControlPDU reply;

	//Ack
	reply.BuildOpenLogicalChannelAck(channelNumber);
	
	//Establish
	in[channelNumber]=e_Established;

	//Send it
	return connection.WriteControlPDU(reply);
}

int H245LogicalChannels::EstablishReject(int channelNumber,unsigned cause)
{
	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;

	//If not in good state
	if(in[channelNumber]!=e_AwaitingEstablishment)
		//Exit
		return FALSE;

	//PDU
	H324ControlPDU reply;

	//Reject
	reply.BuildOpenLogicalChannelReject(channelNumber,cause);

	//Establish
	in[channelNumber]=e_Released;

	//Send it
	return connection.WriteControlPDU(reply);
}

BOOL H245LogicalChannels::HandleOpen(H245_OpenLogicalChannel & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245OpenLogicalChannel received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Create channel & set state
		in[channelNumber] = e_Released;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Released:
			//Set state
			in[channelNumber] = e_AwaitingEstablishment;
			//Send error & continue 
            return connection.OnEvent(Event(e_EstablishIndication,channelNumber,new H245Channel(pdu)));
		case e_AwaitingEstablishment:
			//Set event
			connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
			//Send event
			return connection.OnEvent(Event(e_EstablishIndication,channelNumber,new H245Channel(pdu)));
		case e_Established:
			//Set event
			connection.OnEvent(Event(e_ReleaseIndication,channelNumber,new H245Channel(pdu)));
			//Set state
			in[channelNumber] = e_AwaitingEstablishment;
			//Send event
			return connection.OnEvent(Event(e_EstablishIndication,channelNumber,new H245Channel(pdu)));
		case e_AwaitingRelease:
			return FALSE;
	}
	
	//Exit
	return FALSE;	
}

BOOL H245LogicalChannels::HandleClose(const H245_CloseLogicalChannel & pdu)
{
	H324ControlPDU reply;

	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;

	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
			//Build pdu
			reply.BuildCloseLogicalChannelAck(channelNumber);
			//Send it
			connection.WriteControlPDU(reply);
			//Set state
			in[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
		case e_AwaitingEstablishment:
			//Build pdu
			reply.BuildCloseLogicalChannelAck(channelNumber);
			//Send it
			connection.WriteControlPDU(reply);
			//Set state
			in[channelNumber] = e_Released;
			//Send event
			return connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
		case e_Released:
			//Build pdu
			reply.BuildCloseLogicalChannelAck(channelNumber);
			//Send it
			return connection.WriteControlPDU(reply);
		case e_AwaitingRelease:
			return FALSE;
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleOpenConfirm(const H245_OpenLogicalChannelConfirm & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
		case e_AwaitingEstablishment:
		case e_AwaitingRelease:
		case e_Released:
			return FALSE;
	}
	
	//Exit
	return FALSE;
}

/*

BOOL H245LogicalChannels::HandleRequestClose(const H245_RequestChannelClose & pdu)
{
//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
		case e_AwaitingEstablishment:
		case e_AwaitingRelease:
		case e_Released:
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleRequestCloseAck(const H245_RequestChannelCloseAck & pdu)
{
//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
		case e_AwaitingEstablishment:
		case e_AwaitingRelease:
		case e_Released:
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleRequestCloseReject(const H245_RequestChannelCloseReject & pdu)
{
//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
		case e_AwaitingEstablishment:
		case e_AwaitingRelease:
		case e_Released:
	}
	
	//Exit
	return FALSE;
}

BOOL H245LogicalChannels::HandleRequestCloseRelease(const H245_RequestChannelCloseRelease & pdu)
{
//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Logger::Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Exit
		return FALSE;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Established:
		case e_AwaitingEstablishment:
		case e_AwaitingRelease:
		case e_Released:
	}
	
	//Exit
	return FALSE;
}
*/
