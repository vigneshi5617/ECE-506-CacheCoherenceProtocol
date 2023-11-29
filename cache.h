/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
    INVALID = 0,
    VALID = 1,
    DIRTY = 2,
    SHARED = 3,
    CLEAN = 4,
    MODIFIED = 5,
    EXCLUSIVE = 6,
    SHARED_CLEAN = 7,
    SHARED_MODIFIED = 8,
    EMPTY = 9
};

class cacheLine
{
protected:
    ulong tag;
    ulong Flags;   // 0:INVALID, 1:SHARED, 2:MODIFIED 
    ulong seq;

public:
    cacheLine() { tag = 0; Flags = 0; }
    ulong getTag() { return tag; }
    ulong getFlags() { return Flags; }
    ulong getSeq() { return seq; }
    void setSeq(ulong Seq) { seq = Seq; }
    void setFlags(ulong flags) { Flags = flags; }
    void setTag(ulong a) { tag = a; }
    void invalidate() { tag = 0; Flags = INVALID; }//useful function
    bool isValid() { return ((Flags) != INVALID); }
};

class Cache
{
protected:
    ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
    ulong reads, readMisses, writes, writeMisses, writeBacks;

    //******///
    //add coherence counters here///
    //******///

    cacheLine** cache;
    ulong calcTag(ulong addr) { return (addr >> (log2Blk)); }
    ulong calcIndex(ulong addr) { return ((addr >> log2Blk) & tagMask); }
    ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk)); }

public:
    ulong currentCycle;
    //TODO: temporarily all variables set to public

   
    ulong number_of_flushes;
    ulong num_of_interventions;
    ulong number_of_invalidations;
   
    ulong num_of_busRdx;
    ulong number_of_memory_transactions;
    ulong number_of_Bus_update_signals;


    Cache(int, int, int);
    ~Cache() { delete cache; }

    cacheLine* findLineToReplace(ulong addr);
    cacheLine* fillLine(ulong addr);
    cacheLine* findLine(ulong addr);
    cacheLine* getLRU(ulong);

    ulong getRM() { return readMisses; } ulong getWM() { return writeMisses; }
    ulong getReads() { return reads; }ulong getWrites() { return writes; }
    ulong getWB() { return writeBacks; }

    void writeBack(ulong) { writeBacks++; }
    void Access(ulong, uchar);
    void printStats();
    void updateLRU(cacheLine*);

    //******///
    //add other functions to handle bus transactions///
    //******///

};

#endif


