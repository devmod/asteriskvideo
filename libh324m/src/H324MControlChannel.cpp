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
#include <iostream>
#include <fstream>
#include "H324MControlChannel.h"
#include "log.h"

const unsigned vID[] = {1,37,111,116,111,114,111,108,97,95,49,0}; //Motorola

H324MControlChannel::H324MControlChannel(H245ChannelsFactory* channels) 
{
	//Save the logical channels factory
	cf = channels;
	//Create the master slave negotiator
	ms = new H245MasterSlave(*this);
	//Create the terminal capabilities exchanger
	tc = new H245TerminalCapability(*this);
	//Create round trip
	rt = new H245RoundTripDelay(*this);
	//Create Multiplex table handler
	mt = new H245MuxTable(*this);
	//Create the logical channel negotiator 
	lc = new H245LogicalChannels(*this);
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
	//Initial state
	state = e_None;

	//Send our first request
	tc->TransferRequest(cf->GetLocalCapabilities());
	//Start master Slave
	ms->Request();

	return true;
}

int H324MControlChannel::OnUserInput(const char*input)
{
	//Enque
	inputList.push_back(strdup(input));
	//Exit
	return true;
}

char* H324MControlChannel::GetUserInput()
{
	//Check size
	if (inputList.size()==0)
		return NULL;
	//Get input
	char *input = inputList.front();
	//Remove
	inputList.pop_front();
	//Return input
	return input;
}

int H324MControlChannel::SendUserInput(const char* input)
{
	H324ControlPDU pdu;
	//Create user input
	pdu.BuildUserInputIndication(PString(input));
	//Send
	return WriteControlPDU(pdu);
}

int H324MControlChannel::SendVideoFastUpdatePicture(int channel)
{
	H324ControlPDU pdu;
	//Create command
	pdu.BuilVideoFastUpdatePicture(channel);
	//Send
	return WriteControlPDU(pdu);
}

int H324MControlChannel::MediaSetup()
{
	//Get local capabilities
	H245Capabilities* cap = cf->GetLocalCapabilities();

	//Create H245 channels
	H245Channel audio(e_Audio,cap->amrCap.m_capability,e_al2WithoutSequenceNumbers,false);
	H245Channel video(e_Video,cap->h263Cap.m_capability,e_al2WithoutSequenceNumbers,true);

	//Start opening channels
	lc->EstablishRequest(1,audio);
	lc->EstablishRequest(2,video);

	//Transfer mux table
	return mt->TransferRequest(*cf->GetLocalTable());
}

int H324MControlChannel::Disconnect()
{
	return true;
}

int H324MControlChannel::OnMasterSlaveDetermination(const H245MasterSlave::Event & event)
{
	//Depending on the type
	switch(event.confirm)
	{
		case H245MasterSlave::e_Confirm:
			//Save state
			master = (event.state == H245MasterSlave::e_DeterminedMaster);
			//Finish ms
			state |= e_MasterSlaveConfirmed;
			//If also tc
			if (state & e_CapabilitiesExchanged)
				//Continue with media setup
				MediaSetup();
			return TRUE;
		case H245MasterSlave::e_Indication:
			//Reply
			ms->Request();
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
			//Finish ce
			state |= e_CapabilitiesExchanged;
			//If also ms
			if (state & e_MasterSlaveConfirmed)
				//Continue with media setup
				MediaSetup();
			return TRUE;
		case H245TerminalCapability::e_TransferIndication:
			//Set channel capabilities
			if (cf->SetRemoteCapabilities(event.capabilities))
				//Accept
				tc->TransferResponse(true);
			else
				//Reject
				tc->TransferResponse(false);
			return TRUE;
		case H245TerminalCapability::e_RejectIndication:
			Logger::Debug("TerminalCapability rejected\n");
			return FALSE;
	}
	//Exit
	return FALSE;
}

int H324MControlChannel::OnMultiplexTable(const H245MuxTable::Event &event)
{
	H223MuxTableEntryList list;

	Logger::Debug("-OnMultiplexTable\n");
	switch(event.type)
	{
		case H245MuxTable::e_TransferConfirm:
			//Set event
			cf->OnMuxTableConfirm(*event.entries);
			break;
		case H245MuxTable::e_TransferIndication:
			//Set event
			if (cf->OnMuxTableIndication(*event.muxTable,list))
				//Accept
				mt->TransferResponse(list);
			else
				//Reject
				mt->TransferReject(list);
			break;
		case H245MuxTable::e_TransferReject:
			Logger::Debug("e_TransferReject!!!!\n");
			break;
	}
	return true;
}

int H324MControlChannel::OnLogicalChannel(const H245LogicalChannels::Event &event)
{
	Logger::Debug("-OnLogicalChannel\n");
	switch(event.type)
	{
		case H245LogicalChannels::e_EstablishIndication:
			//Event
			if (cf->OnEstablishIndication(event.number,event.channel))
				//Accept
				lc->EstablishResponse(event.number);
			else
				//Reject
				lc->EstablishReject(event.number);
			return true;
		case H245LogicalChannels::e_EstablishConfirm:
			//Event
			cf->OnEstablishConfirm(event.number);
			return true;
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
	Logger::Debug("-Event!!\n");
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
		case H245Connection::e_LogicalChannelRate:
		case H245Connection::e_ModeRequest:
		case H245Connection::e_RoundTripDelay:
			break;
	}

	//Exits
	return FALSE;
}

int H324MControlChannel::WriteControlPDU(H324ControlPDU & pdu)
{
	Logger::Debug("-WriteControlPDU [%s]\n",(const unsigned char *)pdu.GetTagName());

	//Send pdu to ccsrl layer
	SendPDU(pdu);

	//Exit
	return 1;
}

int H324MControlChannel::OnControlPDU(H324ControlPDU &pdu)
{
	Logger::Debug("-OnControlPDU [%s]\n",(const unsigned char *)pdu.GetTagName());

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
			Logger::Debug("Unknown PDU\n");
			//?????
			return 0;
	} 

	//Exit
	return 1;
}


int H324MControlChannel::OnH245Request(H245_RequestMessage& req)
{
	Logger::Debug("-OnH245Request\n");
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
			return lc->HandleClose(req);
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
	Logger::Debug("-OnH245Response\n");

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
			return lc->HandleReject((H245_OpenLogicalChannelReject&)rep);;
		case H245_ResponseMessage::e_closeLogicalChannelAck:
			return lc->HandleCloseAck((H245_CloseLogicalChannelAck&)rep);;

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
			Logger::Debug("Unhandled Response\n");
			return 0;
		default:
			Logger::Debug("Unknown Response\n");
			return 0;

	}

	return 0;
}

int H324MControlChannel::OnH245Command(H245_CommandMessage& cmd)
{

	Logger::Debug("Unknown Command\n");
	return 1;
}

int H324MControlChannel::OnH245Indication(H245_IndicationMessage& ind)
{
	Logger::Debug("-OnH245Indication\n");

	//Depending on the tag
	switch(ind.GetTag())
	{
		//MasterSlaveDetermination
		case H245_IndicationMessage::e_userInput:
		{
			// Get user indication
			H245_UserInputIndication &uind = (H245_UserInputIndication &)ind;
			// If it's not alphanumeric
			if (uind.GetTag()!=H245_UserInputIndication::e_alphanumeric)
			{
				//Logger::Debug
				Logger::Debug("Unknown user input Indication\n");
				//Exit
				return 0;
			}
			//Get input
			PString input = (PASN_GeneralString&)uind;
			//Handle user input
			return OnUserInput((const char *)input);
		}
		default:
			Logger::Debug("Unknown Indication\n");
	}

	//Exit
	return 1;
}

int H324MControlChannel::OnError(ControlProtocolSource source, const void *str)
{
	switch(source)
	{
		case H245Connection::e_MasterSlaveDetermination:
			Logger::Debug("MasterSlaveDetermination error %s",(char *) str);
			break;
		case H245Connection::e_CapabilityExchange:
			Logger::Debug("CapabilityExchange error %s",(char *) str);
			break;
		case H245Connection::e_RoundTripDelay:
			Logger::Debug("RoundTripDelay error %s",(char *) str);
			break;
		case H245Connection::e_LogicalChannel:
			Logger::Debug("LogicalChannel error %s",(char *) str);
			break;
		case H245Connection::e_LogicalChannelRate:
			Logger::Debug("LogicalChannelRate  error %s",(char *) str);
			break;
		case H245Connection::e_ModeRequest:
			Logger::Debug("ModeRequest error %s",(char *) str);
			break;
		case H245Connection::e_MultiplexTable:
			Logger::Debug("Multiplex error %s",(char *) str);
			break;
	}

	//Exit
	return 1;
}
