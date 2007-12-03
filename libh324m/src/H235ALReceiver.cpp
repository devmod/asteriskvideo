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
#if 0
#include "H223alreceiver.h"
extern "C" {
	#include "standard.h"
	#include "rcpc.h"
	#include "crc4.h"
	#include "crc8.h"
	#include "crc16.h"
	#include "arq.h"
	#include "uep_rcpcc.h"
	#include "LList.h"
	#include "between.h"
}

#define DEF_MAX_ARQ 3

H223ALReceiver::H223ALReceiver(H223ChannelInfo *nfo)
{
	outgoing_data = mkbuffer(MAX_AL_BUFFER_SIZE, sizeof(byte));
    incoming_data = mkbuffer(MAX_AL_BUFFER_SIZE, sizeof(byte));

    type = nfo->type;
    framed = nfo->framed;
    seq_num = nfo->seq_num;

    rcpc_code = nfo->rcpc_code;
    rcpc_rate = nfo->rcpc_rate;

    crc_bytes = nfo->crc_bytes;

    old_threshold = nfo->old_threshold;

    /* Initialize other parameters */
    max_arq = DEF_MAX_ARQ;
    last_recd = 31;
    last_written = 31;
    initialized = 0;
    bad_payloads = 0;
}

H223ALReceiver::~H223ALReceiver()
{

}


/* 
 * write the byte to the incoming data buffer.
 */
void H223ALReceiver::Send(BYTE b)
{
	bwrite(incoming_data, &b);
}

/*
 * called by mux to signal the end of an AL-PDU.
 *
 * check the crc on the data in incoming buffer (incoming_data).  if it 
 * is good, transfers the data to the outgoing buffer (outgoing_data).  
 */
void H223ALReceiver::SendClosingFlag()
    
{
	byte inbuf[MAX_AL_BUFFER_SIZE];
	byte inbyte;
	byte computedCRC,receivedCRC;
	unsigned short computedCRC16,receivedCRC16;
	int i;
	int len=blevel(incoming_data);
  
	//Check length
	if (len <= 0) 
		return;

	if (type == AL1) 
	{
		/* just transfer incoming buffer to outgoing buffer.. */
		for(i=0;i<len;i++) 
		{
			bread(incoming_data,(void *) &inbyte);
			bwrite(outgoing_data,(void *) &inbyte);
		}
	} else if (type == AL2) {
		/* check one byte crc, if good transfer to outgoing.  if not, put back
		into incoming */
		computedCRC = 0;
		for(i=0;i<len-1;i++) 
		{
			bread(incoming_data,(char *) inbuf+i);
			crc8(&computedCRC,inbuf[i]);
		}
		bread(incoming_data,(char *) inbuf+(len-1));
		receivedCRC = inbuf[len-1];
		if (computedCRC == receivedCRC)
		{ 
			/* put data without crc into outgoing buf */
			Debug("AL receiving good CRC\n");
			for(i=0;i<len-1;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		} else { 
			/* put all data back into incoming_data, bad crc */
			Debug("AL receiving bad CRC\n");
			for(i=0;i<len;i++)
				bwrite(incoming_data,(char *) inbuf + i);
		}
	} else if (type == AL3) {
		/* check two byte crc, if good transfer to outgoing.  if not, put back into
			incoming */
		computedCRC16 = 0;
		for(i=0;i<len-2;i++) 
		{
			bread(incoming_data,(char *) inbuf+i);
			crc16((short *)&computedCRC16,inbuf[i]);
		}
		bread(incoming_data,(char *) inbuf+(len-2));
		bread(incoming_data,(char *) inbuf+(len-1));
		receivedCRC16 = inbuf[len-2];
		receivedCRC16 = (receivedCRC16 << 8) | (inbuf[len-1]);
		if (computedCRC16 == receivedCRC16)
		{ 
			/* put data without crc into outgoing buf */
			Debug("AL receiving good CRC\n");
			for(i=0;i<len-2;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		} else {
			/* bad crc, send it anyway. could modify to send with an error msg */
			Debug("AL receiving bad CRC\n");
			for(i=0;i<len-2;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		}
	}

	//Call the event
	OnIncommingData((BYTE *)outgoing_data->data,blevel(outgoing_data));
}

/*
 * Called by AL user (receiver) to check if there is any data waiting for
 * it in the receive buffer.  Returns 1 if data is there, 0 if not
 */
int H223ALReceiver::Indication()
{
	if (blevel(outgoing_data))
		return 1;
	else
		return 0;
}

/* 
 * called by al_user, which must call al_indication before
 */
BYTE H223ALReceiver::Receive()
{
	BYTE b;

	bread(outgoing_data, &b);

	return b;
}

#endif
