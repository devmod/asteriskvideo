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
#include "H245MasterSlave.h"
#include "log.h"

H245MasterSlave::H245MasterSlave(H245Connection &con):H245Negotiator(con) 
{
	//Initialize seed
	srand(time(NULL));

	//Set initial values
	state = e_Idle;
	determinationNumber = 1 + (DWORD)(rand()*(0x1000000/(RAND_MAX+1.0)));
	terminalType = e_MCUOnly;
}

H245MasterSlave::~H245MasterSlave() {
}

BOOL H245MasterSlave::Request()
{
	Logger::Debug("H245 Request MasterSlaveDetermination\n");

	//If already processing
	if (state!=e_Idle)
		//Restart
		return TRUE;
	
	//Determination request
	state = e_Outgoing;
	retryCount = 1;

	//Build pdu
	H324ControlPDU pdu;

	//Build Master slave request
	pdu.BuildMasterSlaveDetermination(terminalType, determinationNumber);

	//Send
	return connection.WriteControlPDU(pdu);
}

BOOL H245MasterSlave::HandleIncoming(const H245_MasterSlaveDetermination & pdu)
{
	//Debug
	Logger::Debug("H245 MasterSlaveDetermination\n");

	H324ControlPDU reply;

	//Determine the master and slave
	MasterSlaveStatus newStatus = DetermineStatus(pdu.m_terminalType,pdu.m_statusDeterminationNumber);

	//Depending on the state
	switch(state)
	{
		//case e_Incoming:
		case e_Idle:
            		//If it's not indeterminate
			if (newStatus != e_Indeterminate)
			{
				//Save new status
				status = newStatus;
				//Incoming state
				state = e_Incoming;
				//Build ACK
				reply.BuildMasterSlaveDeterminationAck(newStatus==e_DeterminedMaster);				
				//Send msg
				connection.WriteControlPDU(reply);
				//Send
				return connection.OnEvent(Event(e_Indication,status));

			} else {
				//Build reject
				reply.BuildMasterSlaveDeterminationReject(H245_MasterSlaveDeterminationReject_cause::e_identicalNumbers);
				//Send msg
				connection.WriteControlPDU(reply);
				//Send 
				return connection.OnEvent(Event(e_Indication,status));
			}
			break;
		case e_Incoming:
			//Error
			return FALSE;
		case e_Outgoing:
			//If it's not indeterminate
			if (newStatus != e_Indeterminate)
			{
				//Save new status
				status = newStatus;
				//Incoming state
				state = e_Incoming;
				//Build ACK
				reply.BuildMasterSlaveDeterminationAck(newStatus==e_DeterminedMaster);				
				//Send 
				connection.WriteControlPDU(reply);
				//Send event
				return connection.OnEvent(Event(e_Indication,status));

			} else {
				//Check retries
				if (retryCount>100)
				{
					//Reset 
					retryCount = 0;
					//Idle
					state = e_Idle;
					//Error
					return FALSE;
				}

				//Generate another rnd
				determinationNumber = 1 + (DWORD)(rand()*(0x1000000/(RAND_MAX+1.0)));
				//Inc counter
				retryCount++;
				//Build Master slave request
				reply.BuildMasterSlaveDetermination(terminalType, determinationNumber);
				//Send msg
				connection.WriteControlPDU(reply);
				//Send 
				return connection.OnEvent(Event(e_Indication,status));
			}
				
			break;
	}
		
	//Exit
	return false;
}

BOOL H245MasterSlave::HandleAck(const H245_MasterSlaveDeterminationAck & pdu)
{
	Logger::Debug("H245 MasterSlave Ack\n");

	MasterSlaveStatus newStatus;

	//Get new status
	if(pdu.m_decision.GetTag()==H245_MasterSlaveDeterminationAck_decision::e_master)
		//Master
		newStatus = e_DeterminedMaster;
	else
		//Slave
		newStatus = e_DeterminedSlave;

	//Depending on the state
	switch(state)
	{
		case e_Outgoing:
			//Save good state
			status = newStatus;
			//Send event
			connection.OnEvent(Event(e_Confirm,status));
			break;
		case e_Idle:
			return FALSE;
		case e_Incoming:
			//Id it's correct
			if (newStatus != status)
				return connection.OnError(H245Connection::e_MasterSlaveDetermination,"Master/Slave mismatch");
			//Good
			connection.OnEvent(Event(e_Confirm,status));
	}

	//Return to idle
	state = e_Idle;

	//Exit
	return TRUE;
	
}

BOOL H245MasterSlave::HandleReject(const H245_MasterSlaveDeterminationReject & pdu)
{
	
	Logger::Debug("H245 Received MasterSlaveDeterminationReject\n");

	//Reply
	H324ControlPDU reply;

	//Depending on the state
	switch (state) 
	{
		case e_Idle :
			return TRUE;
		case e_Outgoing :
			//Check retries
			if (retryCount>100)
			{
				//Reset 
				retryCount = 0;
				//Idle
				state = e_Idle;
				//Error
				return FALSE;
			}

			//Generate another rnd
			determinationNumber = 1 + (DWORD)(rand()*(0x1000000/(RAND_MAX+1.0)));
			//Inc counter
			retryCount++;
			//Build Master slave request
			reply.BuildMasterSlaveDetermination(terminalType, determinationNumber);
			//Send
			return connection.WriteControlPDU(reply);
		case e_Incoming:
			//Error
			return FALSE;
	}

	//Exit
	return TRUE;
}


BOOL H245MasterSlave::HandleRelease(const H245_MasterSlaveDeterminationRelease & /*pdu*/)
{
	Logger::Debug("H245 Received MasterSlaveDeterminationRelease\n");

	if (state == e_Idle)
		return TRUE;

	state = e_Idle;

	return connection.OnError(H245Connection::e_MasterSlaveDetermination,"Aborted");
}

H245MasterSlave::MasterSlaveStatus H245MasterSlave::getStatus() 
{
	return status;
}

H245MasterSlave::MasterSlaveStatus H245MasterSlave::DetermineStatus(DWORD type,DWORD number)
{
	//Compare terminal tipes
	if (type == (DWORD)terminalType)
	{
		//Get Modulo
		DWORD moduloDiff = (number - determinationNumber)&0xffffff;

		//Check
		if (moduloDiff == 0 || moduloDiff == 0x800000) 
		{
			//Indeterminate
			return  e_Indeterminate;
		} else if (moduloDiff < 0x800000) {
			//Master
			return e_DeterminedMaster;
		} else {
			//Slave
			return e_DeterminedSlave;
		}
	} else if (type < (DWORD)terminalType) {
		//Master
		return e_DeterminedMaster;
	} else {
		//Slave
		return e_DeterminedSlave;
	}
}

