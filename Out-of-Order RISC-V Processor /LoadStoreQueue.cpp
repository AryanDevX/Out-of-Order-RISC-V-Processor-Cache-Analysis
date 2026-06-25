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

void LoadStoreQueue::executeCycle(int rob_head, int rob_size, std::vector<int>& Memory, const std::vector<ROBEntry>& rob) {
    //finding oldest entry(not oldest ready)
    int selected_rs = -1;
    int smallest_dist = rob_size + 1;
    for(int i = 0; i<(int)rs_entries.size(); i++){
        if(!rs_entries[i].active) continue;
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
        bool issue_ok = (rs_entries[selected_rs].tag1 == -1 && rs_entries[selected_rs].tag2 == -1);
        if(issue_ok && rs_entries[selected_rs].op == OpCode::LW){
            int load_addr = rs_entries[selected_rs].v1 + rs_entries[selected_rs].imm;
            int load_tag = rs_entries[selected_rs].dest_rob_tag;
            bool must_wait = false;
            for(int idx = rob_head; idx != load_tag; idx = (idx + 1) % rob_size){
                if(!rob[idx].active || rob[idx].op != OpCode::SW) continue;

                // If older store already finished LSQ execution, this load can
                // forward from ROB when it completes.
                if(rob[idx].ready){
                    continue;
                }

                // Try to resolve older store address from LSQ pipeline or RS.
                bool addr_known = false;
                int store_addr = 0;
                bool store_in_pipeline = false;

                for(auto& p : pipeline){
                    if(p.dest_rob_tag == idx){
                        store_in_pipeline = true;
                        addr_known = true;
                        store_addr = p.v1 + p.imm;
                        break;
                    }
                }
                if(!addr_known){
                    for(auto& rs : rs_entries){
                        if(rs.active && rs.dest_rob_tag == idx){
                            if(rs.tag1 == -1){
                                addr_known = true;
                                store_addr = rs.v1 + rs.imm;
                            }
                            break;
                        }
                    }
                }

                // Memory disambiguation:
                // - Unknown older-store address => wait.
                // - Known same-address older store not yet in pipeline => wait.
                //   (If already in pipeline, allow overlap and forward later.)
                if(!addr_known){
                    must_wait = true;
                    break;
                }
                if(store_addr == load_addr && !store_in_pipeline){
                    must_wait = true;
                    break;
                }
            }

            if(must_wait){
                issue_ok = false;
            }
        }
        if(issue_ok){
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

    for(auto& p:pipeline){
        p.cycles_left--;
    }
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
                        // Store-to-load forwarding:
                        // use the youngest older store to same address from:
                        // (1) ready ROB stores, (2) in-flight LSQ store pipeline.
                        bool forwarded = false;
                        int load_tag = p.dest_rob_tag;
                        for(int idx = rob_head; idx != load_tag; idx = (idx + 1) % rob_size){
                            if(!rob[idx].active || rob[idx].op != OpCode::SW){
                                continue;
                            }

                            if(rob[idx].ready && rob[idx].store_add == addr){
                                res = rob[idx].value;
                                forwarded = true;
                                continue;
                            }

                            // Not ready yet: check if this older store is in LSQ pipeline
                            // and can be forwarded directly.
                            for(auto& older : pipeline){
                                if(older.op != OpCode::SW || older.dest_rob_tag != idx){
                                    continue;
                                }
                                int older_addr = older.v1 + older.imm;
                                if(older_addr == addr){
                                    res = older.v2;
                                    forwarded = true;
                                }
                                break;
                            }
                        }
                        if(!forwarded){
                            res = Memory[addr];
                        }
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
        }
    }

    std::vector<PipelineEntry> remaining;
    for(auto& p : pipeline){
        if(p.cycles_left > 0){
            remaining.push_back(p);
        }
    }
    pipeline = remaining;

}
