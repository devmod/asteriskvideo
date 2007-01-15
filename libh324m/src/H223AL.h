#ifndef _H223AL_H_
#define _H223AL_H_

#include "H223Const.h"
#include "H223MuxSDU.h"

class H223ALReceiver
{
public:
	virtual void Send(BYTE b)=0;
	virtual void SendClosingFlag()=0;
};

class H223ALSender
{
public:
	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU()=0;
	virtual void OnPDUCompleted()=0;
};

#if 0

typedef enum { NoAL, AL1, AL2, AL3, AL1M, AL2M, AL3M, Custom } H223ALType;

class H223ALSender;
class H223ALReceiver;

class  H223ChannelInfo
{
public:
	H223ALType type;            /* One of the AL types above                 */
	int framed;                 /* Framed mode? (Frame with HDLC flags)      */
	int seq_num;                /* Sequence numbers?                         */
	int rcpc_code;              /* RCPC Code AL-PDU payloads?                */
	int rcpc_rate;              /* RCPC coding rate                          */
	int arq_type;               /* 0=none, 1=typeI, 2=typeII, 3=typeI/II     */
	int crc_bytes;              /* Append this many CRC bytes to each AL-SDU */
	int old_threshold;          /* Threshold to determine an "old" packet    */
};

#define H324ChanelInfoAL1 {AL1,1,1,1,1,3,1,1}

#define DEF_RCPC_RATE 24
#define DEF_CRC_BYTES 2
#define DEF_OLD_THRESH 10

#define MAX_ARQ_BUFFER_SIZE 10
#define MAX_AL_BUFFER_SIZE 100000

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
		// just transfer incoming buffer to outgoing buffer.. 
		for(i=0;i<len;i++) 
		{
			bread(incoming_data,(void *) &inbyte);
			bwrite(outgoing_data,(void *) &inbyte);
		}
	} else if (type == AL2) {
		// check one byte crc, if good transfer to outgoing.  if not, put back into incoming
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
			// put data without crc into outgoing buf 
			Debug("AL receiving good CRC\n");
			for(i=0;i<len-1;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		} else { 
			// put all data back into incoming_data, bad crc
			Debug("AL receiving bad CRC\n");
			for(i=0;i<len;i++)
				bwrite(incoming_data,(char *) inbuf + i);
		}
	} else if (type == AL3) {
		// check two byte crc, if good transfer to outgoing.  if not, put back into	incoming 
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
			// put data without crc into outgoing buf 
			Debug("AL receiving good CRC\n");
			for(i=0;i<len-2;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		} else {
			// bad crc, send it anyway. could modify to send with an error msg 
			Debug("AL receiving bad CRC\n");
			for(i=0;i<len-2;i++)
				bwrite(outgoing_data,(char *) inbuf + i);
		}
	}

	//Call the event
	OnIncommingData((BYTE *)outgoing_data->data,blevel(outgoing_data));
}

#endif
#endif

