/***************************************************************************
 *   Copyright (C) 2007 by francesco   *
 *   fremmi@ciccio   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef JITTER_H
#define JITTER_H

#include "H223MuxSDU.h"
class jitterBuffer {
	struct env {
		H223MuxSDU *sdu;
		struct env *next;
	};
	int minPackets;
	int minDelay;
	bool wait;
	DWORD ticks;
	int nextPacket;
	struct env *buffer;
	struct env *last;
	int size;
public:
	jitterBuffer(int minPackets, int minDelay);
	~jitterBuffer();
	void Tick(DWORD len);
	void Push(H223MuxSDU *sdu);
	H223MuxSDU *GetSDU(void);
	void SetBuffer(int minPackets, int minDelay);
	int getSize();
};
#endif