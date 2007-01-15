#include "H245Connection.h"


int H245Connection::OnError(ControlProtocolSource source, const void *str)
{

	switch(source)
	{
		case H245Connection::e_MasterSlaveDetermination:
			Debug("MasterSlaveDetermination error %s",(char *) str);
			break;
		case H245Connection::e_CapabilityExchange:
			Debug("CapabilityExchange error %s",(char *) str);
			break;
		case H245Connection::e_RoundTripDelay:
			Debug("RoundTripDelay error %s",(char *) str);
			break;
		case H245Connection::e_LogicalChannel:
			Debug("LogicalChannel error %s",(char *) str);
			break;
		case H245Connection::e_ModeRequest:
			Debug("ModeRequest error %s",(char *) str);
			break;
	}

	//Exit
	return 1;
}

int H245Connection::OnEvent(const H245Connection::Event &event)
{
	//Depending on the event
	switch(event.source)
	{
		case H245Connection::e_MasterSlaveDetermination:
			return OnMasterSlaveDetermination((const H245MasterSlave::Event &)event);
		case H245Connection::e_CapabilityExchange:
			return OnCapabilityExchange((const H245TerminalCapability::Event &)event);
		case H245Connection::e_MultiplexTable:
			return OnMultiplexTable((const H245MuxTable::Event &)event);
		case H245Connection::e_LogicalChannel:
			return OnLogicalChannel((const H245LogicalChannels::Event &)event);
		case H245Connection::e_ModeRequest:
		case H245Connection::e_RoundTripDelay:
			break;
	}

	//Exits
	return FALSE;;
}

int H245Connection::WriteControlPDU(H324ControlPDU & pdu)
{
	//Send pdu to ccsrl layer
	SendPDU(pdu);

	//Exit
	return 1;
}

int H245Connection::OnControlPDU(H324ControlPDU &pdu)
{
	Debug("OnControlPDU\n");

	//Depending on the pdu
	switch(pdu.GetTag())
	{
		case H245_MultimediaSystemControlMessage::e_request:
			//Is a request
			if(!OnH245Request((H245_RequestMessage&)pdu))
			{
				H324ControlPDU reply;
  				reply.BuildFunctionNotUnderstood(pdu);
  				WriteControlPDU(reply);
			}	
			return 1;
		case H245_MultimediaSystemControlMessage::e_response:
			//Is a response
			return OnH245Response((H245_ResponseMessage&)pdu);
		case H245_MultimediaSystemControlMessage::e_command:
			//Is a command
			return OnH245Command((H245_CommandMessage&)pdu);
		case H245_MultimediaSystemControlMessage::e_indication:
			//Is an indication
			return OnH245Indication((H245_IndicationMessage&)pdu);
		default:
			Debug("Unknown PDU\n");
			//?????
			return 0;
	} 

	//Exit
	return 1;
}


int H245Connection::OnH245Request(H245_RequestMessage& req)
{
	//Depending on tue tag
	switch(req.GetTag())
	{
		//MasterSlave
		case H245_RequestMessage::e_masterSlaveDetermination:
			return ms->HandleIncoming(req);
		//TerminalCapabilities
		case H245_RequestMessage::e_terminalCapabilitySet:
			return tc->HandleIncoming(req);
		//RoundTrip
		case H245_RequestMessage::e_roundTripDelayRequest:
			return rt->HandleRequest(req);
		//MultiplesEntry
		case H245_RequestMessage::e_multiplexEntrySend:
			return mt->HandleRequest(req);
		//LogicalChannel
		case H245_RequestMessage::e_openLogicalChannel:
			return lc->HandleOpen(req);
		case H245_RequestMessage::e_closeLogicalChannel:
		//Loop Request
		case H245_RequestMessage::e_maintenanceLoopRequest:
			return loop->HandleRequest(req);

		//More... 
		case H245_RequestMessage::e_nonStandard:		
		case H245_RequestMessage::e_requestChannelClose:
		case H245_RequestMessage::e_requestMultiplexEntry:
		case H245_RequestMessage::e_requestMode:
		case H245_RequestMessage::e_communicationModeRequest:
		case H245_RequestMessage::e_conferenceRequest:
		case H245_RequestMessage::e_multilinkRequest:
		case H245_RequestMessage::e_logicalChannelRateRequest:
		case H245_RequestMessage::e_genericRequest:
  			return 0;
	}

	return 1;
}

int H245Connection::OnH245Response(H245_ResponseMessage& rep)
{
	//Depending on the tag
	switch(rep.GetTag())
	{
		//MasterSlaveDetermination
		case H245_ResponseMessage::e_masterSlaveDeterminationAck:
			return ms->HandleAck(rep);
		case H245_ResponseMessage::e_masterSlaveDeterminationReject:
			return ms->HandleReject(rep);
		//TerminalCapabilities
		case H245_ResponseMessage::e_terminalCapabilitySetAck:
			return tc->HandleAck(rep);
		case H245_ResponseMessage::e_terminalCapabilitySetReject:
			return tc->HandleAck(rep);
		//RoundTrip
		case H245_ResponseMessage::e_roundTripDelayResponse:
			return rt->HandleResponse(rep);
		//MultiplexEntry
		case H245_ResponseMessage::e_multiplexEntrySendAck:
			return mt->HandleAck(rep);
		case H245_ResponseMessage::e_multiplexEntrySendReject:
			return mt->HandleReject(rep);
		//OpenLogicalChannel
		case H245_ResponseMessage::e_openLogicalChannelAck:
			return lc->HandleOpenAck((H245_OpenLogicalChannelAck&)rep);;
		case H245_ResponseMessage::e_openLogicalChannelReject:
			return lc->HandleOpenAck((H245_OpenLogicalChannelAck&)rep);;
		case H245_ResponseMessage::e_closeLogicalChannelAck:
			return lc->HandleOpenAck((H245_OpenLogicalChannelAck&)rep);;

		//More....
		case H245_ResponseMessage::e_nonStandard:
		case H245_ResponseMessage::e_requestChannelCloseAck:
		case H245_ResponseMessage::e_requestChannelCloseReject:
		case H245_ResponseMessage::e_requestMultiplexEntryAck:
		case H245_ResponseMessage::e_requestMultiplexEntryReject:
		case H245_ResponseMessage::e_requestModeAck:
		case H245_ResponseMessage::e_requestModeReject:
		case H245_ResponseMessage::e_maintenanceLoopAck:
		case H245_ResponseMessage::e_maintenanceLoopReject:
		case H245_ResponseMessage::e_communicationModeResponse:
		case H245_ResponseMessage::e_conferenceResponse:
		case H245_ResponseMessage::e_multilinkResponse:
		case H245_ResponseMessage::e_logicalChannelRateAcknowledge:
		case H245_ResponseMessage::e_logicalChannelRateReject:
			Debug("Unhandled Response\n");
			return 0;
		default:
			Debug("Unknown Response\n");
			return 0;

	}

	return 0;
}

int H245Connection::OnH245Command(H245_CommandMessage& cmd)
{
	Debug("Unknown Command\n");
	return 1;
}

int H245Connection::OnH245Indication(H245_IndicationMessage& ind)
{
	Debug("Unknown Indication\n");
	return 1;
}
