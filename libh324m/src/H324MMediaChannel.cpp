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
#include "H324MMediaChannel.h"


int H324MMediaChannel::Init()
{
	return 1;
}
int H324MMediaChannel::End()
{
	//Clean
	if (sender)
		delete sender;
	if (receiver)
		delete receiver;

	return 1;
}


H223ALReceiver*  H324MMediaChannel::GetReceiver()
{
	return receiver;
}
H223ALSender* H324MMediaChannel::GetSender()
{
	return sender;
}

int H324MMediaChannel::SetSenderLayer(int layer)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case 1:
			sender = new H223AL1Sender();
			break;
		case 2:
			sender = new H223AL3Sender();
			break;
		case 3:
			sender = new H223AL3Sender();
			break;
		default:
			return 0;
	}

	return 1;
}

int H324MMediaChannel::SetReceiverLayer(int layer)
{
	//Dependind on the adaptation layer
	switch(layer)
	{
		case 1:
			receiver = new H223AL1Receiver();
			break;
		case 2:
			receiver = new H223AL2Receiver(1);
			break;
		case 3:
			receiver = new H223AL3Receiver();
			break;
		default:
			return 0;
	}

	return 1;
}

