#include "LoadStoreQueue.h"

void LoadStoreQueue::capture(int tag, int val){
    for(int i = 0; i < rs_entries.size(); i++){
        if(rs_entries[i].active){
            if (rs_entries[i].tag1 == tag) {
                rs_entries[i].v1 = val;
                rs_entries[i].tag1 = -1;
            }
            if (rs_entries[i].tag2 == tag){
                rs_entries[i].v2 = val;
                rs_entries[i].tag2 = -1;
            }
        }
    }
}

void LoadStoreQueue::executeCycle(int rob_head, int rob_size, std::vector<int>& Memory){
    if(cycle_remaining>0){
        cycle_remaining--;
        if(cycle_remaining==0){
            switch (op){
                case OpCode::LW:
                    address = v1 + imm;
                    if(address >= 0 && address <Memory.size()){
                        result = Memory[address];
                    }
                    else{
                        has_exception = 1;
                    }
                    break;
                case OpCode::SW:
                    address = v1+imm;
                    if(address >= 0 && address <Memory.size()){
                        store_data = v2;
                    }
                    else{
                        has_exception = 1;
                    }
                    break;
                default:
                    break;
            }
            has_result = 1;
        }
    }
    else if(!has_result){
        int selected_rs = -1;
        int smallest_dist = rob_size + 1; //Initialized with distance larger than rob size.
        //Finding the oldest instruction based on smallest distance:
        for(int i=0; i<rs_entries.size(); i++){
            if(rs_entries[i].active){
                int dist = (rs_entries[i].dest_rob_tag - rob_head + rob_size)%rob_size;
                if(dist < smallest_dist){
                    smallest_dist = dist;
                    selected_rs = i;
                }
            }
        }
        if(selected_rs!=-1){
            if(rs_entries[selected_rs].tag1==-1 && rs_entries[selected_rs].tag2==-1){
                op = rs_entries[selected_rs].op;
                v1 = rs_entries[selected_rs].v1;
                v2 = rs_entries[selected_rs].v2;
                imm = rs_entries[selected_rs].imm;
                dest_rob_tag = rs_entries[selected_rs].dest_rob_tag;
                cycle_remaining = latency;

                rs_pointer = &rs_entries[selected_rs];
            }        
        }
    }
}