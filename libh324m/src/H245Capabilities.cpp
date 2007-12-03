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
#include "H245Capabilities.h"

const unsigned amrID[] = {0,0,8,245,1,1,1};

H245Capabilities::H245Capabilities(const H245_TerminalCapabilitySet & pdu)
{
	
}

H245Capabilities::H245Capabilities()
{
	//All media to false
	audioWithAL1 = false;
	audioWithAL2 = false;
	audioWithAL3 = false;
	videoWithAL1 = false;
	videoWithAL2 = false;
	videoWithAL3 = false;

	//Video
	h263Cap.m_capabilityTableEntryNumber = 1;

	h263Cap.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
	h263Cap.m_capability.SetTag(H245_Capability::e_receiveAndTransmitVideoCapability);

	H245_VideoCapability &video = (H245_VideoCapability&)h263Cap.m_capability;

	video.SetTag(H245_VideoCapability::e_h263VideoCapability);
	H245_H263VideoCapability &h263 = (H245_H263VideoCapability&) video;

	h263.IncludeOptionalField(H245_H263VideoCapability::e_qcifMPI);
	h263.m_qcifMPI = 2;
	h263.m_maxBitRate = 520;
	h263.m_advancedPrediction = false;
	h263.m_arithmeticCoding = false;
	h263.m_unrestrictedVector = false;
	h263.m_pbFrames = false;
	h263.m_temporalSpatialTradeOffCapability = false;

	//Audio amr
	amrCap.m_capabilityTableEntryNumber = 2;
	amrCap.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
	amrCap.m_capability.SetTag(H245_Capability::e_receiveAndTransmitAudioCapability);

	H245_AudioCapability& audio = (H245_AudioCapability&)amrCap.m_capability;

	audio.SetTag(H245_AudioCapability::e_genericAudioCapability);
	H245_GenericCapability &amr = (H245_GenericCapability&) audio;

	amr.m_capabilityIdentifier.SetTag(H245_CapabilityIdentifier::e_standard);
	H245_CapabilityIdentifier &amrId = (H245_CapabilityIdentifier&)amr.m_capabilityIdentifier;
	PASN_ObjectId &id = amrId;
	id.SetValue(amrID,PARRAYSIZE(amrID));

	//Bitrate
	amr.IncludeOptionalField(H245_GenericCapability::e_maxBitRate);
	amr.m_maxBitRate = 122;

	//Collapsing
	amr.IncludeOptionalField(H245_GenericCapability::e_collapsing);
	H245_GenericParameter gp;
	gp.m_parameterIdentifier.SetTag(H245_ParameterIdentifier::e_standard);
	gp.m_parameterValue.SetTag(H245_ParameterValue::e_unsignedMin);
	((PASN_Integer&)gp.m_parameterValue) = 1;
	amr.m_collapsing.Append((PASN_Object*)gp.Clone());

	//Audio g723
	g723Cap.m_capabilityTableEntryNumber = 3;
	g723Cap.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
	g723Cap.m_capability.SetTag(H245_Capability::e_receiveAndTransmitAudioCapability);

	H245_AudioCapability& audio2 = (H245_AudioCapability&)g723Cap.m_capability;

	audio2.SetTag(H245_AudioCapability::e_g7231);
	H245_AudioCapability_g7231 &g723 = (H245_AudioCapability_g7231&) audio2;
	g723.m_maxAl_sduAudioFrames = 1;
	g723.m_silenceSuppression = true;

	//User input
	inputCap.m_capabilityTableEntryNumber = 4;
	inputCap.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
	inputCap.m_capability.SetTag(H245_Capability::e_receiveAndTransmitUserInputCapability);

	H245_UserInputCapability& input = (H245_UserInputCapability&)inputCap.m_capability;
	input.SetTag(H245_UserInputCapability::e_iA5String);
	

}

H245Capabilities::~H245Capabilities(void)
{
}


void H245Capabilities::BuildPDU(H245_TerminalCapabilitySet & pdu)
{
	//Set multiplex capability to h223
	pdu.m_multiplexCapability.SetTag(H245_MultiplexCapability::e_h223Capability);

	//Include optional field
	pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_multiplexCapability);

	//Get h223 cap reference
	H245_H223Capability & h223 = pdu.m_multiplexCapability;

	//Set values
	h223.m_transportWithI_frames = false;

	//Set media muxer capabilities
	h223.m_videoWithAL1 = videoWithAL1;
	h223.m_videoWithAL2 = videoWithAL2;
	h223.m_videoWithAL3 = videoWithAL3;

	h223.m_audioWithAL1 = audioWithAL1;
	h223.m_audioWithAL2 = audioWithAL2;
	h223.m_audioWithAL3 = audioWithAL3;

	h223.m_dataWithAL1 = false;
	h223.m_dataWithAL2 = false;
	h223.m_dataWithAL3 = false;

	//Maximum sizes
	h223.m_maximumAl2SDUSize = 1120;
	h223.m_maximumAl3SDUSize = 1120;
	h223.m_maximumDelayJitter = 0;

	
	//NSRP Support
	h223.m_nsrpSupport = true;
	h223.IncludeOptionalField(H245_H223Capability::e_nsrpSupport);

	//Max mux PDU
	h223.m_maxMUXPDUSizeCapability = false;
	h223.IncludeOptionalField(H245_H223Capability::e_maxMUXPDUSizeCapability);

	
	//Set annexes
	h223.m_mobileOperationTransmitCapability.m_h223AnnexA = false;
	h223.m_mobileOperationTransmitCapability.m_h223AnnexADoubleFlag = false;
	h223.m_mobileOperationTransmitCapability.m_h223AnnexB = true;
	h223.m_mobileOperationTransmitCapability.m_h223AnnexBwithHeader = false;
	h223.m_mobileOperationTransmitCapability.m_modeChangeCapability = false;
	h223.IncludeOptionalField(H245_H223Capability::e_mobileOperationTransmitCapability);

	//Include optional field
	pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityTable);

	//Add audio and video
	pdu.m_capabilityTable.RemoveAll();
	pdu.m_capabilityTable.Append((PASN_Object*)h263Cap.Clone());
	pdu.m_capabilityTable.Append((PASN_Object*)amrCap.Clone());
	pdu.m_capabilityTable.Append((PASN_Object*)g723Cap.Clone());
	pdu.m_capabilityTable.Append((PASN_Object*)inputCap.Clone());

	pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityDescriptors);
	pdu.m_capabilityDescriptors.RemoveAll();
	H245_CapabilityDescriptor des;
	des.IncludeOptionalField(H245_CapabilityDescriptor::e_simultaneousCapabilities);
	des.m_capabilityDescriptorNumber = 1;
	des.m_simultaneousCapabilities.RemoveAll();
	H245_AlternativeCapabilitySet set;
	H245_CapabilityTableEntryNumber number;
	
	set.RemoveAll();
	number.SetValue(1);
	set.Append((PASN_Object *)number.Clone());
	des.m_simultaneousCapabilities.Append((PASN_Object *)set.Clone());

	set.RemoveAll();
	number.SetValue(2);
	set.Append((PASN_Object *)number.Clone());
	number.SetValue(3);
	set.Append((PASN_Object *)number.Clone());
	des.m_simultaneousCapabilities.Append((PASN_Object *)set.Clone());

	pdu.m_capabilityDescriptors.RemoveAll();
	pdu.m_capabilityDescriptors.Append((PASN_Object *)des.Clone());

}
