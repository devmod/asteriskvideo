#ifndef _H324MMEDIACHANNEL_H_
#define _H324MMEDIACHANNEL_H_

#include "H324MAL1.h"
#include "H324MAL2.h"
#include "H324MAL3.h"
#include "Media.h"

class H324MMediaChannel :
	public H223SDUListener
{
public:
	enum State {
      e_Released,
      e_AwaitingEstablishment,
      e_Established,
      e_AwaitingRelease,
      e_AwaitingConfirmation,
      e_AwaitingResponse,
      e_NumStates
    };

public:
	H324MMediaChannel(int jitter,int delay);
	virtual ~H324MMediaChannel();

	int Init();
	void Tick(DWORD value);
	void Reset();
	int End();

	H223ALSender*  GetSender();
	H223ALReceiver* GetReceiver();

	//SDUListener interface
	virtual void OnSDU(BYTE* data,DWORD length);

	int SetSenderLayer(AdaptationLayer layer, int segmentable);
	int SetReceiverLayer(AdaptationLayer layer, int segmentable);

	//Methods
	Frame* GetFrame();
	int SendFrame(Frame *frame);

	int localChannel;
	int remoteChannel;
	int isBidirectional;

public:
	MediaType type;
	State state;

private:
	H223ALReceiver *receiver;
	H223ALSender *sender;
	list<Frame*> frameList;
	int	jitterPackets;
	int jitterActive;
	DWORD ticks;
	DWORD minDelay;
	DWORD nextPacket;
};

class H324MAudioChannel : 
	public H324MMediaChannel
{
public:
	H324MAudioChannel(int jitter,int delay);
};

class H324MVideoChannel :
	public H324MMediaChannel
{
public:
	H324MVideoChannel();
};

#endif
