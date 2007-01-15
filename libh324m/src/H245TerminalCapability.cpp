#include "H245TerminalCapability.h"

#define Debug printf

H245TerminalCapability::H245TerminalCapability(H245Connection & con,H245ChannelsFactory & factory)
	: channels(factory),H245Negotiator(con)

{
	//Reset secuence numbers
	inSequenceNumber = -1;
	outSequenceNumber = 0;

	//Idle state
	inState = e_Idle;
	outState = e_Idle;
}

H245TerminalCapability::~H245TerminalCapability() {
}

/** Outgoing CESE SDL
*/

BOOL H245TerminalCapability::TransferRequest()
{
	//Debug
	Debug("H245 TerminalCapabilitySet TransferRequest\n");

	//We are already in progress 
	if (outState != e_Idle) 
		return TRUE;
	
	//Incremente out secuance number
	outSequenceNumber = (outSequenceNumber+1)%256;

	//Set new State
	outState = e_AwaitingResponse;

	//Build PDU
	H324ControlPDU pdu;

	//Set capabilites
	channels.BuildPDU(pdu.BuildTerminalCapabilitySet(outSequenceNumber));

	//Write pdu
	return connection.WriteControlPDU(pdu);
}

BOOL H245TerminalCapability::HandleAck(const H245_TerminalCapabilitySetAck & pdu)
{
	Debug("H245 Received TerminalCapabilitySetAck\n");
	
	//Check sequence number
	if (pdu.m_sequenceNumber != outSequenceNumber)
		//Exit
		return TRUE;

	//If we are idle
	if (outState == e_Idle)
		//Exit
		return FALSE;

	//Reset state
 	outState = e_Idle;

	//Event
	return connection.OnEvent(Event(e_TransferConfirm));
}

BOOL H245TerminalCapability::HandleReject(const H245_TerminalCapabilitySetReject & pdu)
{
	Debug("H245 Received TerminalCapabilitySetReject\n");
	
	//Check sequence number
	if (pdu.m_sequenceNumber != outSequenceNumber)
		//Exit
		return TRUE;

	//If we are idle
	if (outState == e_Idle)
		//Exit
		return FALSE;

	//Reset state
 	outState = e_Idle;

	//Event
	return connection.OnEvent(Event(e_RejectIndication));;
}


/** Incomming CESE SDL
*/
BOOL H245TerminalCapability::HandleIncoming(const H245_TerminalCapabilitySet & pdu)
{
	Debug("H245 Received TerminalCapabilitySet\n");
	
	//Check secuence number
	if (pdu.m_sequenceNumber == inSequenceNumber) 
		return TRUE;	

	//Get incoming sequence number
	inSequenceNumber = pdu.m_sequenceNumber;

	//Set new state
	inState = e_AwaitingResponse;

	//Send indication
	return connection.OnEvent(Event(e_TransferIndication));
	
}

BOOL H245TerminalCapability::TransferResponse()
{
	//If not waiting for response
	if (outState != e_AwaitingResponse)
		return FALSE;

	//Reply
	H324ControlPDU reply;

	//Accept
	reply.BuildTerminalCapabilitySetAck(inSequenceNumber);
	
	//State
	inState = e_Idle;
	
	//Exit
	return connection.WriteControlPDU(reply);
}

BOOL H245TerminalCapability::RejectRequest()
{
	//If not awaiting state
	if (inState == e_AwaitingResponse)
		return FALSE;

	//Reset state
	inState = e_Idle;

	//Send reject
	H324ControlPDU pdu;

	//Build msg
	pdu.BuildTerminalCapabilitySetReject(outSequenceNumber,H245_TerminalCapabilitySetReject_cause::e_unspecified);

	//Send 
	return connection.WriteControlPDU(pdu);
}


BOOL H245TerminalCapability::HandleRelease(const H245_TerminalCapabilitySetRelease & /*pdu*/)
{
	//If idle
	if (outState == e_Idle)
        return FALSE;

	//Reset state
	outState = e_Idle;

	return connection.OnEvent(Event(e_RejectIndication));
}
