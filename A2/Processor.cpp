#include <Processor.h>

bool Processor::step(){
    stageCommit();

    stageExecuteAndBroadcast();

    stageDecode();

    stageFetch();

    //transferring next to current state:
    ifId = nextIfId;
    idEx = nextIdEx;
    exMem = nextExMem;
    memWb = nextMemWb;
    
    clock_cycle++;
    bool isDone = (pc >= inst_memory.size());
    return !isDone; //returning false if cpu has no more to do.
}