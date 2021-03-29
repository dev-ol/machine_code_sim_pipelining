/* instruction-level simulator for LC3101 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for Project 2 */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDStruct {
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;

typedef struct EXMEMStruct {
    int instr;
    int branchTarget;
    int aluResult;
    int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDStruct {
    int instr;
    int writeData;
} WBENDType;

typedef struct stateStruct {
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; /* number of cycles run so far */
} stateType;


void printState(stateType *);
int convertNum(int num);
void clearRegisters(stateType *statePtr);
int getRegisters(int instruction, int * regA, int * regB);
void ALU(stateType state,stateType * newState);
void DataMemory(stateType  state, stateType * newState);
void WriteBack(stateType state, stateType * newState);
void setInitialState(stateType *statePtr);
void IFID(stateType  state, stateType * newState);
void IDEX(stateType  state, stateType * newState);
void EXMEM(stateType  state, stateType * newState);
void MEMWB(stateType  state, stateType * newState);
void WBEND(stateType  state, stateType * newState);
int stallHazard(stateType  state, stateType * newState);
void forwardHazard(stateType  state, stateType * newState);
int specSquashHazard(stateType  state, stateType * newState);
void checkWBEND(stateType  state, stateType * newState, int nRegA, int nRegB);
void checkEXMEM(stateType  state, stateType * newState,  int nRegA, int nRegB );
void checkMEMWB(stateType  state, stateType * newState,  int nRegA, int nRegB);
void compareSet(int Lreg, int Rreg, int * sR, int iR);

//default
void run(stateType statePtr);
void printInstruction(int instr);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);

int main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 2)
    {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }
    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL)
    {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    clearRegisters(&state);

    /* read in the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++)
    {
        if (sscanf(line, "%d", state.dataMem + state.numMemory) != 1)
        {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }

        printf("memory[%d]=%d\n", state.numMemory, state.dataMem[state.numMemory]);

        if (sscanf(line, "%d", state.instrMem + state.numMemory) != 1)
        {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }

    }
    printf("\t\tinstruction memory:\n");

    for(int i = 0; i < state.numMemory; i++ ) {
        printf("\t\t\tinstrMem[%d]", i );
        printInstruction(state.instrMem[i]);

    }


    run(state);

    return (0);
}

void run(stateType state) {

    stateType newState;
    setInitialState(&state);

    while (1) {

        printState(&state);

        /* check for halt */
        if (opcode(state.MEMWB.instr) == HALT) {
            printf("machine halted\n");
            printf("total of %d cycles executed\n", state.cycles);
            exit(0);
        }

        newState = state;
        newState.cycles++;

        /* --------------------- IF stage --------------------- */

        IFID(state, &newState);

        /* --------------------- ID stage --------------------- */

        IDEX(state,&newState);
        /* --------------------- EX stage --------------------- */

        EXMEM(state,&newState);
        /* --------------------  - MEM stage --------------------- */


        MEMWB(state,&newState);
        /* --------------------- WB stage --------------------- */

        WBEND(state,&newState);

        state = newState; /* this is the last statement before end of the loop.
                    It marks the end of the cycle and updates the
                    current state with the values calculated in this
                    cycle */
    }
}

//alu use to do the calculation
void ALU(stateType state, stateType * newState) {

    int code = opcode(state.IDEX.instr);

    if (ADD == code)
    {
        (*newState).EXMEM.aluResult = (*newState).IDEX.readRegA + (*newState).IDEX.readRegB;
    }
    else if (NAND == code)
    {
        //There is an issue with the nand. only get -1 or 0
        (*newState).EXMEM.aluResult = ~((*newState).IDEX.readRegA & (*newState).IDEX.readRegB);
    }
    else if (LW == code || SW == code )
    {
        (*newState).EXMEM.aluResult = (*newState).IDEX.offset + (*newState).IDEX.readRegA;

    } else if(BEQ == code ) {

        (*newState).EXMEM.aluResult = (*newState).IDEX.readRegA == (*newState).IDEX.readRegB;
    }
}

//get data from memory
void DataMemory(stateType state, stateType * newState) {

    int code = opcode(state.EXMEM.instr);
    EXMEMType input = state.EXMEM;
    MEMWBType output;

    output.instr = input.instr;

    if (LW == code)
    {
        (*newState).MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];

    } else if(SW == code) {

        (*newState).dataMem[state.EXMEM.aluResult] = (*newState).EXMEM.readRegB;

    } else if(ADD == code || NAND == code) {

        (*newState).MEMWB.writeData = state.EXMEM.aluResult;
    }
}

//write back data to the memory
void WriteBack(stateType  state, stateType * newState ) {

    int code = opcode((* newState).WBEND.instr);
    int code2 = opcode((* newState).WBEND.instr);

    int lwDes = field1((* newState).WBEND.instr);
    int addDes = field2((* newState).WBEND.instr);

    if (LW == code)
    {
        (*newState).reg[lwDes] =  (* newState).WBEND.writeData;
    } else if(ADD == code) {
        (*newState).reg[addDes] =  (* newState).WBEND.writeData;

    } else if(NAND == code) {
        (*newState).reg[addDes] =  (* newState).WBEND.writeData;
    } else {

    }

}

//IF stage
void IFID(stateType  state, stateType * newState) {

    (*newState).IFID.pcPlus1 = state.pc + 1;
    (*newState).IFID.instr =  state.instrMem[ state.pc];

    (*newState).pc=state.pc + 1;
}

//ID stage
void IDEX(stateType  state, stateType * newState) {

    (*newState).IDEX.pcPlus1 = state.IFID.pcPlus1;
    (*newState).IDEX.instr =   state.IFID.instr;

    int regAOff, regBOff;

    int offset = getRegisters( state.IDEX.instr,
                               &  regAOff,&   regBOff );

    //read register file and store it IDEX ref
    (*newState).IDEX.readRegA =  state.reg[regAOff];
    (*newState).IDEX.readRegB =  state.reg[regBOff];
    (*newState).IDEX.offset = offset;

    if(stallHazard(state, newState) == 1) {
        (*newState).IFID = state.IFID;
        (*newState).pc = state.pc;
        (*newState).IDEX.instr = NOOPINSTRUCTION;
    }

}

//EX stage
void EXMEM(stateType  state, stateType * newState) {

    (*newState).EXMEM.instr =   state.IDEX.instr;

    (*newState).EXMEM.branchTarget =   state.IDEX.pcPlus1 +  state.IDEX.offset;
    forwardHazard(state,newState);

    printInstruction((*newState).EXMEM.instr);

    (*newState).EXMEM.readRegB =  state.IDEX.readRegB;
    ALU(state,newState);

    if(specSquashHazard(state, newState) == 1) {
        (*newState).pc =   (*newState).EXMEM.branchTarget - 1;
        (*newState).IFID.instr = NOOPINSTRUCTION;
        (*newState).IDEX.instr = NOOPINSTRUCTION;
        (*newState).EXMEM.instr = NOOPINSTRUCTION;
    }
}

//MEM stage
void MEMWB(stateType  state, stateType * newState) {

    (*newState).MEMWB.instr =  state.EXMEM.instr;
    DataMemory(state,newState);
}

//WB stage
void WBEND(stateType  state, stateType * newState) {

    (*newState).WBEND.instr =  state.MEMWB.instr;
    (*newState).WBEND.writeData =  state.MEMWB.writeData;
    WriteBack(state, newState);
}

//check for hazard with LW and stall
int stallHazard(stateType  state, stateType * newState) {

    int nRegA = field0( (*newState).IDEX.instr );
    int nRegB = field1( (*newState).IDEX.instr );
    int destReg = field1( state.IDEX.instr );
    int code = opcode(state.IDEX.instr);

    return(((nRegA == destReg) || (nRegB == destReg)) && (code == LW));
}

//check for data hazard for instructions that
//need data from recent calculation and return it before write back
void forwardHazard(stateType  state, stateType * newState) {

    int nRegA = field0(state.IDEX.instr);
    int nRegB = field1(state.IDEX.instr);

    checkWBEND(state, newState, nRegA, nRegB);
    checkMEMWB(state, newState, nRegA, nRegB);
    checkEXMEM(state, newState, nRegA, nRegB);
}

// check for hazard with beq 
int specSquashHazard(stateType  state, stateType * newState) {

    int code = opcode(state.EXMEM.instr);
    int aluR = state.EXMEM.aluResult;

    return (code == BEQ && aluR == 1);
}

//check for hazard in WB stage
void checkWBEND(stateType  state, stateType * newState, int nRegA, int nRegB) {
    int code =opcode(state.WBEND.instr);

    if(code == LW) {
        int destReg = field1(state.WBEND.instr);

         //compare regA with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegA, destReg,
                   & (*newState).IDEX.readRegA, state.WBEND.writeData );

         //compare regB with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.WBEND.writeData );

    } else if(code == ADD || code == NAND) {

        int destReg = field2(state.WBEND.instr);

        //compare regA with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.WBEND.writeData );
        
        //compare regB with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.WBEND.writeData );
    }

}

//check for hazard in MEM stage
void checkMEMWB(stateType  state, stateType * newState,  int nRegA, int nRegB) {

    int code =opcode(state.MEMWB.instr);

    if(code == LW) {

        int destReg = field1(state.MEMWB.instr);

         //compare regA with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegA, destReg,
                   & (*newState).IDEX.readRegA, state.MEMWB.writeData );

         //compare regB with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.MEMWB.writeData );

    } else if(code == ADD || code == NAND) {

        int destReg = field2(state.MEMWB.instr);

         //compare regA with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.MEMWB.writeData );

        //compare regB with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.MEMWB.writeData );
    }
}

//check for hazard in EX stage
void checkEXMEM(stateType  state, stateType * newState,  int nRegA, int nRegB ) {
    int code = opcode(state.EXMEM.instr);

    if(code == ADD || code == NAND) {

        //getting the  destination register
        int destReg = field2(state.EXMEM.instr);

        //compare regA with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.EXMEM.aluResult );

        //compare regB with the destReg and then store 
        //write data in needed instruction register
        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.EXMEM.aluResult );
    }
}

//utilities

//clear registers
void clearRegisters(stateType *statePtr)
{

    int i;
    for (i = 0; i < 8; i++)
    {
        statePtr->reg[i] = 0;
    }
}

//comparsion and set function
void compareSet(int Lreg, int Rreg, int * sR,  int iR ) {

    if(Lreg == Rreg) {
        (*sR) = iR;
    }
}

//get the registers and return the offset
int getRegisters(int instruction, int * regA, int * regB) {

    (*regA) = field0(instruction);

    (*regB) = field1(instruction);

    int offsetField = convertNum(field2(instruction));

    return offsetField;
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Sun integer */
    if (num & (1 << 15))
    {
        num -= (1 << 16);
    }
    return (num);
}

//set initial state
void setInitialState(stateType *state) {
    (*state).IFID.instr = NOOPINSTRUCTION;
    (*state).IDEX.instr  = NOOPINSTRUCTION;
    (*state).EXMEM.instr  = NOOPINSTRUCTION;
    (*state).MEMWB.instr  = NOOPINSTRUCTION;
    (*state).WBEND.instr  = NOOPINSTRUCTION;
}

//default method
void printState(stateType *statePtr) {
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("\tIFID:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
    printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
    printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
    printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
    printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
    printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
    printf("\tMEMWB:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->WBEND.instr);
    printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int field0(int instruction) {
    return( (instruction>>19) & 0x7);
}

int field1(int instruction) {
    return( (instruction>>16) & 0x7);
}

int field2(int instruction) {
    return(instruction & 0xFFFF);
}

int opcode(int instruction) {
    return(instruction>>22);
}

void printInstruction(int instr) {
    char opcodeString[10];
    if (opcode(instr) == ADD) {
        strcpy(opcodeString, "add");
    } else if (opcode(instr) == NAND) {
        strcpy(opcodeString, "nand");
    } else if (opcode(instr) == LW) {
        strcpy(opcodeString, "lw");
    } else if (opcode(instr) == SW) {
        strcpy(opcodeString, "sw");
    } else if (opcode(instr) == BEQ) {
        strcpy(opcodeString, "beq");
    } else if (opcode(instr) == JALR) {
        strcpy(opcodeString, "jalr");
    } else if (opcode(instr) == HALT) {
        strcpy(opcodeString, "halt");
    } else if (opcode(instr) == NOOP) {
        strcpy(opcodeString, "noop");
    } else {
        strcpy(opcodeString, "data");
    }

    printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
           field2(instr));
}