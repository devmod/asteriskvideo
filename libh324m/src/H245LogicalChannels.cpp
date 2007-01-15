#include "H245LogicalChannels.h"
  
  

H245LogicalChannels::H245LogicalChannels(H245Connection & con,H245ChannelsFactory & factory)
	: H245Negotiator(con),channels(factory)
{
}
H245LogicalChannels::~H245LogicalChannels()
{
}

/** Outgoing LCSE SDL
*/
int H245LogicalChannels::EstablishRequest(int channelNumber)
{
	Debug("H245 H245LogicalChannels Establish Request [%d]\n", channelNumber);

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
			Debug("Released\n");

			//H324ControlPDU pdu;
			H245_OpenLogicalChannel & open = pdu.BuildOpenLogicalChannel(channelNumber);
			//Set capabilities
			if (!channels.BuildChannelPDU(open,channelNumber))
				//End
				return FALSE;
			//Set state
			out[channelNumber] = e_AwaitingEstablishment;
			Debug("WriteOpen\n");
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
			if (!channels.BuildChannelPDU(open,channelNumber))
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
	Debug("H245 H245LogicalChannels Release Request [%d]\n", channelNumber);

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
			H245_CloseLogicalChannel & open = pdu.BuildCloseLogicalChannel(channelNumber);
			//Send pdu
  			return connection.WriteControlPDU(pdu);
		}
		case e_Established:
		{
			//Change state
			out[channelNumber] = e_AwaitingRelease;
			//H324ControlPDU pdu;
			H245_CloseLogicalChannel & open = pdu.BuildCloseLogicalChannel(channelNumber);
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
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Debug("H245 H245LogicalChannelAck received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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
int H245LogicalChannels::EstablishResponse(int channelNumber,int accept)
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

	//Build
	if (accept)
		//Ack
		reply.BuildOpenLogicalChannelAck(channelNumber);
	else
		//Reject
		reply.BuildOpenLogicalChannelAck(channelNumber);

	//Send it
	return connection.WriteControlPDU(reply);
}

BOOL H245LogicalChannels::HandleOpen(const H245_OpenLogicalChannel & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Debug("H245 H245OpenLogicalChannel received [%d]\n", channelNumber);

	//See if channel exist
	if (in.find(channelNumber)==in.end())
		//Create channel & set state
		in[channelNumber] = e_Released;;
	
	//Check state
	switch (in[channelNumber])
	{
		case e_Released:
			//Set state
			in[channelNumber] = e_AwaitingEstablishment;
			//Send error & continue 
            return connection.OnEvent(Event(e_EstablishIndication,channelNumber));
		case e_AwaitingEstablishment:
			//Set event
			connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
			//Send event
			return connection.OnEvent(Event(e_EstablishIndication,channelNumber));
		case e_Established:
			//Set event
			connection.OnEvent(Event(e_ReleaseIndication,channelNumber));
			//Set state
			in[channelNumber] = e_AwaitingEstablishment;
			//Send event
			return connection.OnEvent(Event(e_EstablishIndication,channelNumber));
	}
	
	//Exit
	return FALSE;	
}



BOOL H245LogicalChannels::HandleClose(const H245_CloseLogicalChannel & pdu)
{
	H324ControlPDU reply;

	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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
	}
	
	//Exit
	return FALSE;
}


BOOL H245LogicalChannels::HandleOpenConfirm(const H245_OpenLogicalChannelConfirm & pdu)
{
	//Get channel number
	int channelNumber = pdu.m_forwardLogicalChannelNumber;

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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

	Debug("H245 H245LogicalChannelReject received [%d]\n", channelNumber);

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
