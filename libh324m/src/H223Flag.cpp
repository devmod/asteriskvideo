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
#include "H223Flag.h"

BYTE H223Flag::Append(BYTE b)
{
	BYTE out = 0;

	//Enque the byte into the header
	switch(length)
	{
		case 0:
			buffer[0] = b;
			length = 1;
			break;
		case 1:
			buffer[1] = b;
			length = 2;
			break;
		case 2:
			out       = buffer[0];
			buffer[0] = buffer[1];
			buffer[1] = b;
			break;
	}

	//return the byte
	return out;
}

int H223Flag::IsComplete()
{
	//Check length
	return (length==2);
}

int H223Flag::IsValid()
{
	//Check length
	if (length!=2)
		return 0;

	//Check for flag
	if (buffer[0]==0xE1 && buffer[1]==0x4D)
	{
		//Not complement
		complement = 0;
		//Exit
		return 1;
	}

	//Check for negative flag
	if (buffer[0]==(BYTE)(~0xE1) && buffer[1]==(BYTE)(~0x4D))
	{
		//complement
		complement = 1;
		//Exit
		return 1;
	}

	//No flag
	return 0;
}

void H223Flag::Clear()
{
	//Empty buffers
	buffer[0] = 0;
	buffer[1] = 0;

	//And the length
	length = 0;
}

