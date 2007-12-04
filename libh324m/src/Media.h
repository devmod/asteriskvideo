#ifndef _MEDIA_H_
#define _MEDIA_H_

#include "H324MConfig.h"

enum MediaType
{
	e_Audio = 0,
	e_Video = 1
};

enum MediaCodec
{
	e_AMR	= 0,
	e_H263	= 1
};

enum AdaptationLayer {
	e_unknown,
	e_al1Framed,
	e_al1NotFramed,
	e_al2WithoutSequenceNumbers,
	e_al2WithSequenceNumbers,
	e_al3
};

class Frame
{
public:
	Frame(MediaType type,MediaCodec codec,BYTE *data,DWORD length);
	~Frame();

	MediaType	type;
	MediaCodec	codec;
	BYTE*		data;
	DWORD		dataLength;
};
#endif
