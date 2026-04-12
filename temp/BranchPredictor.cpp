#include "BranchPredictor.h"

int BranchPredictor::predict(int current_pc, int imm, OpCode op){
    if(op==OpCode::J){
        return current_pc+imm;
    }
    if(bht.find(current_pc)==bht.end()){
        bht[current_pc] = BPState::STRONGLY_TAKEN;
    }
    BPState state = bht[current_pc];
    bool predict_taken = (state == BPState::WEAKLY_TAKEN || state == BPState::STRONGLY_TAKEN);
    if(predict_taken){
        return current_pc+imm;
    }
    else{
        return current_pc+1;
    }
}

void BranchPredictor::update(int pc, int actual_target, bool taken, bool was_correct){
    total_branches++;
    if(was_correct){
        correct_predictions++;
    }
    if(bht.find(pc)==bht.end()){
        bht[pc] = BPState::STRONGLY_TAKEN;
    }
    BPState state = bht[pc];
    if(taken){
        if(state == BPState::STRONGLY_NOT_TAKEN) bht[pc] = BPState::WEAKLY_NOT_TAKEN;
        else if(state == BPState::WEAKLY_NOT_TAKEN) bht[pc] = BPState::WEAKLY_TAKEN;
        else if(state == BPState::WEAKLY_TAKEN) bht[pc] = BPState::STRONGLY_TAKEN;
    } 
    else{
        if(state == BPState::STRONGLY_TAKEN) bht[pc] = BPState::WEAKLY_TAKEN;
        else if(state == BPState::WEAKLY_TAKEN) bht[pc] = BPState::WEAKLY_NOT_TAKEN;
        else if(state == BPState::WEAKLY_NOT_TAKEN) bht[pc] = BPState::STRONGLY_NOT_TAKEN;
    }
};