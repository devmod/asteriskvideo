#ifndef _H245CHANNEL_H_
#define _H245CHANNEL_H_

#include "H245.h"
#include "Media.h"

class H245Channel
{
public:
	H245Channel(MediaType type,H245_Capability &cap);
	H245Channel(H245_OpenLogicalChannel & open);

	int BuildChannelPDU(H245_OpenLogicalChannel & open);
	MediaType GetType() {return type;}

private:
	MediaType type;
	H245_Capability *capability;
};

#endif

