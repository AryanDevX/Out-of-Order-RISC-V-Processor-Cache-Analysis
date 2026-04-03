#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class LoadStoreQueue {
public:
    // LSQ reservation station
    int latency;
    std::vector<RSEntry> rs_entries;

    bool has_result = false; // result flag
    bool has_exception = false; // exception flag
    int store_data = 0;

    int cycle_remaining;
    OpCode op;
    int v1, v2, imm, dest_rob_tag, result;
    RSEntry* rs_pointer;
    int address;
    
    void capture(int tag, int val);
    void executeCycle(int rob_head, int rob_size, std::vector<int>& Memory);
};