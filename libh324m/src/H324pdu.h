#ifndef _H324PDU_H_
#define _H324PDU_H_

#include <ptlib.h>
#include <ptlib/sockets.h>
#include "H324MConfig.h"
#include "H245.h"

#define H245_PROTOCOL_VERSION 8 //11

class H324ControlPDU : public H245_MultimediaSystemControlMessage
{
  PCLASSINFO(H324ControlPDU, H245_MultimediaSystemControlMessage);

  public:

    H245_RequestMessage    & Build(H245_RequestMessage   ::Choices request);
    H245_ResponseMessage   & Build(H245_ResponseMessage  ::Choices response);
    H245_CommandMessage    & Build(H245_CommandMessage   ::Choices command);
    H245_IndicationMessage & Build(H245_IndicationMessage::Choices indication);

    H245_MasterSlaveDetermination & BuildMasterSlaveDetermination(
      unsigned terminalType,
      unsigned statusDeterminationNumber
    );
    H245_MasterSlaveDeterminationAck & BuildMasterSlaveDeterminationAck(
      BOOL isMaster
    );
    H245_MasterSlaveDeterminationReject & BuildMasterSlaveDeterminationReject(
      unsigned cause
    );

	H245_TerminalCapabilitySet & BuildTerminalCapabilitySet(
      unsigned sequenceNumber
    );
    H245_TerminalCapabilitySetAck & BuildTerminalCapabilitySetAck(
      unsigned sequenceNumber
    );
    H245_TerminalCapabilitySetReject & BuildTerminalCapabilitySetReject(
      unsigned sequenceNumber,
      unsigned cause
    );

    H245_OpenLogicalChannel & BuildOpenLogicalChannel(
      unsigned forwardLogicalChannelNumber
    );
    H245_RequestChannelClose & BuildRequestChannelClose(
      unsigned channelNumber,
      unsigned reason
    );
    H245_CloseLogicalChannel & BuildCloseLogicalChannel(
      unsigned channelNumber
    );
    H245_OpenLogicalChannelAck & BuildOpenLogicalChannelAck(
      unsigned channelNumber
    );
    H245_OpenLogicalChannelReject & BuildOpenLogicalChannelReject(
      unsigned channelNumber,
      unsigned cause
    );
    H245_OpenLogicalChannelConfirm & BuildOpenLogicalChannelConfirm(
      unsigned channelNumber
    );
    H245_CloseLogicalChannelAck & BuildCloseLogicalChannelAck(
      unsigned channelNumber
    );
    H245_RequestChannelCloseAck & BuildRequestChannelCloseAck(
      unsigned channelNumber
    );
    H245_RequestChannelCloseReject & BuildRequestChannelCloseReject(
      unsigned channelNumber
    );
    H245_RequestChannelCloseRelease & BuildRequestChannelCloseRelease(
      unsigned channelNumber
    );

    H245_RequestMode & BuildRequestMode(
      unsigned sequenceNumber
    );
    H245_RequestModeAck & BuildRequestModeAck(
      unsigned sequenceNumber,
      unsigned response
    );
    H245_RequestModeReject & BuildRequestModeReject(
      unsigned sequenceNumber,
      unsigned cause
    );

    H245_RoundTripDelayRequest & BuildRoundTripDelayRequest(
      unsigned sequenceNumber
    );
    H245_RoundTripDelayResponse & BuildRoundTripDelayResponse(
      unsigned sequenceNumber
    );

    H245_UserInputIndication & BuildUserInputIndication(
      const PString & value
    );
    H245_UserInputIndication & BuildUserInputIndication(
      char tone,               ///< DTMF tone code
      unsigned duration,       ///< Duration of tone in milliseconds
      unsigned logicalChannel, ///< Logical channel number for RTP sync.
      unsigned rtpTimestamp    ///< RTP timestamp in logical channel sync.
    );

    H245_FunctionNotUnderstood & BuildFunctionNotUnderstood(
      const H324ControlPDU & pdu
    );

    H245_EndSessionCommand & BuildEndSessionCommand(
      unsigned reason
    );

    H245_MultiplexEntrySend & BuildMultiplexEntrySend (
      unsigned sequenceNumber
    );

	H245_MultiplexEntrySendAck & BuildMultiplexEntrySendAck(
      unsigned sequenceNumber
    );

    H245_MultiplexEntrySendReject & BuildMultiplexEntrySendReject(
      unsigned sequenceNumber
    );

	H245_MiscellaneousCommand & BuilVideoFastUpdatePicture(
	  unsigned channelNumber
	);

	H245_LogicalChannelRateRequest & BuildLogicalChannelRequest(
			unsigned int seqOrder, 
			unsigned int channel, 
			unsigned int bitRate
		);
	H245_LogicalChannelRateAcknowledge & BuildLogicalChannelRateAck(
			unsigned int seqOrder, 
			unsigned int channel, 
			unsigned int bitRate);

	H245_LogicalChannelRateReject & BuildLogicalChannelRateReject(
			unsigned int seqOrder, 
			unsigned int channel, 
			unsigned int bitRate,
			H245_LogicalChannelRateRejectReason::Choices reason);
};


#endif

