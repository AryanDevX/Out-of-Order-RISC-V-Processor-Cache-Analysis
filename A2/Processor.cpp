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
    branch.latency = 1; //Assuming branches calculates in 1 cycle.
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
    std::vector<std::string> clean_instructions;
    std::string line;
    while(std::getline(file,line)){
        if(line.empty())continue;
        //Removing comments
        size_t comment_pos = line.find('#');
        if(comment_pos != std::string::npos){
            line = line.substr(0, comment_pos);
        }
        //removing leading and trailing whitespace:
        size_t first = line.find_first_not_of(" \t\r\n");
        if(first==std::string::npos)continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, (last-first+1));

        if(line.substr(0,3) == ".A"){
            std::string mem_data = line.substr(3);
            std::stringstream ss(mem_data);
            int val = 0;
            int mem_index = 0;
            while(ss >> val && mem_index < Memory.size()){
                Memory[mem_index++] = val;
            }
            continue;
        }

        if(line.back()==':'){ //its a label like "MAIN:" so store it.
            std::string name = line.substr(0,line.size()-1); //removed ":"
            label_map[name] = clean_instructions.size();
            continue;
        }
        //if reached here then its a valid instruction:
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

        if(op_str=="ADD") inst.op = OpCode::ADD;
        else if(op_str=="SUB") inst.op = OpCode::SUB;
        else if(op_str=="ADDI") inst.op = OpCode::ADDI;
        else if(op_str=="MUL") inst.op = OpCode::MUL;
        else if(op_str=="DIV") inst.op = OpCode::DIV;
        else if(op_str=="REM") inst.op = OpCode::REM;
        else if(op_str=="LW") inst.op = OpCode::LW;
        else if(op_str=="SW") inst.op = OpCode::SW;
        else if(op_str=="BEQ") inst.op = OpCode::BEQ;
        else if(op_str=="BNE") inst.op = OpCode::BNE;
        else if(op_str=="BLT") inst.op = OpCode::BLT;
        else if(op_str=="BLE") inst.op = OpCode::BLE;
        else if(op_str=="J") inst.op = OpCode::J;
        else if(op_str=="SLT") inst.op = OpCode::SLT;
        else if(op_str=="SLTI") inst.op = OpCode::SLTI;
        else if(op_str=="AND") inst.op = OpCode::AND;
        else if(op_str=="OR") inst.op = OpCode::OR;
        else if(op_str=="XOR") inst.op = OpCode::XOR;
        else if(op_str=="ANDI") inst.op = OpCode::ANDI;
        else if(op_str=="ORI") inst.op = OpCode::ORI;
        else if(op_str=="XORI") inst.op = OpCode::XORI;

        //a helper function for removing x from x1 or any other:
        auto parse_reg = [](std::string r){
            if(r[0]=='x') return std::stoi(r.substr(1));
            return std::stoi(r); //for safety
        };
        if(tokens.size() > 0) inst.dest = parse_reg(tokens[0]);
        if(tokens.size() > 1) inst.src1 = parse_reg(tokens[1]);

        //immediate and branch:
        if(token.size()>2){
            if(label_map.find(tokens[2])!=label_map.end()){
                //branch
                int target_pc = label_map[tokens[2]];
                inst.imm = target_pc - inst.pc;
                inst.src2 = 0;
            }
            else if(tokens[2][0]=='x'){
                inst.src2 = parse_reg(tokens[2]);
            }
            else{
                inst.imm = std::stoi(tokens[2]);
            }
        }
        inst_memory.push_back(inst);
    }
}

void Processor::flush(){
    rob_head = 0;
    rob_tail = 0;
    rob_count = 0;
    for(int i = 0; i<RAT.size(); i++){
        RAT[i] = -1;
    }
    ifId.valid = false;
    for (auto& unit : units) {
        unit.has_result = false;
        unit.cycle_remaining = 0;
        for (auto& rs : unit.rs_entries) {
            rs.active = false; 
        }
    }
    lsq->has_result = false;
    lsq->cycle_remaining = 0;
    for (auto& rs : lsq->rs_entries) {
        rs.active = false;
    }
}

void Processor::broadcastOnCDB(){
    std::vector<BroadcastPackage> broadcasts;
    for(auto &unit:units){
        if(unit.has_result){
            broadcasts.push_back({
                unit.dest_rob_tag,
                (int)unit.result,
                unit.has_exception,
                0,
                unit.name,
                &unit,
                nullptr
            });
        }
    }
    if(lsq->has_result){
        int broadcast_val = (lsq->op == OpCode::SW)?lsq->store_data:lsq->result;
        broadcasts.push_back({
            lsq->dest_rob_tag,
            broadcast_val,
            lsq->has_exception,
            lsq->address,
            UnitType::LOADSTORE,
            nullptr,
            lsq
        });
    }

    for(const auto&b:broadcasts){
        rob[b.tag].ready = true;
        rob[b.tag].value = b.val;
        rob[b.tag].has_exception = b.exception;
        if(b.src == UnitType::LOADSTORE){
            rob[b.tag].store_add = b.store_addr;
        }
        
        for(auto& unit : units) {
            unit.capture(b.tag, b.val);
        }
        lsq->capture(b.tag, b.val);

        //cleanup
        if(b.exe_unit){
            b.exe_unit->rs_pointer->active = false;
            b.exe_unit->has_result = false;
        } 
        else if(b.lsq_unit){
            b.lsq_unit->rs_pointer->active = false;
            b.lsq_unit->has_result = false;
        }
    }
}

void Processor::stageFetch(){
    if(ifId.valid){
        return;
    }
    if(pc >=Memory.size()){
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

void Processor::stageDecode(){
    if(!ifId.valid){
        return;
    }

    Instruction cur_inst = ifId.inst;
    if(rob_count < rob_size){
        rob[rob_tail].op = cur_inst.op;
        rob[rob_tail].pc = cur_inst.pc;
        rob[rob_tail].predicted_pc = ifId.predicted_pc;
        rob[rob_tail].ready = false;
        rob[rob_tail].has_exception = false;
    }

    bool writes_to_reg = true;
    if(cur_inst.op == OpCode::SW || cur_inst.op == OpCode::BEQ || cur_inst.op == OpCode::BNE || cur_inst.op == OpCode::BLT || cur_inst.op == OpCode::BLE){
        writes_to_reg = false;
    }

    if(writes_to_reg && cur_inst.dest != 0){
        rob[rob_tail].dest_arch_reg = cur_inst.dest;
        //updating RAT
        RAT[cur_inst.dest] = rob_tail;
    } 
    else{
        rob[rob_tail].dest_arch_reg = -1; //-1 means instruction not update ARF.
    }

    //dispatch to rs:
    ExecutionUnit* target_unit = nullptr;
    bool is_lsq = false;
    switch(cur_inst.op){
        case OpCode::ADD: case OpCode::SUB: case OpCode::ADDI:
            target_unit = &units[0]; //Adder
            break;
        case OpCode::MUL:
            target_unit = &units[1]; //Multiplier
            break;
        case OpCode::DIV:
            target_unit = &units[2];//Divider
            break;
        case OpCode::BEQ: case OpCode::BNE: case OpCode::BLE: case OpCode::BLT: case OpCode::J: case OpCode::SLT: case OpCode::SLTI:
            target_unit = &units[3]; //branch
            break;
        case OpCode::AND: case OpCode::OR: case OpCode::XOR: case OpCode::ANDI: case OpCode::ORI: case OpCode::XORI:
            target_unit = &units[3]; //logic
            break;
        case OpCode::LW: case OpCode::SW:
            is_lsq = true;//handeled differently
            break;
    }

    rob_tail = (rob_tail+1)%rob_size;
    rob_count++;
    //clear the latch so we don't decode the same instruction twice
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
    ROBEntry& head_entry = rob[rob_head];
    if(!head_entry.ready){
        return;
    }
    if(head_entry.has_exception){
        exception = true;
        pc = head_entry.pc;
        return;
    }
    if(head_entry.op == OpCode::SW){
        Memory[head_entry.store_add] = head_entry.value;
    }
    else if(head_entry.dest_arch_reg > 0){
        ARF[head_entry.dest_arch_reg] = head_entry.value;
        if(RAT[head_entry.dest_arch_reg] == rob_head){ //updating the RAT only if no younger instruction was there. 
            RAT[head_entry.dest_arch_reg] = -1; //prevent older instructions from accidentally erasing the dependencies of younger instructions.
        }
    }

    if(head_entry.op == OpCode::BEQ || head_entry.op == OpCode::BNE || head_entry.op == OpCode::BLT || head_entry.op == OpCode::BLE || head_entry.op == OpCode::J){
        bool branch_taken = (head_entry.value == 1);
        int actual_next_pc = branch_taken?(head_entry.pc + head_entry.imm):(head_entry.pc + 1);
        bool was_correct = (head_entry.predicted_pc == actual_next_pc);
        bp.update(head_entry.pc, actual_next_pc, branch_taken, was_correct);
        if(!was_correct){
            // flush the wrong prediction state
            flush();    
            pc = actual_next_pc; 
        }
    }
    rob_head = (rob_head+1)%rob_size;
    rob_count--;
}

bool Processor::step(){
    if(exception){
        return false;
    }
    //stages in reverse order to simulate single-cycle propagation
    stageCommit();
    stageExecuteAndBroadcast();
    stageDecode();
    stageFetch();

    clock_cycle++;
    //stop if we fetched everything and ROB is completely empty
    return !(pc >= inst_memory.size() && rob_count == 0);
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