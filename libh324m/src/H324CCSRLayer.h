#ifndef _H324CCSRLAYER_
#define _H324CCSRLAYER_
#include <ptlib.h>
#include "H223AL.h"
#include "H324pdu.h"
#include "H223MuxSDU.h"

#include <list>

class H324CCSRLayer : 
	public H223ALReceiver,
	public H223ALSender
{
public:
	H324CCSRLayer();
	virtual ~H324CCSRLayer();

	//H223ALReceiver interface
	virtual void Send(BYTE b);
	virtual void SendClosingFlag();

	//H223ALSender interface
	virtual H223MuxSDU* GetNextPDU();
	virtual void OnPDUCompleted();
	virtual int IsSegmentable();

	void SendPDU(H324ControlPDU &pdu);
	void SendNSRP(BYTE sn);

	//Events
	virtual int OnControlPDU(H324ControlPDU &pdu);

protected:
	void BuildCMD();

private:
	std::list<H223MuxSDU*> cmds;
	std::list<H223MuxSDU*> rpls;
	PPER_Stream strm;
	H223MuxSDU* cmd;
	PPER_Stream sdu;
	PPER_Stream ccsrl;
	BYTE	lastsn;
	BYTE	sentsn;
	BYTE	cmdsn;
	int	waiting;
	int	isCmd;
	WORD	counter;
	int	isPDU;
	
};

#endif
