#ifndef _H245CHANNEL_H_
#define _H245CHANNEL_H_

#include "H245.h"
#include "Media.h"

class H245Channel
{
public:
	//Contstructors
	H245Channel(MediaType type,H245_Capability &cap,AdaptationLayer layer=e_al2WithoutSequenceNumbers,int segmentable=true);
	H245Channel(H245_OpenLogicalChannel & open);

	//Methods
	int				BuildChannelPDU(H245_OpenLogicalChannel & open);
	MediaType		GetType() {return type;}
	AdaptationLayer	GetAdaptationLayer();
	int				IsSegmentable();

private:
	MediaType			type;
	H245_Capability*	capability;
	AdaptationLayer		adaptationLayer;
	int					segmentableChannel;
};

#endif

