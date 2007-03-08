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
#include "H223MuxTable.h"

struct MyKey
{
	MyKey(BYTE  mc,BYTE  len)
	{
		lc=mc;
		rc=len;
	}
	BYTE lc;
	BYTE rc;
};

H223MuxTableEntry::H223MuxTableEntry()
{
	//Empty
	fixedLen = 0;
	repeatLen = 0;
	fixed = NULL;
	repeat = NULL;
}

H223MuxTableEntry::H223MuxTableEntry(H223MuxTableEntry* entry)
{
	//Set lengths
	fixedLen = entry->fixedLen;
	repeatLen = entry->repeatLen;

	//Alocate
	fixed = (BYTE*)malloc(fixedLen);
	repeat = (BYTE*)malloc(repeatLen);

	//Fill
	memcpy(fixed,entry->fixed,fixedLen);
	memcpy(repeat,entry->repeat,repeatLen);
}

H223MuxTableEntry::H223MuxTableEntry(const char* f,const char *r)
{
	//Get the length of the fixed part
	fixedLen = strlen(f);

	//Alocate
	fixed = (BYTE*)malloc(fixedLen);

	//Fill
	for (int i=0; i<fixedLen; i++)
		fixed[i]=f[i]-'0';

	//Get the length of the repeating part
	repeatLen = strlen(r);

	//Alocate
	repeat = (BYTE*)malloc(repeatLen);

	//Fill
	for (int i=0; i<repeatLen; i++)
		repeat[i]=r[i]-'0';
}
H223MuxTableEntry::~H223MuxTableEntry()
{
	//Delete
	if (fixed)
		free(fixed);
	if (repeat)
		free(repeat);
}

H223MuxTable::H223MuxTable()
{
	//All tables to null
	for (int i=0;i<16;i++)
		entries[i] = NULL;
}

H223MuxTable::~H223MuxTable()
{
	//Delete al tables
	for (int i=0;i<16;i++)
		if (entries[i])
			delete entries[i];
}
int H223MuxTable::IsSet(int mc)
{
	return entries[mc]!=NULL;
}
int H223MuxTable::SetEntry(int mc,const char* f,const char *r)
{
	//Check mc
	if (mc>=16)
		return 0;

	//If it was assigned
	if (entries[mc])
		//delete it
		delete(entries[mc]);

	//Create a new entrie
	entries[mc] = new H223MuxTableEntry(f,r);

	//good
	return 1;

}

int H223MuxTable::SetEntry(int mc,H223MuxTableEntry *entry)
{
	//Check mc
	if (mc>=16)
		return 0;

	//If it was assigned
	if (entries[mc])
		//delete it
		delete(entries[mc]);

	//Create a new entrie
	entries[mc] = entry;

	//good
	return 1;
}

int H223MuxTable::GetChannel(int mc,int count)
{
	//If the mc is valid
	if (mc>=16 || !entries[mc])
		return -1;

	//If it's in the fixed
	if (count < entries[mc]->fixedLen)
		return entries[mc]->fixed[count];

	//It's in the repeating part
	return entries[mc]->repeat[(count-entries[mc]->fixedLen) % entries[mc]->repeatLen];
}

int H223MuxTable::AppendEntries(H223MuxTable &table,H223MuxTableEntryList &list)
{
	//For each table
	for (int i=0;i<16;i++)
		//If is set
		if (table.entries[i])
		{
			//Set entry
			SetEntry(i,new H223MuxTableEntry(table.entries[i]));
			//Append index to accepted list
			list.push_back(i);
		}

	//Exit
	return 1;
}

H223MuxTable::H223MuxTable(const H245_MultiplexEntrySend & pdu)
{
	//All tables to null
	for (int i=0;i<16;i++)
		entries[i] = NULL;

	//For each descriptor
	for (int i=0;i<pdu.m_multiplexEntryDescriptors.GetSize();i++)
	{
		//Get entry number
		int mc = pdu.m_multiplexEntryDescriptors[i].m_multiplexTableEntryNumber.GetValue();

		//New empty entry
		H223MuxTableEntry * entry = new H223MuxTableEntry();

		//Loop throught element list
		for (int j=0;j<pdu.m_multiplexEntryDescriptors[i].m_elementList.GetSize();j++)
		{
			//Get element
			H245_MultiplexElement &el = pdu.m_multiplexEntryDescriptors[i].m_elementList[j];

			//If it's an element
			if (el.m_type.GetTag()==H245_MultiplexElement_type::e_logicalChannelNumber)
			{	
				//Get element
				int chan = ((PASN_Integer &)((H245_MultiplexElement_type &)el.m_type));

				//If it's fnite
				if (el.m_repeatCount.GetTag()==H245_MultiplexElement_repeatCount::e_finite)
				{
					//Get count
					int rc = ((PASN_Integer &)((H245_MultiplexElement_repeatCount &)el.m_repeatCount)).GetValue();	
					//Realloc fixed
					entry->fixed = (BYTE*)realloc(entry->fixed,entry->fixedLen+rc);
					//Copy channel
					for (int k=entry->fixedLen; k<entry->fixedLen+rc; k++)
						entry->fixed[k] = (BYTE)chan;
					//Increase length
					entry->fixedLen += rc;
				} else {
			 		//Realloc repeat
					entry->repeat = (BYTE*)realloc(entry->repeat,entry->repeatLen+1);
					//Copy channel
					entry->repeat[entry->repeatLen] = (BYTE)chan;
					//Increase length
					entry->repeatLen += 1;
				}
			} else {
				//uffff... nested elements.. 
			}
		}

		//Append entry
		SetEntry(mc,entry);
	}

}

void H223MuxTable::BuildPDU(H245_MultiplexEntrySend & pdu)
{
	//Remove descriptors
	pdu.m_multiplexEntryDescriptors.RemoveAll();

/*
	for (int k=1;k<3;k++)
	{
		//Create an entry
		H245_MultiplexEntryDescriptor des;
		H245_MultiplexElement fixed;

		//Set channel number
		des.m_multiplexTableEntryNumber.SetValue(k);

		//Include elements
		des.IncludeOptionalField(H245_MultiplexEntryDescriptor::e_elementList);

		//Remove elements
		des.m_elementList.RemoveAll();

		//Set type 
		fixed.m_type.SetTag(H245_MultiplexElement_type::e_logicalChannelNumber);

		//And repeat count
		fixed.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_untilClosingFlag);

		//Set channel
		((PASN_Integer &)fixed.m_type.GetObject()) = k;

		//Append Element
		des.m_elementList.Append((PASN_Object*)fixed.Clone());

		//Append descriptor
		pdu.m_multiplexEntryDescriptors.Append((PASN_Object*)des.Clone());
	}

	return;*/

	//For each channel
	for (int i=1;i<16;i++)
		//If we have channel
		if (entries[i])
		{
			//Create an entry
			H245_MultiplexEntryDescriptor des;

			//Set channel number
			des.m_multiplexTableEntryNumber.SetValue(i);

			//Include elements
			des.IncludeOptionalField(H245_MultiplexEntryDescriptor::e_elementList);

			//Remove elements
			des.m_elementList.RemoveAll();

			//If we have fixed part
			if (entries[i]->fixedLen>0)
			{
				//Create fixed element
				H245_MultiplexElement fixed;

				//Set type 
				fixed.m_type.SetTag(H245_MultiplexElement_type::e_subElementList);

				//And repeat count
				fixed.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_finite);

				//Get it
				H245_MultiplexElement_repeatCount &rc = (H245_MultiplexElement_repeatCount &)fixed.m_repeatCount;

				//Get the list for the fixed
				H245_ArrayOf_MultiplexElement &fixedList = fixed.m_type;

				//Remove
				fixedList.RemoveAll();

				//Set to 1
				((PASN_Integer &)rc.GetObject()) = 1;
				
				int repeatc;
				BYTE mc;
				for (int j=0;j<entries[i]->fixedLen;)
				{
					repeatc=0;
					mc=entries[i]->fixed[j] ;
					repeatc++;
					if (j+1)
					while (mc==entries[i]->fixed[j+repeatc])
					{
						repeatc++;
						if ((j+repeatc)==entries[i]->fixedLen)
							break;
					}
					j+=repeatc;

				}
				
				//Loop fixed part
				for (int j=0;j<entries[i]->fixedLen;j++)
				{
					H245_MultiplexElement el;

					//Set type 
					el.m_type.SetTag(H245_MultiplexElement_type::e_logicalChannelNumber);

					//And repeat count
					el.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_finite);

					//Get it
					H245_MultiplexElement_repeatCount &rc = (H245_MultiplexElement_repeatCount &)fixed.m_repeatCount;
					//Get count
					rc = (H245_MultiplexElement_repeatCount &)el.m_repeatCount;

					//Set to rc to 1
					((PASN_Integer &)rc.GetObject()) = 1;

					//Set the channel
					((PASN_Integer &)el.m_type.GetObject()) = entries[i]->fixed[j];

					//Add element
					fixedList.Append((PASN_Object*)el.Clone());
				}
				
				//Append fixed
				des.m_elementList.Append((PASN_Object*)fixed.Clone());
			}

			//If we have repeated part
			if (entries[i]->repeatLen>0)
			{
				//Repeat part element
				H245_MultiplexElement repeat;
				
				list<MyKey> mc_;				

				int repeatc;
				BYTE mc=0;
				MyKey key(mc,mc);
				for (int j=0;j<entries[i]->repeatLen;)
				{
					
					repeatc=0;
					mc=entries[i]->repeat[j] ;
					repeatc++;
					if ((j+1)<entries[i]->repeatLen)
					while (mc==entries[i]->repeat[j+repeatc])
					{
						repeatc++;
						if ((j+repeatc)==entries[i]->repeatLen)
							break;
					}
					key.lc = mc;
					key.rc = repeatc;
					mc_.push_back(key);
					j+=repeatc;
				}



				if (mc_.size()>1)
				{
					//Set type 
					repeat.m_type.SetTag(H245_MultiplexElement_type::e_subElementList);

					//And repeat count
					repeat.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_untilClosingFlag);
					//
					//Get the list for the fixed
					H245_ArrayOf_MultiplexElement &repeatList = repeat.m_type;

					//Remove
					repeatList.RemoveAll();
					while(mc_.size()>0)
					{
						key=mc_.front();
						mc_.pop_front();
						H245_MultiplexElement el;

						//Set type 
						el.m_type.SetTag(H245_MultiplexElement_type::e_logicalChannelNumber);

						//And repeat count
						el.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_finite);
						//Get elem count
						H245_MultiplexElement_repeatCount &rc = (H245_MultiplexElement_repeatCount &)el.m_repeatCount;
						//Set to rc to 1
						((PASN_Integer &)rc.GetObject()) = key.rc ;

						//Set the channel
						((PASN_Integer &)el.m_type.GetObject()) = key.lc ;

						//Add element
						repeatList.Append((PASN_Object*)el.Clone());
					}
				}
				else
				{	
					key=mc_.back();
					//Set type 
					repeat.m_type.SetTag(H245_MultiplexElement_type::e_logicalChannelNumber);

					//And repeat count
					repeat.m_repeatCount.SetTag(H245_MultiplexElement_repeatCount::e_untilClosingFlag);

					//Set channel
					((PASN_Integer &)repeat.m_type.GetObject()) = key.lc  ;
				}
				mc_.clear();

				//Append fixed
				des.m_elementList.Append((PASN_Object*)repeat.Clone());
			}

			//Append descriptor
			pdu.m_multiplexEntryDescriptors.Append((PASN_Object*)des.Clone());
		}
}
