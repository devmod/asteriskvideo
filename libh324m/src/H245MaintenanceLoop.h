#ifndef _H245MAINTENANCELOOP_H_
#define _H245MAINTENANCELOOP_H_

#include "H245Negotiator.h"

class H245MaintenanceLoop : public H245Negotiator
{

public:
    H245MaintenanceLoop(H245Connection & connection);
	virtual ~H245MaintenanceLoop();

    BOOL HandleRequest(const H245_MaintenanceLoopRequest & pdu);

};


#endif
