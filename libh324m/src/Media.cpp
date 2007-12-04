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
#include <stdlib.h>
#include <string.h>
#include "Media.h"

Frame::Frame(MediaType t,MediaCodec c,BYTE *d,DWORD l)
{
	//Set type & codec & length
	type = t;
	codec = c;
	dataLength = l;
	//Alloc memory
	data = (BYTE*)malloc(dataLength);
	//Copy memory
	memcpy(data,d,dataLength);
}

Frame::~Frame()
{
	//Free memory
	free(data);
}

