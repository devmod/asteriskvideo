#ifndef _H245CHANNEL_H_
#define _H245CHANNEL_H_

#include "H245.h"

class H245Channel
{
public:
	enum Type {
		e_Audio,
		e_Video
	};

	H245Channel(Type type,H245_Capability &cap);
	H245Channel(H245_OpenLogicalChannel & open);

	int BuildChannelPDU(H245_OpenLogicalChannel & open);

private:
	Type type;
	H245_Capability *capability;
};

#endif

