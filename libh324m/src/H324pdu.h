/*
 * h323pdu.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H324 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h324pdu.h,v $
 * Revision 1.70  2005/11/30 13:05:01  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 1.69  2005/06/30 01:57:39  csoutheren
 * Updated to H.245v11
 *
 * Revision 1.68  2005/03/04 03:21:20  csoutheren
 * Added local and remote addresses to all PDU logs to assist in debugging
 *
 * Revision 1.67  2005/01/24 00:10:48  csoutheren
 * Added function to set Q.931 info in PDU, thanks to Paul Nader
 *
 * Revision 1.66  2004/12/14 06:22:21  csoutheren
 * More OSP implementation
 *
 * Revision 1.65  2003/04/10 09:36:52  robertj
 * Added some more functions for converting to alias addresses.
 *
 * Revision 1.64  2003/04/01 03:11:01  robertj
 * Added function to get array of AliasAddress into PStringArray.
 *
 * Revision 1.63  2003/03/25 04:56:17  robertj
 * Fixed issues to do with multiple inheritence in transaction reply cache.
 *
 * Revision 1.62  2003/03/20 01:51:07  robertj
 * More abstraction of H.225 RAS and H.501 protocols transaction handling.
 *
 * Revision 1.61  2003/03/01 00:23:42  craigs
 * New PeerElement implementation
 *
 * Revision 1.60  2003/02/25 06:48:15  robertj
 * More work on PDU transaction abstraction.
 *
 * Revision 1.59  2003/02/21 05:28:39  craigs
 * Factored out code for user with peer elements
 *
 * Revision 1.58  2003/02/01 13:31:14  robertj
 * Changes to support CAT authentication in RAS.
 *
 * Revision 1.57  2002/11/28 04:41:44  robertj
 * Added support for RAS ServiceControlIndication command.
 *
 * Revision 1.56  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.55  2002/09/03 06:19:37  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.54  2002/08/12 05:38:21  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.53  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.52  2002/08/05 05:17:37  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.51  2002/07/25 10:55:40  robertj
 * Changes to allow more granularity in PDU dumps, hex output increasing
 *   with increasing trace level.
 *
 * Revision 1.50  2002/07/11 07:04:12  robertj
 * Added build InfoRequest pdu type to RAS.
 *
 * Revision 1.49  2002/05/29 03:55:17  robertj
 * Added protocol version number checking infrastructure, primarily to improve
 *   interoperability with stacks that are unforgiving of new features.
 *
 * Revision 1.48  2002/05/29 00:03:15  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.47  2002/05/07 03:18:12  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.46  2002/05/03 09:18:45  robertj
 * Added automatic retransmission of RAS responses to retried requests.
 *
 * Revision 1.45  2002/03/14 07:57:02  robertj
 * Added ability to specify alias type in H324SetAliasAddress, if not specified
 *   then defaults to previous behaviour, thanks Nils Bokerman.
 *
 * Revision 1.44  2001/12/15 07:09:56  robertj
 * Added functions to get E.164 address from alias address or addresses.
 *
 * Revision 1.43  2001/12/14 06:38:47  robertj
 * Broke out conversion of Q.850 and H.225 release complete codes to
 *   OpenH324 call end reasons enum.
 *
 * Revision 1.42  2001/12/13 10:56:04  robertj
 * Added build of request in progress pdu.
 *
 * Revision 1.41  2001/08/16 07:49:16  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H324Connection.
 *
 * Revision 1.40  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.39  2001/08/06 07:44:52  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.38  2001/08/06 03:08:11  robertj
 * Fission of h324.h to h324ep.h & h324con.h, h324.h now just includes files.
 *
 * Revision 1.37  2001/06/14 06:25:13  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.36  2001/06/14 00:45:19  robertj
 * Added extra parameters for Q.931 fields, thanks Rani Assaf
 *
 * Revision 1.35  2001/05/30 23:34:54  robertj
 * Added functions to send TCS=0 for transmitter side pause.
 *
 * Revision 1.34  2001/04/11 03:01:27  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.33  2001/03/24 00:34:35  robertj
 * Added read/write hook functions so don't have to duplicate code in
 *    H324RasH235PDU descendant class of H324RasPDU.
 *
 * Revision 1.32  2001/03/21 04:52:40  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.31  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.30  2001/01/19 01:20:38  robertj
 * Added non-const function to get access to Q.931 PDU in H324SignalPDU.
 *
 * Revision 1.29  2000/10/04 05:59:09  robertj
 * Minor reorganisation of the H.245 secondary channel start up to make it simpler
 *    to override its behaviour.
 *
 * Revision 1.28  2000/09/25 06:47:54  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.27  2000/09/22 01:35:02  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.26  2000/07/15 09:50:49  robertj
 * Changed adding of Q.931 party numbers to only occur in SETUP.
 *
 * Revision 1.25  2000/06/21 08:07:39  robertj
 * Added cause/reason to release complete PDU, where relevent.
 *
 * Revision 1.24  2000/05/23 11:32:27  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H324Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.23  2000/05/08 14:07:26  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.22  2000/05/08 05:05:43  robertj
 * Fixed bug in H.245 close logical channel timeout, thanks XuPeili.
 *
 * Revision 1.21  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.20  2000/04/10 20:39:30  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 * Added function to extract E164 address from Q.931/H.225 PDU.
 *
 * Revision 1.19  2000/03/25 02:00:39  robertj
 * Added adjustable caller name on connection by connection basis.
 *
 * Revision 1.18  2000/03/21 01:22:01  robertj
 * Fixed incorrect call reference code being used in originated call.
 *
 * Revision 1.17  1999/12/11 02:20:58  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.16  1999/09/10 03:36:48  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.15  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 * Revision 1.14  1999/08/25 05:07:49  robertj
 * File fission (critical mass reached).
 *
 * Revision 1.13  1999/07/16 06:15:59  robertj
 * Corrected semantics for tunnelled master/slave determination in fast start.
 *
 * Revision 1.12  1999/07/16 02:15:30  robertj
 * Fixed more tunneling problems.
 *
 * Revision 1.11  1999/07/15 14:45:35  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.10  1999/07/10 02:51:53  robertj
 * Added mutexing in H245 procedures.
 *
 * Revision 1.9  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.8  1999/06/25 10:25:35  robertj
 * Added maintentance of callIdentifier variable in H.225 channel.
 *
 * Revision 1.7  1999/06/14 05:15:56  robertj
 * Changes for using RTP sessions correctly in H324 Logical Channel context
 *
 * Revision 1.6  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.5  1999/06/09 05:26:20  robertj
 * Major restructuring of classes.
 *
 * Revision 1.4  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.3  1999/04/26 06:14:47  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.2  1999/01/16 02:35:04  robertj
 * GNi compiler compatibility.
 *
 * Revision 1.1  1999/01/16 01:30:58  robertj
 * Initial revision
 *
 */

#ifndef __OPAL_H324PDU_H
#define __OPAL_H324PDU_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif
#include <ptlib.h>
#include <ptlib/sockets.h>
#include "H245.h"


#define H245_PROTOCOL_VERSION 8 //11

/////////////////////////////////////////////////////////////////////////////

/**Wrapper class for the H324 control channel.
 */
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

    H245_MultiplexEntrySendReject & H324ControlPDU::BuildMultiplexEntrySendReject(
      unsigned sequenceNumber
    );

};



#endif // __OPAL_H324PDU_H


/////////////////////////////////////////////////////////////////////////////
