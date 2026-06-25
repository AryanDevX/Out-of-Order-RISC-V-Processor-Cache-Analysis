#pragma once
#include <string>

enum class OpCode { ADD, SUB, ADDI, MUL, DIV, REM, LW, SW, BEQ, BNE, BLT, BLE, J, SLT, SLTI, AND, OR, XOR, ANDI, ORI, XORI };
enum class UnitType { ADDER, MULTIPLIER, DIVIDER, LOADSTORE, BRANCH, LOGIC };

struct Instruction {
    OpCode op;
    int dest;
    int src1;
    int src2;
    int imm;
    int pc;
};

struct ProcessorConfig {
    int num_regs = 32;
    int rob_size = 64;
    int mem_size = 1024;

    int logic_lat = 1;
    int add_lat = 2;
    int mul_lat = 4;
    int div_lat = 5;
    int mem_lat = 4;

    int logic_rs_size = 4;
    int adder_rs_size = 4;
    int mult_rs_size = 2;
    int div_rs_size = 2;
    int br_rs_size = 2;
    int lsq_rs_size = 32;
};

struct ROBEntry {
    bool active = false; //Is holding an active instruction or just value of previous instruction.
    bool ready = false; //Is we have the value from the function unit.
    int dest_arch_reg = -1; //Architectural register ID to update. For SW it is -1.
    int value = 0;
    bool has_exception = false;
    int imm;

    //Data for commit stage:
    OpCode op; //Help the commit stage what to do.
    int pc = 0; //For exception
    int target_pc = 0; //For branches store the calculated target address.
    int store_add = 0; //For SW
    int predicted_pc = 0;
};

struct RSEntry {
    bool active = 0; //Is holding an active instruction.
    OpCode op;

    int imm;
    //Source 1:
    int v1 = 0; //Value of source 1
    int tag1 = -1; //The ROB tag we are waiting. -1 means it is ready.

    //Source 2:
    int v2 = 0;
    int tag2 = -1;

    //Destination:
    int dest_rob_tag = -1; //ROB tag for broadcast.
    int pc = 0; //usefull for pc relative branches.
};

struct PipelineEntry {
    OpCode op;
    int v1, v2, imm;
    int dest_rob_tag;
    int cycles_left;
    RSEntry* rs_pointer;
};
