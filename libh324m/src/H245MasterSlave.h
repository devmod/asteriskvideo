#ifndef __H245_MASTER_SLAVE_
#define __H245_MASTER_SLAVE_

#include "H245Negotiator.h"

class H245MasterSlave:
	public H245Negotiator 
{
public:
	/**Endpoint types.
	*/
	enum TerminalTypes {
		e_Slave = 0,
		e_TerminalOnly = 50,
		e_TerminalAndMC = 70,
		e_GatewayOnly = 60,
		e_GatewayAndMC = 80,
		e_GatewayAndMCWithDataMP = 90,
		e_GatewayAndMCWithAudioMP = 100,
		e_GatewayAndMCWithAVMP = 110,
		e_GatekeeperOnly = 120,
		e_H324Terminal = 128,
		e_GatekeeperWithDataMP = 130,
		e_GatekeeperWithAudioMP = 140,
		e_GatekeeperWithAVMP = 150,
		e_MCUOnly = 160,
		e_MCUWithDataMP = 170,
		e_MCUWithAudioMP = 180,
		e_MCUWithAVMP = 190,
		e_Master = 232

	};

	enum MasterSlaveStatus {
		e_Indeterminate, 
		e_DeterminedMaster, 
		e_DeterminedSlave,
		e_NumStatuses
	};

	enum ConfirmationStatus {
		e_Indication, 
		e_Confirm
	};
	
	/** Events
	*/
	struct Event: public H245Connection::Event
	{
		public:
			Event(ConfirmationStatus c,MasterSlaveStatus s)
			{
				source = H245Connection::e_MasterSlaveDetermination;
				confirm = c;
				state = s;
			};
			MasterSlaveStatus state;
			ConfirmationStatus confirm;
	};


	H245MasterSlave(H245Connection &con);
	virtual ~H245MasterSlave();
	BOOL Request();

	BOOL HandleIncoming(const H245_MasterSlaveDetermination & pdu);
	BOOL HandleAck(const H245_MasterSlaveDeterminationAck & pdu);
	BOOL HandleReject(const H245_MasterSlaveDeterminationReject & pdu);
	BOOL HandleRelease(const H245_MasterSlaveDeterminationRelease & pdu);
	
	MasterSlaveStatus getStatus();
private:
	enum States {
		e_Idle, 
		e_Outgoing, 
		e_Incoming
	};

	MasterSlaveStatus	DetermineStatus(DWORD type,DWORD number);

	DWORD				determinationNumber;
	TerminalTypes		terminalType;
	unsigned			retryCount;
	MasterSlaveStatus	status;
	States				state;

};



#endif
