#ifndef _H245NEGOTIATOR_H_
#define _H245NEGOTIATOR_H_

#include "H245Connection.h"

class H245Negotiator
{
public:
	H245Negotiator(H245Connection &con);
	~H245Negotiator();

protected:
	H245Connection &connection;
};


#endif

