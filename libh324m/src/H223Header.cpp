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
#include "H223Header.h"
extern "C" {
#include "golay.h"
}

H223Header::H223Header()
{
	//Init
	Clear();
}

int H223Header::IsComplete()
{
	return (length==3);
}

int H223Header::IsValid()
{
	//Get the values
	mc   = buffer[0] & 0x0F;
	mpl  = (buffer[0] >> 4) | ((buffer[1] & 0x0F) << 4);
	pm   = (buffer[1] >> 4) | (buffer[2] << 4);

	//Calculate the golay code
	DWORD golay = buffer[2] << 16 | buffer[1] << 8 | buffer[0];

	//Decode it
	int code = golay_decode(golay);

	//Chek it
	if (code==-1)
		//Bad header
		return 0;

	//Get the values
	mc  = code & 0x0F;
	mpl = (code >> 4 ) & 0xFF;

	//Good header
	return 1;
}

BYTE H223Header::Append(BYTE b)
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
			buffer[2] = b;
			length = 3;
			break;
		case 3:
			out       = buffer[0];
			buffer[0] = buffer[1];
			buffer[1] = buffer[2];
			buffer[2] = b;
			break;
	}

	//return the byte
	return out;
}

void H223Header::Clear()
{
	//Empty buffers
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;

	//And the length
	length = 0;
}
