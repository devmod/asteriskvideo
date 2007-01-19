#include "include/h324m.h"

int main()
{
	int idA;
	int idB;
	int i = 0;
	unsigned char aux[160];


	idA = H324MSessionCreate();
	idB = H324MSessionCreate();

	H324MSessionInit(idA);
	H324MSessionInit(idB);

	while(i++<100)	
	{
		H324MSessionWrite(idB,aux,20);
		H324MSessionRead(idA,aux,20);
		H324MSessionWrite(idA,aux,20);
		H324MSessionRead(idB,aux,20);
	}

	H324MSessionEnd(idA);
	H324MSessionEnd(idB);

	H324MSessionDestroy(idA);
	H324MSessionDestroy(idB);

	return 0;
}
