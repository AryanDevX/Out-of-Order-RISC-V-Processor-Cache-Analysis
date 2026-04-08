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
    std::vector<PipelineEntry> pipeline;

    bool has_result = false; // result flag
    bool has_exception = false; // exception flag
    long long result;
    int dest_rob_tag;
    int address;
    int store_data = 0;
    RSEntry* rs_pointer;
    
    void capture(int tag, int val);
    void executeCycle(int rob_head, int rob_size, std::vector<int>& Memory);
};