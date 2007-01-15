#ifndef _H245CAPABILITIES_H_
#define _H245CAPABILITIES_H_

#include "H245.h"

class H245Capabilities
{
public:
	H245Capabilities(const H245_TerminalCapabilitySet & pdu);
	H245Capabilities();

	~H245Capabilities(void);

	void BuildPDU(H245_TerminalCapabilitySet & pdu);

public:
	bool audioWithAL1;
	bool audioWithAL2;
	bool audioWithAL3;
	bool videoWithAL1;
	bool videoWithAL2;
	bool videoWithAL3;
	
public:
	H245_CapabilityTableEntry h263Cap;
	H245_CapabilityTableEntry amrCap;
	H245_CapabilityTableEntry g723Cap;
	H245_CapabilityTableEntry inputCap;
};


#endif
