#include "Processor.h"
#include <map>
#include <algorithm>
#include <sstream>

Processor::Processor(ProcessorConfig& config){
    pc = 0;
    clock_cycle = 0;
    ARF.resize(config.num_regs, 0);
    Memory.resize(config.mem_size);

    rob.resize(config.rob_size);
    RAT.resize(config.num_regs, -1);

    rob_head = 0;
    rob_tail = 0;
    rob_count = 0;
    rob_size = config.rob_size;

    // Instantiate Hardware Units
    //index 0:adder:
    ExecutionUnit adder;
    adder.name = UnitType::ADDER;
    adder.latency = config.add_lat;
    adder.rs_entries.resize(config.adder_rs_size);
    units.push_back(adder);

    //index 1:multiplier:
    ExecutionUnit multiplier;
    multiplier.name = UnitType::MULTIPLIER;
    multiplier.latency = config.mul_lat;
    multiplier.rs_entries.resize(config.mult_rs_size);
    units.push_back(multiplier);

    //index 2: divider:
    ExecutionUnit divider;
    divider.name = UnitType::DIVIDER;
    divider.latency = config.div_lat;
    divider.rs_entries.resize(config.div_rs_size);
    units.push_back(divider);

    //index 3: branch:
    ExecutionUnit branch;
    branch.name = UnitType::BRANCH;
    branch.latency = config.add_lat; //Assuming branches calculates in 1 cycle.
    branch.rs_entries.resize(config.br_rs_size);
    units.push_back(branch);
    
    //index 4: logic:
    ExecutionUnit logic;
    logic.name = UnitType::LOGIC;
    logic.latency = config.logic_lat;
    logic.rs_entries.resize(config.logic_rs_size);
    units.push_back(logic);

    //load store: lsq is a pointer so allocating it dynamically.
    lsq = new LoadStoreQueue();
    lsq->latency = config.mem_lat;
    lsq->rs_entries.resize(config.lsq_rs_size);
}

void Processor::loadProgram(const std::string& filename){
    std::ifstream file(filename);
    std::map<std::string,int> label_map;
    std::map<std::string,int> mem_label_map; //for .A, .B etc
    std::vector<std::string> clean_instructions;
    std::string line;
    int mem_index = 0;

    while(std::getline(file,line)){
        //Removing comments
        size_t comment_pos = line.find('#');
        if(comment_pos != std::string::npos){
            line = line.substr(0,comment_pos);
        }
        //removing leading and trailing whitespace:
        size_t first = line.find_first_not_of(" \t\r\n");
        if(first==std::string::npos)continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, (last-first+1));
        if(line.empty())continue;
        
        if(line[0] == '.'){
            size_t colon = line.find(":");
            std::string label = line.substr(1,colon-1);
            mem_label_map[label] = mem_index;
            std::string mem_data = line.substr(colon+1);
            std::stringstream ss(mem_data);
            int val;
            while(ss >> val){
                Memory[mem_index++] = val;
            }
            continue;
        }

        if(line.back()==':'){ //its a label like "MAIN:" so storing it.
            std::string name = line.substr(0,line.size()-1); //removed ":"
            label_map[name] = clean_instructions.size();
            continue;
        }
        //valid instruction 
        clean_instructions.push_back(line);
    }
    
    for(int i=0; i<clean_instructions.size(); i++){
        std::string text = clean_instructions[i];
        std::replace(text.begin(), text.end(), ',', ' '); //replacing commas with space so stringstream works

        std::stringstream ss(text);
        std::string op_str, token;
        std::vector<std::string> tokens;

        //get the opcode like add:
        ss >> op_str;
        //get the rest:
        while(ss >> token){
            tokens.push_back(token);
        }
        Instruction inst;
        inst.pc = i;
        inst.dest = 0;
        inst.src1 = 0;
        inst.src2 = 0;
        inst.imm = 0;

        if(op_str=="add") inst.op = OpCode::ADD;
        else if(op_str=="sub") inst.op = OpCode::SUB;
        else if(op_str=="addi") inst.op = OpCode::ADDI;
        else if(op_str=="mul") inst.op = OpCode::MUL;
        else if(op_str=="div") inst.op = OpCode::DIV;
        else if(op_str=="rem") inst.op = OpCode::REM;
        else if(op_str=="lw") inst.op = OpCode::LW;
        else if(op_str=="sw") inst.op = OpCode::SW;
        else if(op_str=="beq") inst.op = OpCode::BEQ;
        else if(op_str=="bne") inst.op = OpCode::BNE;
        else if(op_str=="blt") inst.op = OpCode::BLT;
        else if(op_str=="ble") inst.op = OpCode::BLE;
        else if(op_str=="j") inst.op = OpCode::J;
        else if(op_str=="slt") inst.op = OpCode::SLT;
        else if(op_str=="slti") inst.op = OpCode::SLTI;
        else if(op_str=="and") inst.op = OpCode::AND;
        else if(op_str=="or") inst.op = OpCode::OR;
        else if(op_str=="xor") inst.op = OpCode::XOR;
        else if(op_str=="andi") inst.op = OpCode::ANDI;
        else if(op_str=="ori") inst.op = OpCode::ORI;
        else if(op_str=="xori") inst.op = OpCode::XORI;

        //a helper function for removing x from x1 or any other:
        auto parse_reg = [](std::string r){
            if(r[0]=='x') return std::stoi(r.substr(1));
            return std::stoi(r); //for safety
        };

        // Parse based on instruction format
        if(inst.op == OpCode::J){
            if(label_map.find(tokens[0]) != label_map.end()){
                inst.imm = label_map[tokens[0]] - i;
            }
        }
        else if(inst.op == OpCode::LW || inst.op == OpCode::SW){
            // lw x4, B(x1)  or  sw x5, C(x1)
            inst.dest = parse_reg(tokens[0]); // for LW this is dest, for SW this is the data register
            std::string mem_part = tokens[1]; // B(x1)
            size_t paren_open = mem_part.find('(');
            size_t paren_close = mem_part.find(')');
            std::string label_or_offset = mem_part.substr(0, paren_open);
            std::string reg_str = mem_part.substr(paren_open + 1, paren_close - paren_open - 1);

            inst.src1 = parse_reg(reg_str); // x1 the offset register

            // label_or_offset could be a memory label like "B" or a number
            if(mem_label_map.find(label_or_offset) != mem_label_map.end()){
                inst.imm = mem_label_map[label_or_offset];
            } 
            else{
                inst.imm = std::stoi(label_or_offset);
            }
            // For SW: first token is the data source register
            if(inst.op == OpCode::SW){
                inst.src2 = parse_reg(tokens[0]);
                inst.dest = -1;
            }
        }
        // beq x1, x2, label
        else if(inst.op == OpCode::BEQ || inst.op == OpCode::BNE || inst.op == OpCode::BLT || inst.op == OpCode::BLE){
            inst.src1 = parse_reg(tokens[0]);
            inst.src2 = parse_reg(tokens[1]);
            if(label_map.find(tokens[2]) != label_map.end()){
                inst.imm = label_map[tokens[2]] - i;
            }
        }
        // addi x1, x2, 5
        else if(inst.op == OpCode::ADDI || inst.op == OpCode::SLTI || inst.op == OpCode::ANDI || inst.op == OpCode::ORI || inst.op == OpCode::XORI){
            inst.dest = parse_reg(tokens[0]);
            inst.src1 = parse_reg(tokens[1]);
            inst.imm = std::stoi(tokens[2]);
        }
        // R-type: add x1, x2, x3
        else{
            inst.dest = parse_reg(tokens[0]);
            inst.src1 = parse_reg(tokens[1]);
            inst.src2 = parse_reg(tokens[2]);
        }
        inst_memory.push_back(inst);
    }
}

void Processor::flush(){
    rob_head = 0;
    rob_tail = 0;
    rob_count = 0;
    for(auto& r : rob){
        r.active = false;
        r.ready = false;
    }
    for(int i = 0; i<RAT.size(); i++){
        RAT[i] = -1;
    }
    ifId.valid = false;
    for(auto& unit : units){
        unit.has_result = false;
        unit.has_exception = false;
        unit.pipeline.clear();
        for(auto& rs : unit.rs_entries){
            rs.active = false; 
        }
    }
    lsq->has_result = false;
    lsq->has_exception = false;
    lsq->pipeline.clear();
    for(auto& rs : lsq->rs_entries) {
        rs.active = false;
    }
}

void Processor::broadcastOnCDB(){
    // Collect broadcasts from all units
    for(auto& unit : units){
        if(unit.has_result){
            int tag = unit.dest_rob_tag;
            int val = (int)unit.result;
            rob[tag].ready = true;
            rob[tag].value = val;
            rob[tag].has_exception = unit.has_exception;
            //Updating all RS entries waiting on this tag
            for(auto& u:units){
                u.capture(tag, val);
            }
            lsq->capture(tag, val);
            // Cleanup
            unit.rs_pointer->active = false;
            unit.has_result = false;
            unit.has_exception = false;
        }
    }
    //collecting broadcast from LSQ
    if(lsq->has_result){
        int tag = lsq->dest_rob_tag;
        int val = (int)lsq->result;
        rob[tag].ready = true;
        rob[tag].has_exception = lsq->has_exception;
        if(rob[tag].op == OpCode::LW){
            rob[tag].value = val;
        }
        else if(rob[tag].op == OpCode::SW){
            rob[tag].store_add = lsq->address;
            rob[tag].value = lsq->store_data;
        }
        //updating all RS entries waiting on this tag
        for(auto& u : units){
            u.capture(tag, val);
        }
        lsq->capture(tag, val);
        lsq->rs_pointer->active = false;
        lsq->has_result = false;
        lsq->has_exception = false;
    }
}

void Processor::stageFetch(){
    if(ifId.valid){
        return;
    }
    if(pc >=inst_memory.size()){
        return;
    }
    Instruction cur_inst = inst_memory[pc];
    ifId.inst = cur_inst;
    ifId.valid = true;
     
    //branch prediction:
    if(cur_inst.op == OpCode::BEQ || cur_inst.op == OpCode::BNE || cur_inst.op == OpCode::BLT || cur_inst.op == OpCode::BLE || cur_inst.op == OpCode::J){
        int predicted_pc = bp.predict(pc, cur_inst.imm, cur_inst.op);
        ifId.predicted_pc = predicted_pc;
        pc = predicted_pc;
    }
    else{
        pc++;
        ifId.predicted_pc = pc;//default is just next pc.
    }
}

void Processor::renameSource(int reg, int& val, int& tag){
    if(RAT[reg] != -1){
        int src_rob = RAT[reg];
        if(rob[src_rob].ready){
            val = rob[src_rob].value;
            tag = -1;
        } else {
            tag = src_rob;
        }
    } else {
        val = ARF[reg];
        tag = -1;
    }
};

void Processor::stageDecode(){
    if(!ifId.valid) return;
    if(rob_count >= rob_size) return; //stall
    Instruction cur_inst = ifId.inst;
    // Handling J
    if(cur_inst.op == OpCode::J){
        rob[rob_tail].active = true;
        rob[rob_tail].ready = true;
        rob[rob_tail].has_exception = false;
        rob[rob_tail].op = cur_inst.op;
        rob[rob_tail].pc = cur_inst.pc;
        rob[rob_tail].predicted_pc = ifId.predicted_pc;
        rob[rob_tail].imm = cur_inst.imm;
        rob[rob_tail].dest_arch_reg = -1;
        rob[rob_tail].value = 1; // taken
        rob_tail = (rob_tail + 1) % rob_size;
        rob_count++;
        ifId.valid = false;
        return;
    }
    // Find target unit and check RS availability
    ExecutionUnit* target_unit = nullptr;
    bool is_lsq = false;
    std::vector<RSEntry>* rs_list = nullptr;

    switch(cur_inst.op){
        case OpCode::ADD: case OpCode::SUB: case OpCode::ADDI: case OpCode::SLT: case OpCode::SLTI:
            target_unit = &units[0]; break;
        case OpCode::MUL:
            target_unit = &units[1]; break;
        case OpCode::DIV: case OpCode::REM:
            target_unit = &units[2]; break;
        case OpCode::BEQ: case OpCode::BNE: case OpCode::BLT: case OpCode::BLE:
            target_unit = &units[3]; break;
        case OpCode::AND: case OpCode::OR: case OpCode::XOR: case OpCode::ANDI: case OpCode::ORI: case OpCode::XORI:
            target_unit = &units[4]; break;
        case OpCode::LW: case OpCode::SW:
            is_lsq = true; break;
        default: break;
    }

    if(is_lsq) rs_list = &lsq->rs_entries;
    else rs_list = &target_unit->rs_entries;

    // Find free RS slot
    int free_rs = -1;
    for(int i = 0; i < rs_list->size(); i++){
        if(!(*rs_list)[i].active){
            free_rs = i;
            break;
        }
    }
    if(free_rs == -1) return; //RS full so stall
    // Allocate ROB entry
    int rob_tag = rob_tail;
    rob[rob_tag].active = true;
    rob[rob_tag].ready = false;
    rob[rob_tag].has_exception = false;
    rob[rob_tag].op = cur_inst.op;
    rob[rob_tag].pc = cur_inst.pc;
    rob[rob_tag].predicted_pc = ifId.predicted_pc;
    rob[rob_tag].imm = cur_inst.imm;

    bool writes_to_reg = true;
    if(cur_inst.op == OpCode::SW || cur_inst.op == OpCode::BEQ || cur_inst.op == OpCode::BNE || cur_inst.op == OpCode::BLT || cur_inst.op == OpCode::BLE){
        writes_to_reg = false;
    }
    if(writes_to_reg && cur_inst.dest != 0){
        rob[rob_tag].dest_arch_reg = cur_inst.dest;
    } 
    else{
        rob[rob_tag].dest_arch_reg = -1;
    }

    //filling RS entry: register renaming
    RSEntry& rs = (*rs_list)[free_rs];
    rs.active = true;
    rs.op = cur_inst.op;
    rs.imm = cur_inst.imm;
    rs.dest_rob_tag = rob_tag;
    rs.pc = cur_inst.pc;

    //source 1:always a register
    renameSource(cur_inst.src1, rs.v1, rs.tag1);

    //source 2: register or not needed
    bool no_src2_reg = (cur_inst.op == OpCode::ADDI || cur_inst.op == OpCode::SLTI || cur_inst.op == OpCode::ANDI || cur_inst.op == OpCode::ORI || cur_inst.op == OpCode::XORI || cur_inst.op == OpCode::LW);
    if(!no_src2_reg){
        renameSource(cur_inst.src2, rs.v2, rs.tag2);
    }
    else{
        rs.v2 = 0;
        rs.tag2 = -1;
    }
    //update RAT
    if(writes_to_reg && cur_inst.dest != 0){
        RAT[cur_inst.dest] = rob_tag;
    }

    rob_tail = (rob_tail + 1) % rob_size;
    rob_count++;
    ifId.valid = false;

}

void Processor::stageExecuteAndBroadcast(){
    for(auto& unit:units){
        unit.executeCycle(rob_head, rob_size);
    }
    lsq->executeCycle(rob_head,rob_size,Memory);
    broadcastOnCDB();
}

void Processor::stageCommit(){
    if(rob_count == 0) return;

    ROBEntry& head = rob[rob_head];
    if(!head.active || !head.ready) return;

    // Exception
    if(head.has_exception){
        exception = true;
        pc = head.pc;
        flush();
        return;
    }

    // Store
    if(head.op == OpCode::SW){
        Memory[head.store_add] = head.value;
    }
    // Register write
    else if(head.dest_arch_reg > 0){
        ARF[head.dest_arch_reg] = head.value;
        if(RAT[head.dest_arch_reg] == rob_head){
            RAT[head.dest_arch_reg] = -1;
        }
    }
    
    // Branch resolution
    if(head.op == OpCode::BEQ || head.op == OpCode::BNE ||
        head.op == OpCode::BLT || head.op == OpCode::BLE){
            bool branch_taken = (head.value == 1);
            int actual_next_pc = branch_taken ? (head.pc + head.imm) : (head.pc + 1);
            bool was_correct = (head.predicted_pc == actual_next_pc);
            bp.update(head.pc, actual_next_pc, branch_taken, was_correct);
            if(!was_correct){
                flush();
                pc = actual_next_pc;
                return;
            }
        }
        
        // J resolution
        if(head.op == OpCode::J){
            int actual_next_pc = head.pc + head.imm;
        if(head.predicted_pc != actual_next_pc){
            flush();
            pc = actual_next_pc;
            return;
        }
    }

    // Retire
    head.active = false;
    rob_head = (rob_head + 1) % rob_size;
    rob_count--;
}

bool Processor::step(){
    if(exception) return false;
    stageCommit();
    stageExecuteAndBroadcast();
    stageDecode();
    stageFetch();
    clock_cycle++;
    //halt when ROB empty and nothing left to fetch and no instruction in latch
    return !(pc >= (int)inst_memory.size() && rob_count == 0 && !ifId.valid);
}

void Processor::dumpArchitecturalState() {
        std::cout << "\n=== ARCHITECTURAL STATE (CYCLE " << clock_cycle << ") ===\n";
        for (int i = 0; i < ARF.size(); i++) {
            std::cout << "x" << i << ": " << std::setw(4) << ARF[i] << " | ";
            if ((i+1) % 8 == 0) std::cout << std::endl;
        }
        if (exception) {
            std::cout << "EXCEPTION raised by instruction " << pc + 1 << std::endl;
        }
        std::cout << "Branch Predictor Stats: " << bp.correct_predictions << "/" << bp.total_branches << " correct.\n";
    }