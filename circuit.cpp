#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include "gencnfformula.h"

using namespace std;

int main () { 
	/*
	Circuit *c = new Circuit("scheme.out");
	cout << c->toString();
	return 0;	
}
*/
/*	ofstream fout;
	CircuitToSAT* cts = new CircuitToSAT("sat.in");	
	fout.open("sat.out");
	fout << cts->toString();
	delete cts;
	fout.close();
*/

	/* check if file "sat.in" is exist */
	FILE *pFile;
	pFile = fopen("sat.in", "r");

	if (pFile == NULL)
	{
		printf("Filie sat.in doesn't exist\n");
		return 0;
	}
	else
	{
		fclose(pFile);	
	}	
	
	freopen("sat.out", "w", stdout);
	CircuitToSAT cts("sat.in");
	printf("%s", cts.toString().c_str());
	
	/* call SAT solver */
	
	freopen("/dev/tty", "w", stdout);
	printf("CONVERTING TO CNF\n");
	sleep(1);	
	printf("DONE\n");
	system("minisat sat.out solver.out");	
	
	
        
        
        


	return 0;
}
