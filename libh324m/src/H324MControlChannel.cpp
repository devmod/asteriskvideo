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
#include "H324MControlChannel.h"
#include <iostream>
#include <fstream>

const unsigned vID[] = {1,37,111,116,111,114,111,108,97,95,49,0}; //Motorola

H324MControlChannel::H324MControlChannel(H245ChannelsFactory* channels) 
{
	//Save the logical channels factory
	cf = channels;
	//Create the master slave negotiator
	ms = new H245MasterSlave(*this);
	//Create the terminal capabilities exchanger
	tc = new H245TerminalCapability(*this,*cf);
	//Create round trip
	rt = new H245RoundTripDelay(*this);
	//Create Multiplex table handler
	mt = new H245MuxTable(*this);
	//Create the logical channel negotiator 
	lc = new H245LogicalChannels(*this,*cf);
	//Maintenance loop
	loop = new H245MaintenanceLoop(*this);
}

H324MControlChannel::~H324MControlChannel()
{
	//Delete negotiators
	delete ms;
	delete tc;
	delete rt;
	delete mt;
	delete lc;
	delete loop;
}

int H324MControlChannel::CallSetup()
{
	//Send our first request
	tc->TransferRequest();
	//ms->Request();
	//Exit
	return TRUE;
}

int H324MControlChannel::Disconnect()
{
	return true;
}

int H324MControlChannel::OnError(ControlProtocolSource source, const void *str)
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
		case H245Connection::e_MultiplexTable:
			Debug("Multiplex error %s",(char *) str);
			break;
	}

	//Exit
	return 1;
}

int H324MControlChannel::OnMasterSlaveDetermination(const H245MasterSlave::Event & event)
{
	//Depending on the type
	switch(event.confirm)
	{
		case H245MasterSlave::e_Confirm:
			//Debug
			Debug("-MasterSlave confirmed [%d]\n", event.state);
			return TRUE;
		case H245MasterSlave::e_Indication:
			return TRUE;
	}
	//Exit
	return FALSE;
}

int H324MControlChannel::OnCapabilityExchange(const H245TerminalCapability::Event & event)
{
	//Depending on the type
	switch(event.type)
	{
		case H245TerminalCapability::e_TransferConfirm:
			return TRUE;
		case H245TerminalCapability::e_TransferIndication:
			//Accept
			tc->TransferResponse();
			//Start master Slave
			ms->Request();
			//Exit
			return TRUE;
		case H245TerminalCapability::e_RejectIndication:
			Debug("TerminalCapability rejected\n");
			return FALSE;
	}
	//Exit
	return FALSE;
}

int H324MControlChannel::OnMultiplexTable(const H245MuxTable::Event &event)
{
	return true;
}

int H324MControlChannel::OnLogicalChannel(const H245LogicalChannels::Event &event)
{
	Debug("-OnLogicalChannel\n");
	switch(event.type)
	{
		case H245LogicalChannels::e_EstablishIndication:
			lc->EstablishResponse(event.channel,true);
			Debug("OpenLogicalChannel\n");
			//lc->EstablishRequest(video->localChannel);
			return true;
		case H245LogicalChannels::e_EstablishConfirm:
		{
			//Send table
			H223MuxTable table;
			table.SetEntry(1,"","1");
			//Send
			mt->Send(table);
			return true;
		}
		case H245LogicalChannels::e_ReleaseIndication:
		case H245LogicalChannels::e_ReleaseConfirm:
		case H245LogicalChannels::e_ErrorIndication:
			return true;
	}

	//Exit
	return true;
}

int H324MControlChannel::OnEvent(const H245Connection::Event &event)
{
	Debug("-Event!!\n");
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
	return FALSE;
}

int H324MControlChannel::WriteControlPDU(H324ControlPDU & pdu)
{
	cout << "Sending \n" << pdu << "\n";

	//Send pdu to ccsrl layer
	SendPDU(pdu);

	//Exit
	return 1;
}

int H324MControlChannel::OnControlPDU(H324ControlPDU &pdu)
{
	Debug("OnControlPDU\n");
	
	cout << "Received \n" << pdu << "\n";

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


int H324MControlChannel::OnH245Request(H245_RequestMessage& req)
{
	Debug("-OnH245Request\n");
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

int H324MControlChannel::OnH245Response(H245_ResponseMessage& rep)
{
	Debug("-OnH245Response\n");

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

int H324MControlChannel::OnH245Command(H245_CommandMessage& cmd)
{
	Debug("Unknown Command\n");
	return 1;
}

int H324MControlChannel::OnH245Indication(H245_IndicationMessage& ind)
{
	Debug("Unknown Indication\n");
	return 1;
}
