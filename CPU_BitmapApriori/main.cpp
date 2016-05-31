#include "APRIORI/AprioriSets.h"
#include <iostream>
#include <time.h>

int main(int argc, char *argv[])
{

	// input: transaction data	
	// support threshold, e.g., 1% or 0.1% * transaction number

	char *dataFile = "E:\\GPU_Bitmap_Apriori\\data\\fimi\\m_T10I4D10K.bin";
	int minSup = 100;
	char *outputFile = "../output/fimi//m_T10I4D100K_1000.txt";

    AprioriSets a;
	
	// print debug information 
    a.setVerbose();     
	a.setData(dataFile);
    a.setMinSup(minSup);
    a.setOutputSets(outputFile);
    
    clock_t start = clock();
    int sets = a.generateSets();

	cout << "=========================done================================" << endl
		 << "total pattern#: " << sets << endl
		 << "total time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s]" << endl
		 << "Frequent sets written to " << outputFile << endl
		 << "print time: " << a.getPrintTime()/double(CLOCKS_PER_SEC) << endl;
	
	return 0;
}

