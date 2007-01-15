#include "H324CCSRLayer.h"
#include "crc16.h"
#include <iostream>
#include <fstream>
#define SRP_SRP_COMMAND 249
#define SRP_SRP_RESPONSE 251
#define SRP_NSRP_RESPONSE 247


H324CCSRLayer::H324CCSRLayer() : sdu(255),ccsrl(255)
{
	//Initialize variables
	lastsn = -1;
	sentsn = 0;
	cmdsn = -1;
	cmd = NULL;
	waiting = false;
	isPDU = false;

	//Begin stream encoding
	strm.BeginEncoding();
}

H324CCSRLayer::~H324CCSRLayer()
{

}

void H324CCSRLayer::Send(BYTE b)
{
	//Append byte to stream
	sdu.SetAt(sdu.GetSize(),b);
}

void H324CCSRLayer::SendClosingFlag()
{
	//Check minimum length
	if (sdu.GetSize()<3)
		return;

	//The header
	BYTE header = sdu[0];

	//The sequence number
	BYTE sn;

	//The last field
	BYTE lsField;

	//The crc
	CRC16 crc;

	//Feed sdu buffer to crc
	crc.Add(sdu.GetPointer(),sdu.GetSize()-2);

	//Calculate crc
	WORD crcA = crc.Calc();

	//Get sdu crc
	WORD crcB = (sdu[sdu.GetSize()-1] << 8) | sdu[sdu.GetSize()-2];

	//Check it's good crc
	if (crcA!=crcB)
		goto clean;

	//Depending on the type
	switch(header)
	{
		case SRP_SRP_COMMAND:
			//Check minimum length
			if (sdu.GetSize()<5)
				goto clean;

			//And the sn
			sn = sdu[1];

			Debug("Received SRP_SRP_COMMAND [%d]\n",sn);

			//Send NSRP Response
			SendNSRP(sn);

			//Check for retransmission
			if (sn == lastsn)
				goto clean;

			//Update lastsn
			lastsn = sn;

			//Get he ccsrl header
			lsField = sdu[2];

			//Encue the sdu to the ccsrl stream
			ccsrl.Concatenate(PBYTEArray(sdu.GetPointer()+3,sdu.GetSize()-5));

			//If it's the last ccsrl sdu
			if (lsField)
			{
				//Decode
				H324ControlPDU pdu;
			
				std::fstream flog;
				//flog.open ("c:\\logs\\h245.txt",ios::out|ios::app);
				flog.open ("h245.log",ios::out|ios::app);
	
				flog << "-Receiving\r\n";
				//Decode
				while (!ccsrl.IsAtEnd() && pdu.Decode(ccsrl))
				{
					//Launch event
					OnControlPDU(pdu);
					pdu.PrintOn(flog);
					flog << "\r\n";
				}
				flog.close();

				//Reset the decoder just if something went wrong
				ccsrl.ResetDecoder();

				//Clean 
				ccsrl.SetSize(0);
			}
			break;
		case SRP_NSRP_RESPONSE:
			//Check nsrp field
			if (sdu[1]==cmdsn)
			{
				Debug("Received SRP_NSRP_RESPONSE [%d]\n",sdu[1]);
				//End waiting
				waiting = false;
			} else
				Debug("Received out of order SRP_NSRP_RESPONSE [%d]\n",sdu[1]);
			break;
		case SRP_SRP_RESPONSE:
			Debug("Received SRP_SRP_RESPONSE\n");
			//End waiting
			waiting = false;
			break;
	}

clean:
	//Clean sdu
	sdu.SetSize(0);
}


void H324CCSRLayer::SendNSRP(BYTE sn)
{
	Debug("Sending NSRP [%d]\n",sn);

	//The header
	BYTE header[2];

	//Set the type
	header[0] = SRP_NSRP_RESPONSE;
	header[1] = sn;

		//Create the crc
	CRC16 crc;

	//Add the crc
	crc.Add(header,2);

	//Get the crc
	WORD c = crc.Calc();
	
	//Create the SDU
	H223MuxSDU* rpl = new H223MuxSDU(header,2);

	//Add crc
	rpl->Push(((BYTE*)&c)[0]);
	rpl->Push(((BYTE*)&c)[1]);

	//Enqueue to the end list
	rpls.push_back(rpl);
}

void H324CCSRLayer::SendPDU(H324ControlPDU &pdu)
{
	//Encode pdu
	pdu.Encode(strm);
	//Set flag
	isPDU = true;

	Debug("Encode PDU [%d]\n",strm.GetSize());

	BuildCMD();
}

void H324CCSRLayer::BuildCMD()
{
	//If it's not empty
	if (!isPDU)
		return;

	//Finish encoding
	strm.CompleteEncoding();

	//Get the h245 encoded length
	int pduLen = strm.GetSize();
	int len = 0;
	int packetLen = 0;


	Debug("Sending CMD [%d,%d]\n",sentsn,pduLen);

	//CCSRL partitioning
	while (len<pduLen)
	{
		BYTE lsField;

		//Calculate next packet size
		if (pduLen-len>256)
		{
			//Not last packet
			packetLen = 256;
			lsField = 0x00;
		} else {
			//Last packet
			packetLen = pduLen-len;
			lsField = 0xFF;
		}

		//SRP header
		BYTE header[2];

		//Fill it
		header[0] = SRP_SRP_COMMAND;
		header[1] = sentsn++;

		//Create the SDU
		H223MuxSDU* cmd = new H223MuxSDU(header,2);

		//Calculate checksum
		CRC16 crc;

		//Add to crc
		crc.Add(header,2);

		//Append CCSRL header field
		cmd->Push(lsField);

		//Add to crc
		crc.Add(lsField);

		//Append payload to sdu
		cmd->Push(strm.GetPointer()+len,packetLen);

		//Append payload to crc
		crc.Add(strm.GetPointer()+len,packetLen);

		//Get the crc
		WORD c = crc.Calc();
	
		//Add crc
		cmd->Push(((BYTE*)&c)[0]);
		cmd->Push(((BYTE*)&c)[1]);

		//Enqueue the list
		cmds.push_back(cmd);

		//Increment length
		len +=packetLen;
	}

	//Clean stream
	strm.SetSize(0);

	//No cmd
	isPDU = false;

	//Begin encoding
	strm.BeginEncoding();
}

H223MuxSDU* H324CCSRLayer::GetNextPDU()
{
	//No cmd
	isCmd = false;

	//If we have any pending reply
	if(rpls.size()>0)
		return rpls.front();

	//It's a cmd
	isCmd = true;

	//Still waiting for reply?
	if (!waiting)
	{
		//Got response so delete
		if (cmd!=NULL)
		{
			//Delete
			delete cmd;
			//No comand
			cmd = NULL;
		}
		
		//Build pdu
		//BuildCMD();

		//If we don't have elements
		if (cmds.size()==0)
			return NULL;

		//Get first command
		cmd = cmds.front();

		//Remove
		cmds.pop_front();	

		//Update last sequence numbar
		cmdsn = (cmdsn+1)%256;

		//Sending cmd
		Debug("Sending CMD [%d] - %d left\n",cmdsn,cmds.size());
		{
			
			PPER_Stream aux;
			aux.Concatenate(PBYTEArray(cmd->GetPointer()+3,cmd->Length()-5));

			//Decode
			H324ControlPDU pdu;
			
			fstream log;
			//log.open ("c:\\logs\\h245.txt",ios::out|ios::app);
			log.open ("h245.log",ios::out|ios::app);
			log << "-Sending\n";
			//Decode
			while (!aux.IsAtEnd() && pdu.Decode(aux))
			{
				pdu.PrintOn(log);
				log << "\r\n";
			}

			log.close();
		}

		//Wait for reply
		waiting = false;

		//Reset counter
		counter = 0;

	} else{
		//Still waiting for repsonse
		if (counter++<20)
			//Wait for at least 20 to retransmit
			return NULL;

		//Reset counter
		counter = 0;

		//Retransmit
		cmd->Begin();

		//Debug("-Send retry CMD\n");
	}
	
	//Return cmd
	return cmd;
}

void H324CCSRLayer::OnPDUCompleted()
{
	//If it was response
	if (!isCmd)
	{
		//Delete the first element
		delete rpls.front();

		//Remove
		rpls.pop_front();
	} else {
		//Wait for response to delete
		waiting = true;
	}
}

int H324CCSRLayer::OnControlPDU(H324ControlPDU &pdu)
{
	//Exit
	return 1;
}
