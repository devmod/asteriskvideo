#include "include/h324m.h"

int main()
{
	int id;
	int i = 0;
	unsigned char aux[160];


	id = H324MSessionCreate();

	H324MSessionInit(id);

	while(i++<100)	
	{
		H324MSessionRead(id,aux,160);
		H324MSessionWrite(id,aux,160);
	}

	H324MSessionEnd(id);

	H324MSessionDestroy(id);

	return 0;
}
