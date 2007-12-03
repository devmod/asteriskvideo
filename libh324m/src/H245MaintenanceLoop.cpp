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
#include "H245MaintenanceLoop.h"

H245MaintenanceLoop::H245MaintenanceLoop(H245Connection & con):
	H245Negotiator(con)
{
}

H245MaintenanceLoop::~H245MaintenanceLoop()
{
}

BOOL H245MaintenanceLoop::HandleRequest(const H245_MaintenanceLoopRequest & pdu)
{
	H324ControlPDU reply;

	//Build ack
	H245_MaintenanceLoopAck &ack = reply.Build(H245_ResponseMessage::e_maintenanceLoopAck);


	//Depending on the type
	switch(pdu.m_type.GetTag())
	{
		case H245_MaintenanceLoopRequest_type::e_logicalChannelLoop:
			//Set tag
			ack.m_type.SetTag(H245_MaintenanceLoopAck_type::e_logicalChannelLoop);
			//Get number
			(H245_LogicalChannelNumber &)ack.m_type = (const H245_LogicalChannelNumber &)pdu.m_type;
			break;	
		case H245_MaintenanceLoopRequest_type::e_mediaLoop:
			//Set tag
			ack.m_type.SetTag(H245_MaintenanceLoopAck_type::e_mediaLoop);
			//Get number
			(H245_LogicalChannelNumber &)ack.m_type = (const H245_LogicalChannelNumber &)pdu.m_type;
			break;
		case H245_MaintenanceLoopRequest_type::e_systemLoop:
			//Set tag
			ack.m_type.SetTag(H245_MaintenanceLoopAck_type::e_systemLoop);
			break;
	}

	return connection.WriteControlPDU(reply);
}
