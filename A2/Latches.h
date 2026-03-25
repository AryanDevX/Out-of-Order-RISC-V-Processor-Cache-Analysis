#ifndef LATCHES_HPP
#define LATCHES_HPP

#include <cstdint>

//Fetch to decode latch:
struct IfIDLatch{
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instruction = 0;
};

//Decode to execute latch:
struct IdExLatch{
    bool valid = false;
    uint32_t pc = 0;

    //Decoded variables:
    int opcode = 0;
    int rs1Val = 0;
    int rs2Val = 0;
    int imm = 0; //Immediate value
    int rd = 0;
    int rs1 = 0;
    int rs2 = 0;

    //Control signals:
    bool regWrite = 0;
    bool memRead = 0;
    bool memWrite = 0;
    bool isBrach = 0;
    int aluOp = 0; //ALU operation
};

struct ExMemLatch{
    bool valid = 0;
    uint32_t instruction = 0;

    int aluResult = 0;
    int storeData = 0;
    int rd = 0;

    bool regWrite = 0;
    bool memWrite = 0;
    bool memRead = 0;
};

struct MemWbLatch{
    bool valid = 0;
    
    int aluResult = 0;
    int memData = 0;
    int rd = 0;

    bool regWrite = 0;
    bool memToReg = 0; //true: use memData; false: use aluResult
};

#endif