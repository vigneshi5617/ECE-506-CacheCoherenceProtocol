/*******************************************************
                          main.cc
********************************************************/
#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include <istream>
#include <iomanip>                
#include "cache.h"
using namespace std;


// Global cacheLine pointers for use within the function
cacheLine* line = NULL;
cacheLine* otherLine = NULL;


//Dragon
void Dragon(Cache** cacheArray, ulong processor_num, ulong address, char op, ulong num_proc)
{
    // state: null, Sc, Sm, E, Modified 
    /*There are four different states in Dragon protocol
    they are
    state:
    1.null
    2.Sc
    3.Sm
    4.E
    5.Modified */

    // Here we are going to test if there are any copies exists in the system or not . test if any copy exists
    bool copies = false;
    //line = cacheArray[processor_num]->findLine(address);
    for (int i = 0; i < num_proc; i++)
    {
        if (i != processor_num && cacheArray[i]->findLine(address) != NULL && cacheArray[i]->findLine(address)->getFlags() != INVALID)
        {
            copies = true;
        }
    }


    // Lets instatiate the state for null, 
    // For the Read operation in the file we get ,
    // For the read Operation we get the convertion of the states as follows :
    //  null -> SharedClean(C), Exclusive(!C)
    //  write: null -> SharedWrite(C), Modified(!C)
    if (cacheArray[processor_num]->findLine(address) == NULL || cacheArray[processor_num]->findLine(address)->getFlags() == INVALID)
    {
        if (op == 'r')
        {
            cacheArray[processor_num]->Access(address, op);
            if (copies == true)
            {

                (cacheArray[processor_num]->findLine(address))->setFlags(SHARED_CLEAN);

                for (int i = 0; i < num_proc; i++)
                {
                    if (i != processor_num && cacheArray[i]->findLine(address) != NULL && cacheArray[i]->findLine(address)->getFlags() != INVALID)
                    {
                        if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
                        {
                            cacheArray[i]->num_of_interventions++;
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->writeBack(address); // write back 
                            (cacheArray[i]->findLine(address))->setFlags(SHARED_MODIFIED);
                            //cacheArray[i]->num_of_Bus_upd++; //functionally wrong
                            continue;
                        }
                        if ((cacheArray[i]->findLine(address))->getFlags() == SHARED_MODIFIED)
                        {
                            cacheArray[i]->number_of_flushes++; // snoop a busRd, flush, according to FSM
                            cacheArray[i]->writeBack(address); // We will do the write back
                            //cacheArray[i]->num_of_Bus_upd++; //Increase thew Bus update
                        }
                        if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
                        {
                            cacheArray[i]->num_of_interventions++;
                            (cacheArray[i]->findLine(address))->setFlags(SHARED_CLEAN);
                        }
                    }
                }

            }
            else
            {
                // no copy is there we need to  as exclusive state
                //cacheArray[proc_num]->num_of_mem_trans++;
                (cacheArray[processor_num]->findLine(address))->setFlags(EXCLUSIVE);
            }
            return;
        }
        else
        {
            // now Suppose we have a write operation in the file , Check if there is any copies exist in the system or not.

            cacheArray[processor_num]->Access(address, op);
            line = cacheArray[processor_num]->findLine(address);
            if (copies == true)
            {
                line->setFlags(SHARED_MODIFIED);
                (cacheArray[processor_num])->number_of_Bus_update_signals++; //Increment the number of Bus update Signals as we have Write operation and its Copy exists.
                //lets check the other cores and see if there exists and are in any other flags
                for (int i = 0; i < num_proc; i++)
                {
                    if (i != processor_num && cacheArray[i]->findLine(address) != NULL && cacheArray[i]->findLine(address)->getFlags() != INVALID)
                    {
                        if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
                        {
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->num_of_interventions++;
                            cacheArray[i]->writeBack(address); // So we need to do the write back operation  now for the core which is in Modified state
                        }
                        if ((cacheArray[i]->findLine(address))->getFlags() == EXCLUSIVE)
                        {
                            cacheArray[i]->num_of_interventions++;
                            //cacheArray[i]->writeBack(address); 
                        }

                        if ((cacheArray[i]->findLine(address))->getFlags() == SHARED_MODIFIED)
                        {
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->writeBack(address); // So we need to do the write back operation  now for the core which is in Modified 
                            //cacheArray[i]->num_of_Bus_upd++; // Added by me
                        }

                        (cacheArray[i]->findLine(address))->setFlags(SHARED_CLEAN);
                    }
                }
            }
            else
            {
                line->setFlags(MODIFIED);
            }
            return;
        }
    }



    // For the  SHARED_CLEAN state

    if ((cacheArray[processor_num]->findLine(address))->getFlags() == SHARED_CLEAN)
    {
        if (op == 'r')
        {
            cacheArray[processor_num]->Access(address, op);
            line = cacheArray[processor_num]->findLine(address);
            line->setFlags(SHARED_CLEAN);
        }
        else
        {
            cacheArray[processor_num]->Access(address, op);
            //cacheArray[proc_num]->num_of_flushes++;
            line = cacheArray[processor_num]->findLine(address);
            if (copies == true)
            {
                line->setFlags(SHARED_MODIFIED);
                cacheArray[processor_num]->number_of_Bus_update_signals++; //correct
            }
            else
            {
                line->setFlags(MODIFIED);
                cacheArray[processor_num]->number_of_Bus_update_signals++;//correct
            }

            for (int i = 0; i < num_proc; i++)
            {
                if (i != processor_num && cacheArray[i]->findLine(address) != NULL)
                {
                    // change SHARED_MODIFIED state to SHARED_CLEAN if any
                    if ((cacheArray[i]->findLine(address))->getFlags() == SHARED_MODIFIED)
                    {
                        (cacheArray[i]->findLine(address))->setFlags(SHARED_CLEAN);
                        //cacheArray[proc_num]->num_of_flushes++;
                        //cacheArray[i]->writeBack(address); // write backs
                        //cacheArray[i]->num_of_Bus_upd++; //392 --> giving difference 
                    }


                }
            }
        }
        return;
    }

    // Lets CHeck for the  EXCLUSIVE state
    if ((cacheArray[processor_num]->findLine(address))->getFlags() == EXCLUSIVE)
    {	//For Processor Read (PrRd)  --- For Read operation : EXCLUSIVE ---> EXCLUSIVE or  For the  Write operation : EXCLUSIVE ---> MODIFIED
        if (op == 'r')
        {
            cacheArray[processor_num]->Access(address, op);
            line = cacheArray[processor_num]->findLine(address);
            line->setFlags(EXCLUSIVE);
            return;
        }
        else
        {
            //For the Write operation we dont need to check Copies exists or not
            // So we just need to do EXCLUSIVE ---> MODIFIED
            cacheArray[processor_num]->Access(address, op);
            line = cacheArray[processor_num]->findLine(address);
            line->setFlags(MODIFIED);
            return;
        }
    }

    // for MODIFIED state, no state change and no bus transaction
    if ((cacheArray[processor_num]->findLine(address))->getFlags() == MODIFIED)
    {
        cacheArray[processor_num]->Access(address, op);
        line = cacheArray[processor_num]->findLine(address);
        line->setFlags(MODIFIED);

        return;
    }


    // for SHARED_MODIFIED state 
    // It doesnt matter , whatever state you are , we dont need to conver it into any state.
    if ((cacheArray[processor_num]->findLine(address))->getFlags() == SHARED_MODIFIED)
    {
        //For the Read operation when your in SHARED_MODIFIED ,WHEN WE HAVE read operation we need to keep it in the same state. 
        if (op == 'r')
        {
            cacheArray[processor_num]->Access(address, op);
            (cacheArray[processor_num]->findLine(address))->setFlags(SHARED_MODIFIED);

        }
        else
        {
            //For the Write operation when your in SHARED_MODIFIED ,WHEN WE HAVE read operation we need to keep it in the same state.
            cacheArray[processor_num]->Access(address, op);
            if (copies == true)
            {
                // when write to it and there are other copies, stay the state unchanged
                (cacheArray[processor_num]->findLine(address))->setFlags(SHARED_MODIFIED);
                cacheArray[processor_num]->number_of_Bus_update_signals++;
            }
            else
            {
                // otherwise, change to MODIFIED
                (cacheArray[processor_num]->findLine(address))->setFlags(MODIFIED);
                cacheArray[processor_num]->number_of_Bus_update_signals++;

            }
            //cacheArray[proc_num]->writeBack(address); // write back
        }
    }
    return;
}


void Modified_MSI(Cache** cacheArray, ulong processor_num, ulong address, char op, ulong num_proc)
{

   //Intially check if there is the address in the cache or Not. This is case for Cache Misses .
    if (cacheArray[processor_num]->findLine(address) == NULL)
    {
        if (op == 'r')
        {
            cacheArray[processor_num]->Access(address, op); //Here we increase the Cache Parameters cache misses, reads,read misses and Fills the line with cache
            line = cacheArray[processor_num]->findLine(address);

            if (line != NULL) {
                line->setFlags(CLEAN); // As the Line is intially null we make it as CLEAN FlAG
            }
            //Check other processors , states and find the Cache Parameters
            for (ulong i = 0; i < num_proc; i++)
            {
                if (i != processor_num && (cacheArray[i]->findLine(address)) != NULL) {
                    otherLine = cacheArray[i]->findLine(address);
                    if (otherLine != NULL) {
                        if (otherLine->getFlags() == MODIFIED)
                        {
                            cacheArray[i]->number_of_invalidations++;
                            otherLine->invalidate();
                           // cacheArray[processor_num]->num_of_cache_to_cache_transfer++;
                            cacheArray[i]->writeBack(address);
                            cacheArray[i]->number_of_memory_transactions++;
                            cacheArray[i]->number_of_flushes++;
                        }
                        if (otherLine->getFlags() == CLEAN) {
                            cacheArray[i]->number_of_invalidations++;
                            otherLine->invalidate();
                        }
                    }
                }
            }
            return;
        }
        else {
            cacheArray[processor_num]->Access(address, op);
            line = cacheArray[processor_num]->findLine(address);

            if (line != NULL) {
                line->setFlags(MODIFIED);
            }

            cacheArray[processor_num]->num_of_busRdx++;

            for (ulong i = 0; i < num_proc; i++)
            {
                if (i != processor_num && (cacheArray[i]->findLine(address)) != NULL) {
                    otherLine = cacheArray[i]->findLine(address);
                    if (otherLine != NULL) {
                        if (otherLine->getFlags() == MODIFIED)
                        {
                            cacheArray[i]->number_of_invalidations++;
                            (otherLine)->invalidate();
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->number_of_memory_transactions++;
                            cacheArray[i]->writeBack(address);
                        }
                        if (otherLine->getFlags() == CLEAN) {
                            cacheArray[i]->number_of_invalidations++;
                            otherLine->invalidate();
                        }
                    }
                }
            }
            return;
        }
    }
    else {
        cacheLine* currentLine = cacheArray[processor_num]->findLine(address);

        if ((currentLine)->getFlags() == INVALID) {
            if (op == 'r') {
                cacheArray[processor_num]->Access(address, op);
                (cacheArray[processor_num]->findLine(address))->setFlags(CLEAN);
                for (ulong i = 0; i < num_proc; i++)
                {
                    if (i != processor_num && cacheArray[i]->findLine(address) != NULL) {
                        if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
                        {

                            cacheArray[i]->number_of_invalidations++;
                            (cacheArray[i]->findLine(address))->invalidate();
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->number_of_memory_transactions++;
                            cacheArray[i]->writeBack(address);
                        }
                        if ((cacheArray[i]->findLine(address))->getFlags() == CLEAN) {
                            cacheArray[i]->number_of_invalidations++;
                            (cacheArray[i]->findLine(address))->invalidate();
                        }
                    }
                }
            }
            else {
                cacheArray[processor_num]->findLine(address)->setFlags(MODIFIED);

                cacheArray[processor_num]->num_of_busRdx++;

                for (ulong i = 0; i < num_proc; i++)
                {
                    if (i != processor_num && cacheArray[i]->findLine(address) != NULL) {
                        if ((cacheArray[i]->findLine(address))->getFlags() == MODIFIED)
                        {
                            cacheArray[i]->number_of_invalidations++;
                            (cacheArray[i]->findLine(address))->invalidate();
                            cacheArray[i]->number_of_flushes++;
                            cacheArray[i]->number_of_memory_transactions++;
                            cacheArray[i]->writeBack(address);
                        }
                        if ((cacheArray[i]->findLine(address))->getFlags() == CLEAN) {
                            cacheArray[i]->number_of_invalidations++;
                            (cacheArray[i]->findLine(address))->invalidate();
                        }
                    }
                }
                return;
            }
        }
        if ((currentLine)->getFlags() == CLEAN) {
            if (op == 'r') {
                cacheArray[processor_num]->Access(address, op);

                cacheArray[processor_num]->findLine(address)->setFlags(CLEAN);
            }
            else {
                cacheArray[processor_num]->Access(address, op);
                cacheArray[processor_num]->findLine(address)->setFlags(MODIFIED);
                return;

            }
        }
        if ((currentLine)->getFlags() == MODIFIED)
        {
            cacheArray[processor_num]->Access(address, op);
            cacheArray[processor_num]->findLine(address)->setFlags(MODIFIED);
        }
    }
}

 



int main(int argc, char* argv[]) {
    ifstream fin;
    FILE* pFile;
    ulong processor_number;

    if (argv[1] == NULL) {
        printf("input format: ");
        printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
        exit(0);
    }

    ulong cache_size = atoi(argv[1]);
    ulong cache_assoc = atoi(argv[2]);
    ulong blk_size = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol = atoi(argv[5]); /* 0:MODIFIED_MSI 1:DRAGON*/
    char* fname = (char*)malloc(20);
    fname = argv[6];

    //printf("===== Simulator configuration =====\n");
    // print out simulator configuration here

    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache**)malloc(num_processors * sizeof(Cache*));
    for (ulong i = 0; i < num_processors; i++) {
        if (protocol == 0) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        }
        if (protocol == 1) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        }
        if (protocol == 2) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        }
    }


    pFile = fopen(fname, "r");
    if (pFile == 0)
    {
        printf("Trace file problem\n");
        exit(0);
    }

    //ulong proc;
    uchar op;
    ulong addr;

    // int line = 1;

         //********************************//
         //print out all caches' statistics //
         //********************************//


    if (pFile == 0)
    {
        printf("Trace file problem\n");
        exit(0);
    }
    while (fscanf(pFile, "%lx %c %lx", &processor_number, &op, &addr) != EOF) {
        // Print for debugging
       // printf("Read from trace file: Processor %lx, Operation %c, Address %lx\n", processor_number, op, addr);
        // Call the MMSI function
        if (protocol == 0) {
            Modified_MSI(cacheArray, processor_number, addr, op, num_processors);
        }
        if (protocol == 1) {
            Dragon(cacheArray, processor_number, addr, op, num_processors);
        }


    }
    // }
    //while (fscanf(pFile, "%d %c %lx", &processor_number, &op, &addr) != EOF) {
    //    // Print for debugging
    //    printf("Read from trace file: Processor %d, Operation %c, Address %lx\n", processor_number, op, addr);

    //    // Call the MMSI function
    //    MMSI(cacheArray, processor_number, addr, op, num_processors);
    //}

    /*if (protocol == 0)
    {
        goto MSI;
    }
    fclose(pFile);*/

    if (protocol == 0)
    {
        goto ModifiedMSI;
    }
    fclose(pFile);
    if (protocol == 1)
    {
        goto DRAGON;
    }
//MSI:
//    goto RESULT;

ModifiedMSI:
    goto RESULT;

DRAGON:
    goto RESULT1;



RESULT: // give the final output of Modified MSI
    cout << "===== 506 Personal information =====" << endl;
    cout << "Name: Vignesh Anand" << endl;
    cout << "Unity ID: vanand3" << endl;
    cout << "ECE492 Students? NO" << endl;
    cout << "===== " << 506 << " SMP Simulator configuration =====" << endl;
    cout << "L1_SIZE:               " << cache_size << endl;
    cout << "L1_ASSOC:               " << cache_assoc << endl;
    cout << "L1_BLOCKSIZE:           " << blk_size << endl;
    cout << "NUMBER OF PROCESSORS:   " << num_processors << endl;
    cout << "COHERENCE PROTOCOL:     " << (protocol == 0 ? "MSI" : "Unknown") << endl;
    cout << "TRACE FILE:             " << fname << endl;
    for (ulong i = 0; i < num_processors; i++)
    {
        cout << "============ Simulation results (Cache " << i << ") ============" << endl;
        cout << "01. number of reads:                            " << cacheArray[i]->getReads() << endl;
        cout << "02. number of read misses:                      " << cacheArray[i]->getRM() << endl;
        cout << "03. number of writes:                           " << cacheArray[i]->getWrites() << endl;
        cout << "04. number of write misses:                     " << cacheArray[i]->getWM() << endl;
        cout << "05. total miss rate:                            " << fixed << setprecision(2) << (cacheArray[i]->getWM() + cacheArray[i]->getRM()) * 100.0 / (cacheArray[i]->getReads() + cacheArray[i]->getWrites()) << '%' << endl;
        cout << "06. number of writebacks:                       " << cacheArray[i]->getWB() << endl;
        cout << "07. number of memory transactions:              " << cacheArray[i]->getRM() + cacheArray[i]->getWM() + cacheArray[i]->getWB() << endl;
        cout << "08. number of invalidations:                    " << cacheArray[i]->number_of_invalidations << endl;
        cout << "09. number of flushes:                          " << cacheArray[i]->number_of_flushes << endl;
        cout << "10. number of BusRdX:                           " << cacheArray[i]->num_of_busRdx << endl;
    }
    return 0;


RESULT1: // give the final result of DRAGON
    cout << "===== 506 Personal information =====" << endl;
    cout << "Name: Vignesh Anand" << endl;
    cout << "Unity ID: vanand3" << endl;
    cout << "ECE492 Students? NO" << endl;
    //cout << "===== 506 SMP Simulator configuration =====" << endl;
    //cout << "L1_SIZE: " << cache_size << endl;
    //cout << "L1_ASSOC: " << cache_assoc << endl;
    //cout << "L1_BLOCKSIZE: " << blk_size << endl;
    //cout << "NUMBER OF PROCESSORS: " << num_processors << endl;
    //cout << "COHERENCE PROTOCOL: " << (protocol == 1 ? "Dragon" : "Unknown") << endl; // Assuming 1 is Dragon and 0 is Unknown
    cout << "===== " << 506 << " SMP Simulator configuration =====" << endl;
    cout << "L1_SIZE:                " << cache_size << endl;
    cout << "L1_ASSOC:               " << cache_assoc << endl;
    cout << "L1_BLOCKSIZE:           " << blk_size << endl;
    cout << "NUMBER OF PROCESSORS:   " << num_processors << endl;
    cout << "COHERENCE PROTOCOL:     " << (protocol == 1 ? "Dragon" : "Unknown") << endl;
    cout << "TRACE FILE:             " << fname << endl;

    for (int i = 0; i < num_processors; i++)
    {
        cout << "============ Simulation results (Cache " << i << ") ============" << endl;
        cout << "01. number of reads:                            " << cacheArray[i]->getReads() << endl;
        cout << "02. number of read misses:                      " << cacheArray[i]->getRM() << endl;
        cout << "03. number of writes:                           " << cacheArray[i]->getWrites() << endl;
        cout << "04. number of write misses:                     " << cacheArray[i]->getWM() << endl;
        cout << "05. total miss rate:                            " << fixed << setprecision(2) << (cacheArray[i]->getWM() + cacheArray[i]->getRM()) * 100.0 / (cacheArray[i]->getReads() + cacheArray[i]->getWrites()) << '%' << endl;
        cout << "06. number of writebacks:                       "<< cacheArray[i]->getWB() << endl;
        cout << "07. number of memory transactions:              " << cacheArray[i]->getWM() + cacheArray[i]->getRM() + cacheArray[i]->getWB() << endl;
        cout << "08. number of interventions:                    " << cacheArray[i]->num_of_interventions << endl;
        cout << "09. number of flushes:                          " << cacheArray[i]->number_of_flushes << endl;
        cout << "10. number of Bus Transactions(BusUpd):         " << cacheArray[i]->number_of_Bus_update_signals << endl;
    }
    return 0;
    // Free memory for CacheArray
    for (ulong i = 0; i < num_processors; i++) {
        delete cacheArray[i];
    }

    // Free memory for CacheArray itself
    free(cacheArray);

    //// Free memory for CacheArray itself
    //delete[] cacheArray;

    //// Free memory for fname
    //delete fname;
    
    //delete otherLine;
    //delete line;
   
    return 0;
}





