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
#include "H324pdu.h"

const unsigned H245_ProtocolID[] = { 0,0,8,245 ,0,H245_PROTOCOL_VERSION };

/////////////////////////////////////////////////////////////////////////////

H245_RequestMessage & H324ControlPDU::Build(H245_RequestMessage::Choices request)
{
  SetTag(e_request);
  H245_RequestMessage & msg = *this;
  msg.SetTag(request);
  return msg;
}


H245_ResponseMessage & H324ControlPDU::Build(H245_ResponseMessage::Choices response)
{
  SetTag(e_response);
  H245_ResponseMessage & resp = *this;
  resp.SetTag(response);
  return resp;
}


H245_CommandMessage & H324ControlPDU::Build(H245_CommandMessage::Choices command)
{
  SetTag(e_command);
  H245_CommandMessage & cmd = *this;
  cmd.SetTag(command);
  return cmd;
}


H245_IndicationMessage & H324ControlPDU::Build(H245_IndicationMessage::Choices indication)
{
  SetTag(e_indication);
  H245_IndicationMessage & ind = *this;
  ind.SetTag(indication);
  return ind;
}


H245_MasterSlaveDetermination & 
      H324ControlPDU::BuildMasterSlaveDetermination(unsigned terminalType,
                                                    unsigned statusDeterminationNumber)
{
  H245_MasterSlaveDetermination & msd = Build(H245_RequestMessage::e_masterSlaveDetermination);
  msd.m_terminalType = terminalType;
  msd.m_statusDeterminationNumber = statusDeterminationNumber;
  return msd;
}


H245_MasterSlaveDeterminationAck &
      H324ControlPDU::BuildMasterSlaveDeterminationAck(BOOL isMaster)
{
  H245_MasterSlaveDeterminationAck & msda = Build(H245_ResponseMessage::e_masterSlaveDeterminationAck);
  msda.m_decision.SetTag(isMaster
                            ? H245_MasterSlaveDeterminationAck_decision::e_slave
                            : H245_MasterSlaveDeterminationAck_decision::e_master);
  return msda;
}


H245_MasterSlaveDeterminationReject &
      H324ControlPDU::BuildMasterSlaveDeterminationReject(unsigned cause)
{
  H245_MasterSlaveDeterminationReject & msdr = Build(H245_ResponseMessage::e_masterSlaveDeterminationReject);
  msdr.m_cause.SetTag(cause);
  return msdr;
}


H245_TerminalCapabilitySet &
      H324ControlPDU::BuildTerminalCapabilitySet(unsigned sequenceNumber)
{
  H245_TerminalCapabilitySet & cap = Build(H245_RequestMessage::e_terminalCapabilitySet);

  cap.m_sequenceNumber = sequenceNumber;
  cap.m_protocolIdentifier.SetValue(H245_ProtocolID, PARRAYSIZE(H245_ProtocolID));

  return cap;
}


H245_TerminalCapabilitySetAck &
      H324ControlPDU::BuildTerminalCapabilitySetAck(unsigned sequenceNumber)
{
  H245_TerminalCapabilitySetAck & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetAck);
  cap.m_sequenceNumber = sequenceNumber;
  return cap;
}


H245_TerminalCapabilitySetReject &
      H324ControlPDU::BuildTerminalCapabilitySetReject(unsigned sequenceNumber,
                                                       unsigned cause)
{
  H245_TerminalCapabilitySetReject & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetReject);
  cap.m_sequenceNumber = sequenceNumber;
  cap.m_cause.SetTag(cause);

  return cap;
}


H245_OpenLogicalChannel &
      H324ControlPDU::BuildOpenLogicalChannel(unsigned forwardLogicalChannelNumber)
{
  H245_OpenLogicalChannel & open = Build(H245_RequestMessage::e_openLogicalChannel);
  open.m_forwardLogicalChannelNumber = forwardLogicalChannelNumber;
  return open;
}


H245_RequestChannelClose &
      H324ControlPDU::BuildRequestChannelClose(unsigned channelNumber,
                                               unsigned reason)
{
  H245_RequestChannelClose & rcc = Build(H245_RequestMessage::e_requestChannelClose);
  rcc.m_forwardLogicalChannelNumber = channelNumber;
  rcc.IncludeOptionalField(H245_RequestChannelClose::e_reason);
  rcc.m_reason.SetTag(reason);
  return rcc;
}


H245_CloseLogicalChannel &
      H324ControlPDU::BuildCloseLogicalChannel(unsigned channelNumber)
{
  H245_CloseLogicalChannel & clc = Build(H245_RequestMessage::e_closeLogicalChannel);
  clc.m_forwardLogicalChannelNumber = channelNumber;
  clc.m_source.SetTag(H245_CloseLogicalChannel_source::e_lcse);
  return clc;
}


H245_OpenLogicalChannelAck &
      H324ControlPDU::BuildOpenLogicalChannelAck(unsigned channelNumber)
{
  H245_OpenLogicalChannelAck & ack = Build(H245_ResponseMessage::e_openLogicalChannelAck);
  ack.m_forwardLogicalChannelNumber = channelNumber;
  return ack;
}


H245_OpenLogicalChannelReject &
      H324ControlPDU::BuildOpenLogicalChannelReject(unsigned channelNumber,
                                                    unsigned cause)
{
  H245_OpenLogicalChannelReject & reject = Build(H245_ResponseMessage::e_openLogicalChannelReject);
  reject.m_forwardLogicalChannelNumber = channelNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_OpenLogicalChannelConfirm &
      H324ControlPDU::BuildOpenLogicalChannelConfirm(unsigned channelNumber)
{
  H245_OpenLogicalChannelConfirm & chan = Build(H245_IndicationMessage::e_openLogicalChannelConfirm);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_CloseLogicalChannelAck &
      H324ControlPDU::BuildCloseLogicalChannelAck(unsigned channelNumber)
{
  H245_CloseLogicalChannelAck & chan = Build(H245_ResponseMessage::e_closeLogicalChannelAck);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_RequestChannelCloseAck &
      H324ControlPDU::BuildRequestChannelCloseAck(unsigned channelNumber)
{
  H245_RequestChannelCloseAck & rcca = Build(H245_ResponseMessage::e_requestChannelCloseAck);
  rcca.m_forwardLogicalChannelNumber = channelNumber;
  return rcca;
}


H245_RequestChannelCloseReject &
      H324ControlPDU::BuildRequestChannelCloseReject(unsigned channelNumber)
{
  H245_RequestChannelCloseReject & rccr = Build(H245_ResponseMessage::e_requestChannelCloseReject);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestChannelCloseRelease &
      H324ControlPDU::BuildRequestChannelCloseRelease(unsigned channelNumber)
{
  H245_RequestChannelCloseRelease & rccr = Build(H245_IndicationMessage::e_requestChannelCloseRelease);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestMode & H324ControlPDU::BuildRequestMode(unsigned sequenceNumber)
{
  H245_RequestMode & rm = Build(H245_RequestMessage::e_requestMode);
  rm.m_sequenceNumber = sequenceNumber;

  return rm;
}


H245_RequestModeAck & H324ControlPDU::BuildRequestModeAck(unsigned sequenceNumber,
                                                          unsigned response)
{
  H245_RequestModeAck & ack = Build(H245_ResponseMessage::e_requestModeAck);
  ack.m_sequenceNumber = sequenceNumber;
  ack.m_response.SetTag(response);
  return ack;
}


H245_RequestModeReject & H324ControlPDU::BuildRequestModeReject(unsigned sequenceNumber,
                                                                unsigned cause)
{
  H245_RequestModeReject & reject = Build(H245_ResponseMessage::e_requestModeReject);
  reject.m_sequenceNumber = sequenceNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_RoundTripDelayRequest &
      H324ControlPDU::BuildRoundTripDelayRequest(unsigned sequenceNumber)
{
  H245_RoundTripDelayRequest & req = Build(H245_RequestMessage::e_roundTripDelayRequest);
  req.m_sequenceNumber = sequenceNumber;
  return req;
}


H245_RoundTripDelayResponse &
      H324ControlPDU::BuildRoundTripDelayResponse(unsigned sequenceNumber)
{
  H245_RoundTripDelayResponse & resp = Build(H245_ResponseMessage::e_roundTripDelayResponse);
  resp.m_sequenceNumber = sequenceNumber;
  return resp;
}


H245_UserInputIndication &
      H324ControlPDU::BuildUserInputIndication(const PString & value)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);
  ind.SetTag(H245_UserInputIndication::e_alphanumeric);
  (PASN_GeneralString &)ind = value;
  return ind;
}


H245_UserInputIndication & H324ControlPDU::BuildUserInputIndication(char tone,
                                                                    unsigned duration,
                                                                    unsigned logicalChannel,
                                                                    unsigned rtpTimestamp)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);

  if (tone != ' ') {
    ind.SetTag(H245_UserInputIndication::e_signal);
    H245_UserInputIndication_signal & sig = ind;

    sig.m_signalType.SetValue(tone);

    if (duration > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signal::e_duration);
      sig.m_duration = duration;
    }

    if (logicalChannel > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signal::e_rtp);
      sig.m_rtp.m_logicalChannelNumber = logicalChannel;
      sig.m_rtp.m_timestamp = rtpTimestamp;
    }
  }
  else {
    ind.SetTag(H245_UserInputIndication::e_signalUpdate);
    H245_UserInputIndication_signalUpdate & sig = ind;

    sig.m_duration = duration;
    if (logicalChannel > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signalUpdate::e_rtp);
      sig.m_rtp.m_logicalChannelNumber = logicalChannel;
    }
  }

  return ind;
}


H245_FunctionNotUnderstood &
      H324ControlPDU::BuildFunctionNotUnderstood(const H324ControlPDU & pdu)
{
  H245_FunctionNotUnderstood & fnu = Build(H245_IndicationMessage::e_functionNotUnderstood);

  switch (pdu.GetTag()) {
    case H245_MultimediaSystemControlMessage::e_request :
      fnu.SetTag(H245_FunctionNotUnderstood::e_request);
      (H245_RequestMessage &)fnu = (const H245_RequestMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_response :
      fnu.SetTag(H245_FunctionNotUnderstood::e_response);
      (H245_ResponseMessage &)fnu = (const H245_ResponseMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_command :
      fnu.SetTag(H245_FunctionNotUnderstood::e_command);
      (H245_CommandMessage &)fnu = (const H245_CommandMessage &)pdu;
      break;
  }

  return fnu;
}


H245_EndSessionCommand & H324ControlPDU::BuildEndSessionCommand(unsigned reason)
{
  H245_EndSessionCommand & end = Build(H245_CommandMessage::e_endSessionCommand);
  end.SetTag(reason);
  return end;
}


H245_MultiplexEntrySend & H324ControlPDU::BuildMultiplexEntrySend (unsigned sequenceNumber)
{
  H245_MultiplexEntrySend & rm = Build(H245_RequestMessage::e_multiplexEntrySend);
  rm.m_sequenceNumber = sequenceNumber;

  return rm;
}


H245_MultiplexEntrySendAck & H324ControlPDU::BuildMultiplexEntrySendAck(unsigned sequenceNumber)
{
  H245_MultiplexEntrySendAck & ack = Build(H245_ResponseMessage::e_multiplexEntrySendAck);
  ack.m_sequenceNumber = sequenceNumber;
  return ack;
}


H245_MultiplexEntrySendReject & H324ControlPDU::BuildMultiplexEntrySendReject(unsigned sequenceNumber)
{
  H245_MultiplexEntrySendReject & reject = Build(H245_ResponseMessage::e_multiplexEntrySendReject);
  reject.m_sequenceNumber = sequenceNumber;
  return reject;
}

H245_MiscellaneousCommand & H324ControlPDU::BuilVideoFastUpdatePicture(unsigned channelNumber)
{
  H245_MiscellaneousCommand & cmd = Build(H245_CommandMessage::e_miscellaneousCommand);
  cmd.m_logicalChannelNumber = channelNumber;
  cmd.m_type = H245_MiscellaneousCommand_type::e_videoFastUpdatePicture;
  return cmd;
}


H245_LogicalChannelRateRequest & H324ControlPDU::BuildLogicalChannelRequest(
	unsigned int seqOrder,
	unsigned int channel, 
	unsigned int bitRate)
{
	H245_LogicalChannelRateRequest & cmd =
		Build(H245_RequestMessage::e_logicalChannelRateRequest);

	cmd.m_sequenceNumber = seqOrder;	
	cmd.m_logicalChannelNumber = channel;
	cmd.m_maximumBitRate = bitRate;
	return cmd;
}

H245_LogicalChannelRateAcknowledge & H324ControlPDU::BuildLogicalChannelRateAck(
		unsigned int seqOrder,
		unsigned int channel, 
		unsigned int bitRate)
{
	H245_LogicalChannelRateAcknowledge & cmd =
		Build(H245_ResponseMessage::e_logicalChannelRateAcknowledge);
		
	cmd.m_sequenceNumber = seqOrder;	
	cmd.m_logicalChannelNumber = channel;
	cmd.m_maximumBitRate = bitRate;
	return cmd;
}

H245_LogicalChannelRateReject &
	H324ControlPDU::BuildLogicalChannelRateReject(
		unsigned int seqOrder,
		unsigned int channel, 
		unsigned int bitRate,
		H245_LogicalChannelRateRejectReason::Choices reason)
{
	H245_LogicalChannelRateReject & cmd =
		Build(H245_ResponseMessage::e_logicalChannelRateReject);
	cmd.m_sequenceNumber        = seqOrder;
	cmd.m_logicalChannelNumber  = channel;
	cmd.m_currentMaximumBitRate = bitRate;
	cmd.IncludeOptionalField(H245_LogicalChannelRateReject::e_currentMaximumBitRate);
	((H245_LogicalChannelRateRejectReason&)cmd.m_rejectReason).SetTag(reason);
	return cmd;
}


	

/////////////////////////////////////////////////////////////////////////////
