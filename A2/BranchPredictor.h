#pragma once
#include "Basics.h"
#include <iostream>
#include <vector>
#include <unordered_map>

enum class BPState {
    STRONGLY_NOT_TAKEN = 0,
    WEAKLY_NOT_TAKEN = 1,
    WEAKLY_TAKEN = 2,
    STRONGLY_TAKEN = 3
};

class BranchPredictor{
private:
    //maps pc to its current 2 bit state
    std::unordered_map<int, BPState> bht;
    
    //for calculated target address
    std::unordered_map<int, int> btb;
public:
    int total_branches = 0;
    int correct_predictions = 0;

    int predict(int current_pc, int imm, OpCode op);
    void update(int pc, int actual_target, bool taken, bool was_correct);
};