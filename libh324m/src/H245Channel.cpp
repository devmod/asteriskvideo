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
#include "H245Channel.h"


H245Channel::H245Channel(MediaType mediaType,H245_Capability &cap,AdaptationLayer layer,int segmentable)
{
	//Set type
	type = mediaType;
	//Clone capability
	capability = (H245_Capability *)cap.Clone();
	//Get adaptation layer
	adaptationLayer = layer;
	//Get segmentable
	segmentableChannel = segmentable;
}

H245Channel::H245Channel(H245_OpenLogicalChannel & open) 
{
	//Get type
	switch(open.m_forwardLogicalChannelParameters.m_dataType.GetTag())
	{
		case H245_DataType::e_audioData:
			{
				//Set type
				type = e_Audio;
				//Get audio type
				H245_AudioCapability &audioCap = (H245_AudioCapability &)open.m_forwardLogicalChannelParameters.m_dataType;
				//Clone
				capability = (H245_Capability *)audioCap.Clone();
			}
			break;
		case H245_DataType::e_videoData:
			{
				//Set type
				type = e_Video;
				//Get audio type
				H245_VideoCapability &videoCap = (H245_VideoCapability &)open.m_forwardLogicalChannelParameters.m_dataType;
				//Clone
				capability = (H245_Capability *)videoCap.Clone();
			}
			break;
	}

	//Get muxer parameters
	H245_H223LogicalChannelParameters & h223 = open.m_forwardLogicalChannelParameters.m_multiplexParameters;

	//Get adaptation layer
	switch(h223.m_adaptationLayerType.GetTag())
	{
		case H245_H223LogicalChannelParameters_adaptationLayerType::e_al1Framed:
			// AL1
			adaptationLayer = e_al1Framed;
			break;
		case H245_H223LogicalChannelParameters_adaptationLayerType::e_al1NotFramed:
			// AL1
			adaptationLayer = e_al1NotFramed;
			break;
		case H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithoutSequenceNumbers:
			// AL2
			adaptationLayer = e_al2WithoutSequenceNumbers;
			break;
		case H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithSequenceNumbers:
			// AL2
			adaptationLayer = e_al2WithSequenceNumbers;
			break;
		case H245_H223LogicalChannelParameters_adaptationLayerType::e_al3:
			// AL3
			adaptationLayer = e_al3;
			break;
		default:
			// uh?
			adaptationLayer = e_unknown;
	}

	//Get segmentable flag
	segmentableChannel = h223.m_segmentableFlag; 
}

int H245Channel::BuildChannelPDU(H245_OpenLogicalChannel & open)
{
	//Depending on media type
	switch(type)
	{
		case e_Video:
			//Set video type
			open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_videoData);
			//Set video capabilities
			(H245_VideoCapability &) open.m_forwardLogicalChannelParameters.m_dataType = (H245_VideoCapability&)*capability;
			break;
		case e_Audio:
			//Set audio type
			open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_audioData);
			//Set audio capabilities
			(H245_AudioCapability &) open.m_forwardLogicalChannelParameters.m_dataType = (H245_AudioCapability&)*capability;
			break;
	}

	//Mux Capabilities
	open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h223LogicalChannelParameters);
	//Set h223
	H245_H223LogicalChannelParameters & h223 = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
	//Depending on the type of muxin
	switch(adaptationLayer)
	{
		case e_al1Framed:
			// AL1
			h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al1Framed);
			break;
		case e_al1NotFramed:
			// AL1
			h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al1NotFramed);
			break;
		case e_al2WithoutSequenceNumbers:
			// AL2
			h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithoutSequenceNumbers);
			break;
		case e_al2WithSequenceNumbers:
			// AL2
			h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al2WithSequenceNumbers);
			break;
		case e_al3:
			// AL3
			h223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al3);
			break;
		case e_unknown:
			// uh?
			return 0;
	}

	//Set segmentable flag
	h223.m_segmentableFlag = segmentableChannel; 

	//Exit
	return 1;
}

AdaptationLayer H245Channel::GetAdaptationLayer()
{
	//Return adaptation layer
	return adaptationLayer;
}

int H245Channel::IsSegmentable()
{
	//Is channel segmentable?
	return segmentableChannel;
}


#if 0
		/*H245_H223LogicalChannelParameters_adaptationLayerType_al3 &al3 = h223.m_adaptationLayerType;
		al3.m_controlFieldOctets = 1;
		al3.m_sendBufferSize = 4231;
		h223.m_segmentableFlag = true;*/

		//Set capabilities
		open.m_reverseLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_videoData);
		open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
		(H245_VideoCapability &) open.m_reverseLogicalChannelParameters.m_dataType = (H245_VideoCapability&)local.h263Cap.m_capability;

		//Mux Capabilities
		open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h223LogicalChannelParameters);
		open.m_reverseLogicalChannelParameters.IncludeOptionalField(H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
		//Set h223
		H245_H223LogicalChannelParameters & rh223 = open.m_reverseLogicalChannelParameters.m_multiplexParameters;
		rh223.m_adaptationLayerType.SetTag(H245_H223LogicalChannelParameters_adaptationLayerType::e_al3);
		H245_H223LogicalChannelParameters_adaptationLayerType_al3 &ral3 = rh223.m_adaptationLayerType;
		ral3.m_controlFieldOctets = 1;
		ral3.m_sendBufferSize = 4231;
		rh223.m_segmentableFlag = true;
#endif
