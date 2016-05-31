#include "APRIORI/AprioriSets.h"
#include <iostream>
#include <time.h>
#include "Kernel\GPUMiner_Kernel.h"

int main(int argc, char *argv[])
{
	// input: transaction data	
	// support threshold, e.g., 1% or 0.1% * transaction number
	
	//char *dataFile = "../data/fimi/m_T40I10D100K.bin";
	//char *dataFile = "../data/fimi/r2.bin";
	//char *dataFile = "../data/fimi/m_T10I4D1K.bin";
	//char *dataFile = "E:\\design\\readnewopenMp.bin";
	char *dataFile = "E:\\GPU_Bitmap_Apriori\\data\\fimi\\m_T10I4D100K_new.bin";
	//char *dataFile = "E:\\design\\read_reserve_T10I4D1K.bin";
	//char *dataFile = "../data/fimi/m_D101000.bin";
	int minSup = 1000;
	//char *outputFile = "../output/fimi/T40I10D100K_cpu.txt";
	//char *outputFile = "../output/fimi/T10I4D1K_cpu.txt";
	//char *outputFile = "../output/fimi/r2New1New.txt";
	char *outputFile = "../output/fimi/m_T10I4D100K_result.txt";
	//char *outputFile = "../output/fimi/m_D101000_result.txt";


	int bound = 1000;  // bound <= 1000 in our experiments


    AprioriSets a;
	
	// print debug information 
    a.setVerbose();     
	a.setData(dataFile);
    a.setMinSup(minSup);
    a.setOutputSets(outputFile);
	a.setBound(bound);
    
    clock_t start = clock();
    int sets = a.generateSets();

	cout << "=========================done================================" << endl
		 << "total pattern#: " << sets << endl
		 << "total time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s]" << endl
		 << "Frequent sets written to " << outputFile << endl
		 << "print time: " << a.getPrintTime()/double(CLOCKS_PER_SEC) << endl;


	return 0;
}

