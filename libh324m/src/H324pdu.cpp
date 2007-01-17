/*
 * h324pdu.cxx
 *
 * H.324 PDU definitions
 *
 * Open H324 Library
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
 * $Log: h324pdu.cxx,v $
 * Revision 1.146  2005/12/09 05:20:07  csoutheren
 * Added ability to set explicit Q.931 cause code on call end
 *
 * Revision 1.145  2005/11/21 20:59:59  shorne
 * Adjusted Call End Reasons
 *
 * Revision 1.144  2005/09/16 08:12:50  csoutheren
 * Added ability to set timeout for connect
 *
 * Revision 1.143  2005/08/30 08:29:31  csoutheren
 * Added new error code
 *
 * Revision 1.142  2005/03/04 03:21:21  csoutheren
 * Added local and remote addresses to all PDU logs to assist in debugging
 *
 * Revision 1.141  2005/01/04 08:08:45  csoutheren
 * More changes to implement the new configuration methodology, and also to
 * attack the global static problem
 *
 * Revision 1.140  2005/01/03 06:26:09  csoutheren
 * Added extensive support for disabling code modules at compile time
 *
 * Revision 1.139  2004/12/21 23:33:47  csoutheren
 * Fixed #defines for H.460, thanks to Simon Horne
 *
 * Revision 1.138  2004/12/14 06:22:22  csoutheren
 * More OSP implementation
 *
 * Revision 1.137  2004/12/08 01:59:23  csoutheren
 * initial support for Transnexus OSP toolkit
 *
 * Revision 1.136  2004/09/03 01:06:10  csoutheren
 * Added initial hooks for H.460 GEF
 * Thanks to Simon Horne and ISVO (Asia) Pte Ltd. for this contribution
 *
 * Revision 1.135  2004/05/07 06:44:16  csoutheren
 * Fixed problem with empty Q>931 DisplayName
 *
 * Revision 1.134  2004/04/20 05:24:54  csoutheren
 * Added ability to specify Q.931 DisplayName
 *
 * Revision 1.133  2004/04/03 08:28:06  csoutheren
 * Remove pseudo-RTTI and replaced with real RTTI
 *
 * Revision 1.132  2004/04/03 06:56:52  rjongbloed
 * Many and various changes to support new Visual C++ 2003
 *
 * Revision 1.131  2004/02/26 08:32:47  csoutheren
 * Added release complete codes for MCU
 *
 * Revision 1.130  2003/12/11 05:41:00  csoutheren
 * Added storage of H.225 version in endpoint structure
 * Disabled sending RIPs to endpoints that cannot handle them
 *
 * Revision 1.129  2003/05/29 09:19:52  rjongbloed
 * Fixed minor problem with including DisplayName IE if localPartyName
 *   is blank, now does not include it, thanks Auri Vizgaitis
 *
 * Revision 1.128  2003/05/06 06:24:16  robertj
 * Added continuous DTMF tone support (H.245 UserInputIndication - signalUpdate)
 *   as per header documentation, thanks Auri Vizgaitis
 *
 * Revision 1.127  2003/04/10 09:37:20  robertj
 * Added some more functions for converting to alias addresses.
 *
 * Revision 1.126  2003/04/01 03:11:51  robertj
 * Added function to get array of AliasAddress into PStringArray.
 *
 * Revision 1.125  2003/03/25 04:56:21  robertj
 * Fixed issues to do with multiple inheritence in transaction reply cache.
 *
 * Revision 1.124  2003/03/20 01:51:12  robertj
 * More abstraction of H.225 RAS and H.501 protocols transaction handling.
 *
 * Revision 1.123  2003/02/25 06:48:19  robertj
 * More work on PDU transaction abstraction.
 *
 * Revision 1.122  2003/02/21 05:25:45  craigs
 * Abstracted out underlying transports for use with peerelements
 *
 * Revision 1.121  2003/02/03 04:31:05  robertj
 * Added special case for Cisco vendor field, they leave it rather incomplete,
 *
 * Revision 1.120  2003/01/26 02:57:58  craigs
 * Fixed oops in last checkin
 *
 * Revision 1.119  2003/01/26 02:50:38  craigs
 * Change so SETUP PDU uses conference and callIdentifier from H324Connection,
 * rather than both doing seperately and then overwriting
 *
 * Revision 1.118  2002/11/28 04:41:48  robertj
 * Added support for RAS ServiceControlIndication command.
 *
 * Revision 1.117  2002/11/27 06:54:57  robertj
 * Added Service Control Session management as per Annex K/H.324 via RAS
 *   only at this stage.
 * Added H.248 ASN and very primitive infrastructure for linking into the
 *   Service Control Session management system.
 * Added basic infrastructure for Annex K/H.324 HTTP transport system.
 * Added Call Credit Service Control to display account balances.
 *
 * Revision 1.116  2002/11/21 05:21:42  robertj
 * Fixed bug where get lots of zeros at the end of some PDU's
 *
 * Revision 1.115  2002/11/21 04:15:36  robertj
 * Added some changes to help interop with brain dead ASN decoders that do not
 *   know to ignore fields (eg greater than version 2) they do not understand.
 *
 * Revision 1.114  2002/11/13 04:38:17  robertj
 * Added ability to get (and set) Q.931 release complete cause codes.
 *
 * Revision 1.113  2002/11/07 03:50:28  robertj
 * Added extra "congested" Q.931 codes.
 *
 * Revision 1.112  2002/10/31 00:45:22  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.111  2002/10/08 13:08:21  robertj
 * Changed for IPv6 support, thanks Sébastien Josset.
 *
 * Revision 1.110  2002/08/14 01:07:22  robertj
 * Added translation of Q.931 unallocated number release complete code to
 *   OpenH324 EndedByNoUser which is the nearest match.
 *
 * Revision 1.109  2002/08/12 05:38:24  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.108  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.107  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.106  2002/07/31 02:25:04  robertj
 * Fixed translation of some call end reasons for to Q.931 codes.
 *
 * Revision 1.105  2002/07/25 10:55:44  robertj
 * Changes to allow more granularity in PDU dumps, hex output increasing
 *   with increasing trace level.
 *
 * Revision 1.104  2002/07/11 07:04:12  robertj
 * Added build InfoRequest pdu type to RAS.
 *
 * Revision 1.103  2002/06/13 03:59:56  craigs
 * Added codes to progress messages to allow inband audio before connect
 *
 * Revision 1.102  2002/05/29 03:55:21  robertj
 * Added protocol version number checking infrastructure, primarily to improve
 *   interoperability with stacks that are unforgiving of new features.
 *
 * Revision 1.101  2002/05/29 00:03:19  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.100  2002/05/21 09:32:49  robertj
 * Added ability to set multiple alias names ona  connection by connection
 *   basis, defaults to endpoint list, thanks Artis Kugevics
 *
 * Revision 1.99  2002/05/07 03:18:15  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.98  2002/05/03 09:18:49  robertj
 * Added automatic retransmission of RAS responses to retried requests.
 *
 * Revision 1.97  2002/04/24 01:08:09  robertj
 * Added output of RAS pdu sequence number to level 3 trace output.
 *
 * Revision 1.96  2002/03/27 06:04:43  robertj
 * Added Temporary Failure end code for connection, an application may
 *   immediately retry the call if this occurs.
 *
 * Revision 1.95  2002/03/14 07:56:48  robertj
 * Added ability to specify alias type in H324SetAliasAddress, if not specified
 *   then defaults to previous behaviour, thanks Nils Bokerman.
 *
 * Revision 1.94  2002/02/13 07:52:30  robertj
 * Fixed missing parameters on Q.931 calling number, thanks Markus Rydh
 *
 * Revision 1.93  2002/02/01 01:48:45  robertj
 * Some more fixes for T.120 channel establishment, more to do!
 *
 * Revision 1.92  2002/01/18 06:01:23  robertj
 * Added some H324v4 functions (fastConnectRefused & TCS in SETUP)
 *
 * Revision 1.91  2001/12/15 07:10:59  robertj
 * Added functions to get E.164 address from alias address or addresses.
 *
 * Revision 1.90  2001/12/14 08:36:36  robertj
 * More implementation of T.38, thanks Adam Lazur
 *
 * Revision 1.89  2001/12/14 06:38:35  robertj
 * Broke out conversion of Q.850 and H.225 release complete codes to
 *   OpenH324 call end reasons enum.
 *
 * Revision 1.88  2001/12/13 10:56:28  robertj
 * Added build of request in progress pdu.
 *
 * Revision 1.87  2001/10/18 00:58:51  robertj
 * Fixed problem with GetDestinationAlias() adding source aliases instead
 *   of the destination from the setup PDU, thanks Mikael Stolt.
 *
 * Revision 1.86  2001/10/09 06:55:26  robertj
 * Fixed separating destCallSignalAddress fields with tabs in
 *    GetDestinationAlias() function, thanks Lee Kirchhoff
 *
 * Revision 1.85  2001/09/26 07:05:29  robertj
 * Fixed incorrect tags in building some PDU's, thanks Chris Purvis.
 *
 * Revision 1.84  2001/09/14 00:08:20  robertj
 * Optimised H324SetAliasAddress to use IsE164 function.
 *
 * Revision 1.83  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.82  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H324Connection.
 *
 * Revision 1.81  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.80  2001/08/06 03:08:56  robertj
 * Fission of h324.h to h324ep.h & h324con.h, h324.h now just includes files.
 *
 * Revision 1.79  2001/06/14 06:25:16  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.78  2001/06/14 00:45:21  robertj
 * Added extra parameters for Q.931 fields, thanks Rani Assaf
 *
 * Revision 1.77  2001/06/05 03:14:41  robertj
 * Upgraded H.225 ASN to v4 and H.245 ASN to v7.
 *
 * Revision 1.76  2001/05/30 23:34:54  robertj
 * Added functions to send TCS=0 for transmitter side pause.
 *
 * Revision 1.75  2001/05/09 04:07:55  robertj
 * Added more call end codes for busy and congested.
 *
 * Revision 1.74  2001/05/03 06:45:21  robertj
 * Changed trace so dumps PDU if gets an error in decode.
 *
 * Revision 1.73  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.72  2001/03/24 00:58:03  robertj
 * Fixed MSVC warnings.
 *
 * Revision 1.71  2001/03/24 00:34:49  robertj
 * Added read/write hook functions so don't have to duplicate code in
 *    H324RasH235PDU descendant class of H324RasPDU.
 *
 * Revision 1.70  2001/03/23 05:38:30  robertj
 * Added PTRACE_IF to output trace if a conditional is TRUE.
 *
 * Revision 1.69  2001/03/02 06:59:59  robertj
 * Enhanced the globally unique identifier class.
 *
 * Revision 1.68  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.67  2001/01/18 06:04:46  robertj
 * Bullet proofed code so local alias can not be empty string. This actually
 *   fixes an ASN PER encoding bug causing an assert.
 *
 * Revision 1.66  2000/10/12 05:11:54  robertj
 * Added trace log if get transport error on writing PDU.
 *
 * Revision 1.65  2000/09/25 06:48:11  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.64  2000/09/22 01:35:51  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.63  2000/09/20 01:50:22  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.62  2000/09/05 01:16:20  robertj
 * Added "security" call end reason code.
 *
 * Revision 1.61  2000/07/15 09:51:41  robertj
 * Changed adding of Q.931 party numbers to only occur in SETUP.
 *
 * Revision 1.60  2000/07/13 12:29:49  robertj
 * Added some more cause codes on release complete,
 *
 * Revision 1.59  2000/07/12 10:20:43  robertj
 * Fixed incorrect tag code in H.245 ModeChange reject PDU.
 *
 * Revision 1.58  2000/07/09 15:21:11  robertj
 * Changed reference to the word "field" to be more correct IE or "Information Element"
 * Fixed return value of Q.931/H.225 PDU read so returns TRUE if no H.225 data in the
 *     User-User IE. Just flag it as empty and continue processing PDU's.
 *
 * Revision 1.57  2000/06/21 23:59:44  robertj
 * Fixed copy/paste error setting Q.931 display name to incorrect value.
 *
 * Revision 1.56  2000/06/21 08:07:47  robertj
 * Added cause/reason to release complete PDU, where relevent.
 *
 * Revision 1.55  2000/06/07 05:48:06  robertj
 * Added call forwarding.
 *
 * Revision 1.54  2000/05/25 01:59:05  robertj
 * Fixed bugs in calculation of GlLobally Uniqie ID according to DCE/H.225 rules.
 *
 * Revision 1.53  2000/05/23 11:32:37  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H324Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.52  2000/05/15 08:38:59  robertj
 * Removed addition of calling/called party number field in Q.931 if there isn't one.
 *
 * Revision 1.51  2000/05/09 12:19:31  robertj
 * Added ability to get and set "distinctive ring" Q.931 functionality.
 *
 * Revision 1.50  2000/05/08 14:07:35  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.49  2000/05/08 05:06:27  robertj
 * Fixed bug in H.245 close logical channel timeout, thanks XuPeili.
 *
 * Revision 1.48  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.47  2000/04/14 17:29:43  robertj
 * Fixed display of error message on timeout when timeouts are not errors.
 *
 * Revision 1.46  2000/04/10 20:39:18  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 * Added function to extract E164 address from Q.931/H.225 PDU.
 *
 * Revision 1.45  2000/03/29 04:42:19  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.44  2000/03/25 02:01:07  robertj
 * Added adjustable caller name on connection by connection basis.
 *
 * Revision 1.43  2000/03/21 01:08:10  robertj
 * Fixed incorrect call reference code being used in originated call.
 *
 * Revision 1.42  2000/02/17 12:07:43  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.41  1999/12/23 22:47:09  robertj
 * Added calling party number field.
 *
 * Revision 1.40  1999/12/11 02:21:00  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.39  1999/11/16 13:21:38  robertj
 * Removed extraneous error trace when doing asynchronous answer call.
 *
 * Revision 1.38  1999/11/15 14:11:29  robertj
 * Fixed trace output stream being put back after setting hex/fillchar modes.
 *
 * Revision 1.37  1999/11/10 23:30:20  robertj
 * Fixed unexpected closing of transport on PDU read error.
 *
 * Revision 1.36  1999/11/01 00:48:31  robertj
 * Added assert for illegal condition in capabilities, must have set if have table.
 *
 * Revision 1.35  1999/10/30 23:48:21  robertj
 * Fixed incorrect PDU type for H225 RAS location request.
 *
 * Revision 1.34  1999/10/29 03:35:06  robertj
 * Fixed setting of unique ID using fake MAC address from Win32 PPP device.
 *
 * Revision 1.33  1999/09/21 14:09:49  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.32  1999/09/10 09:03:01  robertj
 * Used new GetInterfaceTable() function to get ethernet address for UniqueID
 *
 * Revision 1.31  1999/09/10 03:36:48  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.30  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.29  1999/08/31 11:37:30  robertj
 * Fixed problem with apparently randomly losing signalling channel.
 *
 * Revision 1.28  1999/08/25 05:08:14  robertj
 * File fission (critical mass reached).
 *
 * Revision 1.27  1999/08/13 06:34:38  robertj
 * Fixed problem in CallPartyNumber Q.931 encoding.
 * Added field name display to Q.931 protocol.
 *
 * Revision 1.26  1999/08/10 13:14:15  robertj
 * Added Q.931 Called Number field if have "phone number" style destination addres.
 *
 * Revision 1.25  1999/08/10 11:38:03  robertj
 * Changed population of setup UUIE destinationAddress if can be IA5 string.
 *
 * Revision 1.24  1999/07/26 05:10:30  robertj
 * Fixed yet another race condition on connection termination.
 *
 * Revision 1.23  1999/07/16 14:03:52  robertj
 * Fixed bug in Master/Slave negotiation that can cause looping.
 *
 * Revision 1.22  1999/07/16 06:15:59  robertj
 * Corrected semantics for tunnelled master/slave determination in fast start.
 *
 * Revision 1.21  1999/07/16 02:15:30  robertj
 * Fixed more tunneling problems.
 *
 * Revision 1.20  1999/07/15 14:45:36  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.19  1999/07/15 09:08:04  robertj
 * Added extra debugging for if have PDU decoding error.
 *
 * Revision 1.18  1999/07/15 09:04:31  robertj
 * Fixed some fast start bugs
 *
 * Revision 1.17  1999/07/10 02:51:36  robertj
 * Added mutexing in H245 procedures. Also fixed MSD state bug.
 *
 * Revision 1.16  1999/07/09 06:09:50  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.15  1999/06/25 10:25:35  robertj
 * Added maintentance of callIdentifier variable in H.225 channel.
 *
 * Revision 1.14  1999/06/22 13:45:40  robertj
 * Fixed conferenceIdentifier generation algorithm to bas as in spec.
 *
 * Revision 1.13  1999/06/19 15:18:38  robertj
 * Fixed bug in MasterSlaveDeterminationAck pdu has incorrect master/slave state.
 *
 * Revision 1.12  1999/06/14 15:08:40  robertj
 * Added GSM codec class frame work (still no actual codec).
 *
 * Revision 1.11  1999/06/14 06:39:08  robertj
 * Fixed problem with getting transmit flag to channel from PDU negotiator
 *
 * Revision 1.10  1999/06/14 05:15:56  robertj
 * Changes for using RTP sessions correctly in H324 Logical Channel context
 *
 * Revision 1.9  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.8  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.7  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.6  1999/04/26 06:20:22  robertj
 * Fixed bugs in protocol
 *
 * Revision 1.5  1999/04/26 06:14:47  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.4  1999/02/23 11:04:28  robertj
 * Added capability to make outgoing call.
 *
 * Revision 1.3  1999/02/06 09:23:39  robertj
 * BeOS port
 *
 * Revision 1.2  1999/01/16 02:34:57  robertj
 * GNU compiler compatibility.
 *
 * Revision 1.1  1999/01/16 01:30:54  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "H324pdu.h"
#endif

#include "H324pdu.h"

#define new PNEW

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




/////////////////////////////////////////////////////////////////////////////
