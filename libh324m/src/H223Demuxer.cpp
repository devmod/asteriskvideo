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
#include <string.h>
#include "H223Demuxer.h"
#define NONE  0
#define HEAD  1
#define PDU   2

#define linesize 104
char line1[linesize];
char line2[linesize];
char line3[linesize];
char *l1;
char *l2;
int num=0;


H223Demuxer::H223Demuxer()
{
}

H223Demuxer::~H223Demuxer()
{
}

int H223Demuxer::SetChannel(int num,H223ALReceiver *receiver)
{
	//Check for null channel
	if (!receiver)
		//Error
		return 0;

	//Save the reciever
	al[num] = receiver;

	return 1;
}

int H223Demuxer::Open(H223MuxTable *table)
{
	//Check for null table
	if (!table)
		//Error
		return 0;

	//The table
	mux = table;

	//Reset state
	state= NONE;

	//Reset flags
	flag.Clear();
	begin.Clear();

	//And header
	header.Clear();

	return true;
}

int H223Demuxer::Close()
{
	//exit
	return 1;
}

void H223Demuxer::StartPDU(H223Flag &flag)
{
	//Copy the flag
	begin = flag;
	//Reset counter
	counter = 0;
	//No channel
	channel = -1;
	
	{l2[-3] = 'f';l2[-2] = 'l';l2[-1] = 'g';}
}

void H223Demuxer::EndPDU(H223Flag &flag)
{
	
	//If the flag is the complement
	if (flag.complement)
	{
		{l2[-6] = 'd';l2[-5] = 'n';l2[-4] = 'e';}
		//if there is channel
		if ((channel!=-1) && (al[channel]!=NULL))
			//Send the closing pdu to the last channel
			al[channel]->SendClosingFlag();
	} else
		{l2[-6] = 'e';l2[-5] = 'n';l2[-4] = 'd';}
}

void H223Demuxer::Demultiplex(BYTE b)
{
	//New line
	if (num % 32 == 0)
	{
		if (num>0)
		{
			int fd = open("h245.log",O_CREAT|O_WRONLY|O_APPEND);
			write(fd,line3,linesize-6);
			if (line3[linesize-5]==' ') {
				write(fd,line2+2,6);
			} else if (line3[linesize-3]!=' ') {
				write(fd,line3+linesize-6,3);
				write(fd,line2+5,3);
			} else {
				write(fd,line3+linesize-6,6);
			}
			memset(line2,' ',8);
			write(fd,"\r\n",2);
			write(fd,line1,linesize);
			write(fd,"\r\n",2);
			memcpy(line3,line2,linesize);
			close(fd);
		} else
			memset(line3,' ',linesize);
		sprintf(line1,"%.8X ",num);
		memset(line2,' ',linesize);
		l1 = line1+8;
		l2 = line2+8;

	}
	sprintf(l1," %.2X",b);
	sprintf(l2,"   ");
	l1+=3;
	l2+=3;
	num++;

	//Depending on the state
	switch(state)
	{
		case NONE:
			//Append the byte to the flag
			flag.Append(b);
			
			//If we have a full header
			if (!flag.IsValid())
				return;
	
			//Start the PDU
			StartPDU(flag);

			//Clear flag
			flag.Clear();

			//Change state
			state = HEAD;
			
			break;
		case HEAD:
			//Append the byte to the header
			header.Append(b);

			//If still don't we have a complete header
			if (!header.IsComplete())
				return;
	
			//Is the header correct?
			if (!header.IsValid())
			{
				//Clean the header
				header.Clear();
				//Reset state
				state = NONE;
			}

			if (header.mpl!=0)
				sprintf(l2-6," t%.1dl%.2x",header.mc,header.mpl);l2[-3]=' ';

			//We have a good header go for the pdu
            state = PDU;

			break;
		case PDU:
			//Check if the buffer of the flag is full or not
			int complete = flag.IsComplete();

			//Append the byte to the flag and get previous one
			BYTE  a = flag.Append(b);

			//Send the byte to the corresponding AL
			if (complete)
				Send(a);

			//If we have a good flag
			if (!flag.IsValid())
				//And return
				return;
			
			//Set end PDU
			EndPDU(flag);

			//Start the next PDU
			StartPDU(flag);

			//Clear flag
			flag.Clear();

			//Clear the header 
			header.Clear();

			//Change state
			state = HEAD;
			break;
	}
}


void H223Demuxer::Send(BYTE b)
{
	sprintf(l2-9," xx");l2[-6]=' ';
	//Get the next channel from the mux table
	channel = mux->GetChannel(header.mc,counter++);

	//Check channel
	if ((channel<0) || (channel>15))
		//Exit
		return;

	sprintf(l2-9," n%.1d",channel);l2[-6]=' ';
	//Get channel
	std::map<int,H223ALReceiver*>::iterator it = al.find(channel);

	//If not found 
	if (it==al.end())
		return;
	sprintf(l2-9," c%.1d",channel);l2[-6]=' ';
	//Get channel
	H223ALReceiver *recv = it->second;

	//If it's not null
	if (recv)
		//Send byte
		recv->Send(b);
}

