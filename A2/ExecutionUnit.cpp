#include "ExecutionUnit.h"

bool overflow(long long result){
    return (result > 2147483647LL || result < -2147483648LL);
}

void ExecutionUnit::capture(int tag, int val){
    for(int i=0; i<(int)rs_entries.size(); i++){
        if(rs_entries[i].active){
            if (rs_entries[i].tag1 == tag) {
                rs_entries[i].v1 = val;
                rs_entries[i].tag1 = -1;
            }
            if(rs_entries[i].tag2 ==tag){
                rs_entries[i].v2 =val;
                rs_entries[i].tag2 = -1; 
            }
        }
    }
}

void ExecutionUnit::executeCycle(int rob_head, int rob_size){
    for(auto& p : pipeline){
        p.cycles_left--;
    }
    std::vector<RSEntry*> finished_rs;
    //checking if any finished:
    for(auto& p : pipeline){
        if(p.cycles_left == 0){
            long long res = 0;
            bool exc = false;
            switch(p.op){
                case OpCode::ADD:
                    res = (long long)p.v1 + p.v2;
                    exc = overflow(res);
                    break;
                case OpCode::SUB:
                    res = (long long)p.v1 - p.v2;
                    exc = overflow(res);
                    break;
                case OpCode::ADDI:
                    res = (long long)p.v1 + p.imm;
                    exc = overflow(res);
                    break;
                case OpCode::MUL:
                    res = (long long)p.v1 * p.v2;
                    exc = overflow(res);
                    break;
                case OpCode::DIV:
                    if (p.v2 == 0) { exc = true; }
                    else {
                        res = (long long)p.v1 / p.v2;
                        exc = overflow(res);
                    }
                    break;
                case OpCode::REM:
                    if (p.v2 == 0) { exc = true; }
                    else {
                        res = (long long)p.v1 % p.v2;
                        exc = overflow(res);
                    }
                    break;
                case OpCode::SLT:
                    res = (p.v1 < p.v2) ? 1 : 0;
                    break;
                case OpCode::SLTI:
                    res = (p.v1 < p.imm) ? 1 : 0;
                    break;

                //Branch:
                case OpCode::BEQ:
                    res = (p.v1 == p.v2) ? 1 : 0;
                    break;
                case OpCode::BNE:
                    res = (p.v1 != p.v2) ? 1 : 0;
                    break;
                case OpCode::BLT:
                    res = (p.v1 < p.v2) ? 1 : 0;
                    break;
                case OpCode::BLE:
                    res = (p.v1 <= p.v2) ? 1 : 0;
                    break;

                //Logic:
                case OpCode::AND:
                    res = p.v1 & p.v2;
                    break;
                case OpCode::OR:
                    res = p.v1 | p.v2;
                    break;
                case OpCode::XOR:
                    res = p.v1 ^ p.v2;
                    break;
                case OpCode::ANDI:
                    res = p.v1 & p.imm;
                    break;
                case OpCode::ORI:
                    res = p.v1 | p.imm;
                    break;
                case OpCode::XORI:
                    res = p.v1 ^ p.imm;
                    break;
                default:
                    break;
            }

            has_result = true;
            has_exception = exc;
            result = res;
            dest_rob_tag = p.dest_rob_tag;
            rs_pointer = p.rs_pointer;
            finished_rs.push_back(p.rs_pointer);
        }
    }
    std::vector<PipelineEntry> remaining;
    for(auto& p : pipeline){
        if(p.cycles_left > 0){
            remaining.push_back(p);
        }
    }
    pipeline = remaining;

    int selected_rs = -1;
    int smallest_dist = rob_size + 1;
    for(int i = 0; i < (int)rs_entries.size(); i++){
        if(!rs_entries[i].active || rs_entries[i].tag1 != -1 || rs_entries[i].tag2 != -1) continue;
        bool finished_this_cycle = false;
        for(auto* done_ptr : finished_rs){
            if(done_ptr == &rs_entries[i]){
                finished_this_cycle = true;
                break;
            }
        }
        if(finished_this_cycle) continue;
        // check not already in pipeline
        bool already_in = false;
        for(auto& p : pipeline){
            if(p.rs_pointer == &rs_entries[i]){
                already_in = true;
                break;
            }
        }
        if(already_in) continue;
        int dist = (rs_entries[i].dest_rob_tag - rob_head + rob_size) % rob_size;
        if (dist < smallest_dist) {
            smallest_dist = dist;
            selected_rs = i;
        }
    }
    if(selected_rs != -1){
        PipelineEntry p;
        p.op = rs_entries[selected_rs].op;
        p.v1 = rs_entries[selected_rs].v1;
        p.v2 = rs_entries[selected_rs].v2;
        p.imm = rs_entries[selected_rs].imm;
        p.dest_rob_tag = rs_entries[selected_rs].dest_rob_tag;
        p.cycles_left = latency;
        p.rs_pointer = &rs_entries[selected_rs];
        pipeline.push_back(p);
    }
}
