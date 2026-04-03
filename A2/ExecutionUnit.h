#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class ExecutionUnit {
public:
    // per-unit reservation station
    UnitType name;
    int latency;
    std::vector<RSEntry> rs_entries;
    
    bool has_result = false; // result flag
    bool has_exception = false; // exception flag
    
    void capture(int tag, int val);
    void executeCycle(int rob_head, int rob_size);

    int cycle_remaining;
    OpCode op;
    int v1, v2, imm, dest_rob_tag, pc;
    long long result;
    RSEntry* rs_pointer; //To remember which RS entry to remove after completion.
    
};