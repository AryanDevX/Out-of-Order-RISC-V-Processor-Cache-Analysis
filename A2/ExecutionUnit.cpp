#include "ExecutionUnit.h"

bool overflow(long long result){
    return (result > 2147483647LL || result < -2147483648LL);
}

void ExecutionUnit::capture(int tag, int val){
    for(int i=0; i<rs_entries.size(); i++){
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
    if(cycle_remaining>0){
        cycle_remaining--;
        if(cycle_remaining==0){
            switch(op){
                case OpCode::ADD:
                    result = (long long) v1 + v2;
                    has_exception = overflow(result);
                    break;
                case OpCode::SUB:
                    result = (long long) v1 - v2;
                    has_exception = overflow(result);
                    break;
                case OpCode::ADDI:
                    result = (long long) v1 + imm;
                    has_exception = overflow(result);
                    break; 
                case OpCode::MUL:
                    result = (long long)v1*v2;
                    has_exception = overflow(result);
                    break;
                case OpCode::DIV:
                    if(v2 == 0){
                        has_exception = 1;
                        break;
                    }
                    result = (long long)v1/v2;
                    break;
                case OpCode::REM:
                    if(v2 == 0){
                        has_exception = 1;
                        break;
                    }
                    result = (long long)v1%v2;
                    break;

                //Branch:
                case OpCode::BEQ:
                    result = (v1==v2)?1:0;
                    break;
                case OpCode::BNE:
                    result = (v1!=v2)?1:0;
                    break;
                case OpCode::BLT:
                    result = (v1<v2)?1:0;
                    break;
                case OpCode::BLE:
                    result = (v1<=v2)?1:0;
                    break;
                case OpCode::J:
                    result = v1;
                    break;
                case OpCode::SLT:
                    result = (v1<v2)?1:0;
                    break;
                case OpCode::SLTI:
                    result = (v1<imm)?1:0;
                    break;

                //Logic:
                case OpCode::AND:
                    result = (v1&v2);
                    break;
                case OpCode::OR:
                    result = (v1|v2);
                    break;
                case OpCode::XOR:
                    result = (v1^v2);
                    break;
                case OpCode::ANDI:
                    result = (v1&imm);
                    break;
                case OpCode::ORI:
                    result = (v1|imm);
                    break;
                case OpCode::XORI:
                    result = (v1^imm);
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
            if(rs_entries[i].active && rs_entries[i].tag1 == -1 && rs_entries[i].tag2==-1){
                int dist = (rs_entries[i].dest_rob_tag - rob_head + rob_size)%rob_size;
                if(dist < smallest_dist){
                    smallest_dist = dist;
                    selected_rs = i;
                }
            }
        }
        if(selected_rs!=-1){
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