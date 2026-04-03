#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "Basics.h"
#include "BranchPredictor.h"
#include "ExecutionUnit.h"
#include "LoadStoreQueue.h"

struct BroadcastPackage{
    int tag;
    int val;
    bool exception;
    int store_addr;
    UnitType src;
    ExecutionUnit* exe_unit; //ptr to clean the execution unit;
    LoadStoreQueue* lsq_unit; 
};

struct IfIDLatch {
    Instruction inst; //instruction fetched last cycle
    bool valid = false; //is there an actual instruction
    int predicted_pc = 0;
};

class Processor {
private:
    IfIDLatch ifId;
    
public:
    int pc;
    int clock_cycle;

    // pipeline registers

    std::vector<Instruction> inst_memory;

    // architectural state (do not change)
    std::vector<int> ARF; // regFile
    std::vector<int> Memory; // Memory
    bool exception = false; // exception bit

    // register alias table / reorder buffer
    std::vector<ROBEntry> rob;
    std::vector<int> RAT;

    int rob_head;
    int rob_tail;
    int rob_count; //how many instructions are there currently.
    int rob_size;
    std::vector<ExecutionUnit> units;
    LoadStoreQueue* lsq;
    BranchPredictor bp;

    Processor(ProcessorConfig& config);

    void loadProgram(const std::string& filename);

    void flush();

    void broadcastOnCDB();

    void stageFetch();

    void stageDecode();

    void stageExecuteAndBroadcast();

    void stageCommit();

    bool step();

    void dumpArchitecturalState();
};