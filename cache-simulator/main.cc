/*******************************************************
main.cc
Amro Awad & Yan Solihin
2013
{ajawad,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <list>
#include <vector>
#include <fstream>
#include <sstream>
#include <conio.h>
using namespace std;

#include "cache.h"

void MSI(vector <Cache*> cacheArray, int proc_num, ulong address, char op, int num_proc)
{

	bool C = false;
	for (int i = 0; i<num_proc; i++)
	{
		if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
		{
			C = true;
		}
	}

	//For NULL State
	if (cacheArray[proc_num]->findLine(address) == NULL)
	{	//PrRd --- For Read : INVALID ---> SHARED | FOR Write : INVALID ---> MODIFIED
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			//if((cacheArray[proc_num]-> findLine(address))->getFlags() == INVALID)
			cacheArray[proc_num]->num_of_invalid_to_shared++;
			(cacheArray[proc_num]->findLine(address))->setFlags(SHARED);
			//Generate BusRD to turn modified to shared
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
					{
						cacheArray[i]->num_of_modified_to_shared++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						cacheArray[i]->num_of_flushes++;
						cacheArray[i]->num_of_interventions++;
					}
					(cacheArray[i]->findLine(address))->setFlags(SHARED);

				}
			}
			goto stop;
		}
		else
		{	//Invalid ---> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_invalid_to_modified++;
			//Generates BusRDx to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					(cacheArray[i]->findLine(address))->invalidate();;
					cacheArray[i]->num_of_invalidations++;
					cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					cacheArray[i]->num_of_flushes++;
				}
			}
			goto stop;
		}
	}
	//For INVALID State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == INVALID)
	{	//PrRd --- For Read : INVALID ---> SHARED | FOR Write : INVALID ---> MODIFIED
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(SHARED);
			cacheArray[proc_num]->num_of_invalid_to_shared++;
			goto stop;
		}
		else
		{	//Invalid ---> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_invalid_to_modified++;
			//Generates BusRDx to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					(cacheArray[i]->findLine(address))->invalidate();
					cacheArray[i]->num_of_invalidations++;
				}
			}
			goto stop;
		}
	}
	//For SHARED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == SHARED)
	{
		/*READ*/			if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			//Generate BusRD to turn modified to shared
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
					{
						cacheArray[i]->num_of_modified_to_shared++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						cacheArray[i]->num_of_flushes++;
						cacheArray[i]->num_of_interventions++;
					}
					(cacheArray[i]->findLine(address))->setFlags(SHARED);
				}
			}
			goto stop;
		}
		/*WRITE*/			else
		{	//Shared  ----> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_shared_to_modified++;
			//Generates BusRDx to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;
					}
					(cacheArray[i]->findLine(address))->invalidate();
				}
			}
			goto stop;
		}
	}
	//For MODIFIED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == MODIFIED)
	{
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
		else
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
	}

stop:;
}

void MESI(vector <Cache*> cacheArray, int proc_num, ulong address, char op, int num_proc)
{
	//For INVALID State
	if (cacheArray[proc_num]->findLine(address) == NULL)
	{	//PrRd --- For Read(C) : INVALID ---> SHARED | For Read(!C) : INVALID ---> EXCLUSIVE | FOR Write : INVALID ---> MODIFIED
		bool C = false;
		for (int i = 0; i<num_proc; i++)
		{
			if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
			{
				C = true;
			}
		}
		if (op == 'r')
		{
			if (C == true)
			{
				cacheArray[proc_num]->Access(address, op);
				cacheArray[proc_num]->num_of_invalid_to_shared++;
				(cacheArray[proc_num]->findLine(address))->setFlags(SHARED);
				//Cache to Cache Transfer when Line exists in another processor and State INVALID-->SHARED
				cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
				//Generate BusRD to turn modified to shared
				for (int i = 0; i<num_proc; i++)
				{
					if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
					{
						if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
						{
							cacheArray[i]->num_of_modified_to_shared++;
							cacheArray[i]->num_of_interventions++;
							cacheArray[i]->num_of_flushes++;
							//cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}
						else if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
						{
							cacheArray[i]->num_of_exclusive_to_shared++;
							cacheArray[i]->num_of_interventions++;
							//cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}
						(cacheArray[i]->findLine(address))->setFlags(SHARED);

					}
				}
			}
			else
			{
				cacheArray[proc_num]->Access(address, op);
				cacheArray[proc_num]->num_of_invalid_to_exclusive++;
				(cacheArray[proc_num]->findLine(address))->setFlags(EXCLUSIVE);
				//Generate BusRD to turn modified to shared 
				for (int i = 0; i<num_proc; i++)
				{
					if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
					{
						if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
						{
							cacheArray[i]->num_of_modified_to_shared++;
							cacheArray[i]->num_of_interventions++;
							cacheArray[i]->num_of_flushes++;
						}
						if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
						{
							cacheArray[i]->num_of_exclusive_to_shared++;
							cacheArray[i]->num_of_interventions++;
							//FlushOpt
							//cacheArray[i]->num_of_chache_to_cache_transfer++;
						}
						(cacheArray[i]->findLine(address))->setFlags(SHARED);

					}
				}
			}
			goto stop;
		}
		else
		{	//Invalid ---> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_invalid_to_modified++;
			//Generates BusRDx to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
					{
						cacheArray[i]->num_of_flushes++;
						cacheArray[i]->num_of_invalidations++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					}
					else
					{
						cacheArray[i]->num_of_invalidations++;

					}
					(cacheArray[i]->findLine(address))->invalidate();
				}
			}
			goto stop;
		}
	}
	//For Exclusive State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == EXCLUSIVE)
	{	//PrRd --- For Read : EXCLUSIVE ---> EXCLUSIVE | FOR Write : EXCLUSIVE ---> MODIFIED
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(EXCLUSIVE);
			goto stop;
		}
		else
		{	//EXCLUSIVE ---> MODIFIED
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_exclusive_to_modified++;
			goto stop;
		}
	}
	//For SHARED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == SHARED)
	{
		/*READ*/			if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			goto stop;
		}
		/*WRITE*/			else
		{	//Shared  ----> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_shared_to_modified++;
			//Generates BusUPGR to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;
						//cacheArray[i]->num_of_chache_to_cache_transfer++;
					}
					else
					{
						cacheArray[i]->num_of_invalidations++;
					}
					(cacheArray[i]->findLine(address))->invalidate();
				}
			}
			goto stop;
		}
	}
	//For MODIFIED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == MODIFIED)
	{
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
		else
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
	}

stop:;
}

void MOESI(vector <Cache*> cacheArray, int proc_num, ulong address, char op, int num_proc)
{
	//For INVALID State
	if (cacheArray[proc_num]->findLine(address) == NULL)
	{	//PrRd --- For Read(C) : INVALID ---> SHARED | For Read(!C) : INVALID ---> EXCLUSIVE | FOR Write : INVALID ---> MODIFIED
		bool C = false;
		for (int i = 0; i<num_proc; i++)
		{
			if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
			{
				C = true;
			}
		}
		if (op == 'r')
		{
			if (C == true)
			{
				cacheArray[proc_num]->Access(address, op);
				cacheArray[proc_num]->num_of_invalid_to_shared++;
				//Generate BusRD to turn modified to owned
				for (int i = 0; i<num_proc; i++)
				{
					if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
					{	//MODIFIED --> OWNED on BusRD
						if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
						{
							cacheArray[i]->num_of_modified_to_owned++;
							cacheArray[i]->num_of_flushes++;
							(cacheArray[i]->findLine(address))->setFlags(OWNER);
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;

						}
						else if ((cacheArray[i]->findLine(address))->getFlags() == OWNER)
						{
							cacheArray[i]->num_of_flushes++;
							(cacheArray[i]->findLine(address))->setFlags(OWNER);
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}
						//EXCLUSIVE --> SHARED on BusRD
						else if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
						{
							cacheArray[i]->num_of_exclusive_to_shared++;
							cacheArray[i]->num_of_interventions++;
							(cacheArray[i]->findLine(address))->setFlags(SHARED);
							//FlushOpt
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}

					}
				}
			}
			else
			{
				cacheArray[proc_num]->Access(address, op);
				cacheArray[proc_num]->num_of_invalid_to_exclusive++;
				(cacheArray[proc_num]->findLine(address))->setFlags(EXCLUSIVE);
				//Generate BusRD to turn modified to shared 
				for (int i = 0; i<num_proc; i++)
				{
					if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
					{	//MODIFIED --> OWNED on BusRD
						if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
						{
							cacheArray[i]->num_of_modified_to_owned++;
							cacheArray[i]->num_of_flushes++;
							(cacheArray[i]->findLine(address))->setFlags(OWNER);
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}
						else if ((cacheArray[i]->findLine(address))->getFlags() == OWNER)
						{
							cacheArray[i]->num_of_flushes++;
							(cacheArray[i]->findLine(address))->setFlags(OWNER);
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}
						//EXCLUSIVE --> SHARED on BusRD
						else if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
						{
							cacheArray[i]->num_of_exclusive_to_shared++;
							(cacheArray[i]->findLine(address))->setFlags(SHARED);
							//FlushOpt
							cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
						}

					}
				}
			}
			goto stop;
		}
		else
		{	//Invalid ---> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_invalid_to_modified++;
			//Generates BusRDx to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;

					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
					{
						cacheArray[i]->num_of_invalidations++;
						cacheArray[i]->num_of_flushes++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == OWNER)
					{
						cacheArray[i]->num_of_invalidations++;
						cacheArray[i]->num_of_flushes++;
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
					{
						cacheArray[i]->num_of_invalidations++;
						//FlusOpt
						cacheArray[proc_num]->num_of_chache_to_cache_transfer++;
					}
					else
					{
						cacheArray[i]->num_of_invalidations++;
						//cacheArray[i]->num_of_chache_to_cache_transfer++;
					}
					(cacheArray[i]->findLine(address))->invalidate();
				}
			}
			goto stop;
		}
	}
	//For Exclusive State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == EXCLUSIVE)
	{	//PrRd --- For Read : EXCLUSIVE ---> EXCLUSIVE | FOR Write : EXCLUSIVE ---> MODIFIED
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(EXCLUSIVE);
			goto stop;
		}
		else
		{	//EXCLUSIVE ---> MODIFIED
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_exclusive_to_modified++;
			goto stop;
		}
	}
	//For SHARED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == SHARED)
	{
		/*READ*/			if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			goto stop;
		}
		/*WRITE*/			else
		{	//Shared  ----> Modified
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_shared_to_modified++;
			//Generates BusUPGR to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;

					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == OWNER)
					{
						cacheArray[i]->num_of_invalidations++;
					}
					else
					{
						cacheArray[i]->num_of_invalidations++;
					}
					(cacheArray[i]->findLine(address))->invalidate();
					//cacheArray[i]->num_of_chache_to_cache_transfer++;
				}
			}
			goto stop;
		}
	}
	//For MODIFIED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == MODIFIED)
	{
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
		else
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			goto stop;
		}
	}
	//For OWNED State
	if ((cacheArray[proc_num]->findLine(address))->getFlags() == OWNER)
	{
		if (op == 'r')
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(OWNER);
			goto stop;
		}
		else
		{
			cacheArray[proc_num]->Access(address, op);
			(cacheArray[proc_num]->findLine(address))->setFlags(MODIFIED);
			cacheArray[proc_num]->num_of_owned_to_modified++;
			//Generates BusUPGR to invalidate other caches
			for (int i = 0; i<num_proc; i++)
			{
				if (i != proc_num && cacheArray[i]->findLine(address) != NULL)
				{
					if ((cacheArray[i]->findLine(address))->getFlags() == SHARED)
					{
						cacheArray[i]->num_of_shared_to_invalid++;
						cacheArray[i]->num_of_invalidations++;
					}
					else if ((cacheArray[i]->findLine(address))->getFlags() == OWNER)
					{
						cacheArray[i]->num_of_invalidations++;
					}
					else
					{
						cacheArray[i]->num_of_invalidations++;
					}
					(cacheArray[i]->findLine(address))->invalidate();
				}
			}
			goto stop;
		}
	}

stop:;
}



int main(int argc, char *argv[])
{

	ifstream fin;
	FILE * pFile;

	//if(argv[1] == NULL){
	// printf("input format: ");
	//printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
	//exit(0);
	// }

	/*****uncomment the next five lines*****/
	//int cache_size = atoi(argv[1]);
	//int cache_assoc= atoi(argv[2]);
	//int blk_size   = atoi(argv[3]);
	//int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	//	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:MOESI*/
	//	char *fname =  (char *)malloc(20);
	//	fname = argv[6];
	//	list <Cache*> cacheArray;

	int cache_size = 1024;
	int cache_assoc = 8;
	int blk_size = 148;
	int num_processors = 16;/*1, 2, 4, 8*/
	int protocol = 0;	 /*0:MSI, 1:MESI, 2:MOESI*/
	char *fname = (char *)malloc(20);
	fname = "CGad";
	vector <Cache*> cacheArray;


	//****************************************************//
	//*******print out simulator configuration here*******//
	//****************************************************//
	cout << "===== 506 SMP Simulator Configuration =====" << endl;
	cout << "L1_SIZE:                        " << cache_size << endl;
	cout << "L1_ASSOC:                       " << cache_assoc << endl;
	cout << "L1_BLOCKSIZE:                   " << blk_size << endl;
	cout << "NUMBER OF PROCESSORS:           " << num_processors << endl;
	cout << "COHERENCE PROTOCOL:             " << "MSI" << endl;
	cout << "TRACE FILE:                     " << "CGad" << endl;

	//*********************************************//
	//*****create an array of caches here**********//
	//*********************************************//	
	for (int i = 0; i< num_processors; i++)
	{
		Cache* c = new Cache(cache_size, cache_assoc, blk_size);
		cacheArray.push_back(c);
	}

	pFile = fopen("C:/Users/Abhishek/Desktop/smp_cache/smp_cache/CGaw", "r");
	if (pFile == 0)
	{
		printf("Trace file problem\n");
		exit(0);
	}
	else
	{
		///******************************************************************//
		//**read trace file,line by line,each(processor#,operation,address)**//
		//*****propagate each request down through memory hierarchy**********//
		//*****by calling cachesArray[processor#]->Access(...)***************//
		///******************************************************************//
		string line;
		std::ifstream file("C:/Users/Abhishek/Desktop/smp_cache/smp_cache/CGaw");

		string::size_type pos;
		int i = 0;//String Split Counter
		int proc_num;//Processor Number
		char op;// 'w' Write 'r' Read
		ulong address;// Mem Address
		while (getline(file, line))
		{
			istringstream iss(line);
			do
			{
				string sub;
				iss >> sub;
				if (sub != "")
				{
					i++;
					if (i == 1)
					{
						proc_num = atoi(sub.c_str());
					}
					if (i == 2)
					{
						op = sub.at(0);
					}
					if (i == 3)
					{
						address = strtol(sub.c_str(), NULL, 16);
						i = 0;
					}
				}

			} while (iss);

			// Access the cache

			//Implement Protocols
			//MSI
			MSI(cacheArray, proc_num, address, op, num_processors);
			//cacheArray[proc_num]->Access(address,op);
		}
	}
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for (int i = 0; i<num_processors - 2; i++)
	{
		cout << "===== Simulation results (Cache_" << i << ")      =====" << endl;
		cout << "01. number of reads: 				" << cacheArray[i]->getReads() << endl;
		cout << "02. number of read misses: 			" << cacheArray[i]->getRM() << endl;
		cout << "03. number of writes: 				" << cacheArray[i]->getWrites() << endl;
		cout << "04. number of write misses: 			" << cacheArray[i]->getWM() << endl;
		cout << "05. number of write backs:			" << cacheArray[i]->getWB() << endl;
		cout << "06. number of invalid to exclusive (INV->EXC):  " << cacheArray[i]->num_of_invalid_to_exclusive << endl;
		cout << "07. number of invalid to shared (INV->SHD):     " << cacheArray[i]->num_of_invalid_to_shared << endl;
		cout << "08. number of modified to shared (MOD->SHD):    " << cacheArray[i]->num_of_modified_to_shared << endl;
		cout << "09. number of exclusive to shared (EXC->SHD):   " << cacheArray[i]->num_of_exclusive_to_shared << endl;
		cout << "10. number of shared to modified (SHD->MOD):    " << cacheArray[i]->num_of_shared_to_modified << endl;
		cout << "11. number of invalid to modified (INV->MOD):   " << cacheArray[i]->num_of_invalid_to_modified << endl;
		cout << "12. number of exclusive to modified (EXC->MOD): " << cacheArray[i]->num_of_exclusive_to_modified << endl;
		cout << "13. number of owned to modified (OWN->MOD):     " << cacheArray[i]->num_of_owned_to_modified << endl;
		cout << "14. number of modified to owned (MOD->OWN):     " << cacheArray[i]->num_of_modified_to_owned << endl;
		cout << "15. number of shared to invalid (SHD->INV):     " << cacheArray[i]->num_of_shared_to_invalid << endl;
		cout << "16. number of cache to cache transfers:         " << cacheArray[i]->num_of_chache_to_cache_transfer << endl;
		cout << "17. number of interventions:                    " << cacheArray[i]->num_of_interventions << endl;
		cout << "18. number of invalidations:                    " << cacheArray[i]->num_of_invalidations << endl;
		cout << "19. number of flushes:                          " << cacheArray[i]->num_of_flushes << endl;
	}
	getch();
}


