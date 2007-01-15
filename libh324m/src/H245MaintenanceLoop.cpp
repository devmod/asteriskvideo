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
