#include "LoadStoreQueue.h"

void LoadStoreQueue::capture(int tag, int val){
    for(int i = 0; i < (int)rs_entries.size(); i++){
        if (rs_entries[i].active) {
            if(rs_entries[i].tag1 == tag){
                rs_entries[i].v1 = val;
                rs_entries[i].tag1 = -1;
            }
            if(rs_entries[i].tag2 == tag){
                rs_entries[i].v2 = val;
                rs_entries[i].tag2 = -1;
            }
        }
    }
}

void LoadStoreQueue::executeCycle(int rob_head, int rob_size, std::vector<int>& Memory) {
    for(auto& p:pipeline){
        p.cycles_left--;
    }
    std::vector<RSEntry*> finished_rs;
    //check if any finished
    for(auto& p :pipeline){
        if(p.cycles_left == 0){
            bool exc = false;
            long long res = 0;
            int addr = p.v1 + p.imm;
            switch(p.op){
                case OpCode::LW:
                    if(addr < 0 || addr >= (int)Memory.size()){
                        exc = true;
                    } 
                    else{
                        res = Memory[addr];
                    }
                    break;
                case OpCode::SW:
                    if(addr < 0 || addr >= (int)Memory.size()){
                        exc = true;
                    } 
                    else{
                        store_data = p.v2;
                    }
                    break;
                default:
                    break;
            }
            has_result = true;
            has_exception = exc;
            result = res;
            address = addr;
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

    //finding oldest entry(not oldest ready)
    int selected_rs = -1;
    int smallest_dist = rob_size + 1;
    for(int i = 0; i<(int)rs_entries.size(); i++){
        if(!rs_entries[i].active) continue;
        bool finished_this_cycle = false;
        for(auto* done_ptr : finished_rs){
            if(done_ptr == &rs_entries[i]){
                finished_this_cycle = true;
                break;
            }
        }
        if(finished_this_cycle) continue;
        // skip if already in pipeline
        bool already_in = false;
        for(auto& p : pipeline){
            if(p.rs_pointer == &rs_entries[i]){
                already_in = true;
                break;
            }
        }
        if(already_in) continue;
        int dist = (rs_entries[i].dest_rob_tag - rob_head + rob_size) % rob_size;
        if(dist < smallest_dist){
            smallest_dist = dist;
            selected_rs = i;
        }
    }

    //Issuing only if the oldest entry is ready
    if(selected_rs != -1){
        if(rs_entries[selected_rs].tag1 == -1 && rs_entries[selected_rs].tag2 == -1){
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
        // if not ready, stall
    }
}
