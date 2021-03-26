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
void addOp(stateType *state);
int getBytes(int *instruction, int shiftAmount);
int convertNum(int num);



void clearRegisters(stateType *statePtr);
int getRegisters(int instruction, int * regA, int * regB);
int getOpcode(int instruction);
int getBytes(int *instruction, int shiftAmount);
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

    // performTask(&state);

    run(state);

    return (0);
}

void run(stateType state) {

    stateType newState;
    setInitialState(&state);

    while (1) {

        printState(&state);

        // printf("\nopcode : %d", state.MEMWB.instr);
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

        //for now
        state = newState; /* this is the last statement before end of the loop.
                    It marks the end of the cycle and updates the
                    current state with the values calculated in this
                    cycle */

        //printf("\nnew opcode : %d", state.MEMWB.instr);
    }
}


void ALU(stateType state, stateType * newState) {
        printf("\nALU");
    int opcode = getOpcode(state.IDEX.instr);
    printf("\nopcoe : %d", opcode);

    if (ADD == opcode)
    {
        // (*newState).EXMEM.aluResult = (*newState).IDEX.readRegA + (*newState).IDEX.readRegB;
        (*newState).EXMEM.aluResult = (*newState).IDEX.readRegA + (*newState).IDEX.readRegB;
         printf("\nnand  Aalu %d ", (*newState).IDEX.readRegA );
        printf("\nnand  balu %d ", (*newState).IDEX.readRegB );
        printf("\nnand  result %d ",(*newState).EXMEM.aluResult );
    }
    else if (NAND == opcode)
    {
      //  int test = ~(*newState).IDEX.readRegA & (9); //(*newState).IDEX.readRegB;
        (*newState).EXMEM.aluResult = ~((*newState).IDEX.readRegA & (*newState).IDEX.readRegB);
        printf("\nnand  Aalu %d ", (*newState).IDEX.readRegA );
        printf("\nnand  balu %d ", (*newState).IDEX.readRegB );
        printf("\nnand  result %d ",(*newState).EXMEM.aluResult );
    }
    else if (LW == opcode || SW == opcode )
    {
        (*newState).EXMEM.aluResult = (*newState).IDEX.offset + (*newState).IDEX.readRegA;
        printf("\nlw regA %d", (*newState).IDEX.readRegA);
        printf("\nlw offset alu %d", (*newState).IDEX.offset);
        printf("\n--lw alu %d", (*newState).EXMEM.aluResult );

    } else if(BEQ == opcode ) {

        (*newState).EXMEM.aluResult = (*newState).IDEX.readRegA == (*newState).IDEX.readRegB;
        
         printf("\nBEQ Aalu %d ", (*newState).IDEX.readRegA );
        printf("\nBEQ balu %d ", (*newState).IDEX.readRegB );
        printf("\nBEQ  result %d ",(*newState).EXMEM.aluResult );
        

    } else {
    }
}

void DataMemory(stateType state, stateType * newState) {

    int opcode = getOpcode(state.EXMEM.instr);
    EXMEMType input = state.EXMEM;
    MEMWBType output;

    output.instr = input.instr;

    if (LW == opcode)
    {
        printf("\nloading lw");
        (*newState).MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];

    } else if(SW == opcode) {

        (*newState).dataMem[state.EXMEM.aluResult] = (*newState).EXMEM.readRegB;

    } else if(ADD == opcode || NAND == opcode) {

        (*newState).MEMWB.writeData = state.EXMEM.aluResult;
    } else {

    }
}

void WriteBack(stateType  state, stateType * newState ) {
    printf("\n write back called");
   // printState(&state);
   // printf("\nsecond");
   /// printState(newState);

    printf("\n ------------------------");
    int code = opcode((* newState).WBEND.instr);
    int code2 = opcode((* newState).WBEND.instr);
    printf("\ncode %d", code);
    printf("\ncode2 %d", code2);

    int lwDes = field1((* newState).WBEND.instr);
    printf("\nlwdes %d", lwDes);
    int addDes = field2((* newState).WBEND.instr);

    if (LW == code)
    {
         printf("\n LW back called");
        (*newState).reg[lwDes] =  (* newState).WBEND.writeData;
    } else if(ADD == code) {
        //print
        (*newState).reg[addDes] =  (* newState).WBEND.writeData;

    } else if(NAND == code) {
        (*newState).reg[addDes] =  (* newState).WBEND.writeData;
    } else {

    }
 printf("\n write back ended");
    //printState(&state);
   // printf("\nsecond");
   // printState(newState);
    
}

void IFID(stateType  state, stateType * newState) {

    (*newState).IFID.pcPlus1 = state.pc + 1;
    (*newState).IFID.instr =  state.instrMem[ state.pc];

    (*newState).pc=state.pc + 1;


}

void IDEX(stateType  state, stateType * newState) {

    (*newState).IDEX.pcPlus1 = state.IFID.pcPlus1;
    (*newState).IDEX.instr =   state.IFID.instr;

    int regAOff, regBOff;

    int offset = getRegisters( state.IDEX.instr,
                               &  regAOff,&   regBOff );

    printf("offset %d !!",offset);

    //read register file and store it IDEX ref
    (*newState).IDEX.readRegA =  state.reg[regAOff];
    (*newState).IDEX.readRegB =  state.reg[regBOff];
    (*newState).IDEX.offset = offset;

    if(stallHazard(state, newState) == 1) {
        printf("\nyes\n");
        (*newState).IFID = state.IFID;
        (*newState).pc = state.pc;
        (*newState).IDEX.instr = NOOPINSTRUCTION;
    } else {
        printf("\nno\n");
    }
    printf("offset %d !!",(*newState).IDEX.offset);

}

void EXMEM(stateType  state, stateType * newState) {

    printf("---offset %d, code %d !!",state.IDEX.offset,  getOpcode(state.IDEX.instr));

    (*newState).EXMEM.instr =   state.IDEX.instr;

    (*newState).EXMEM.branchTarget =   state.IDEX.pcPlus1 +  state.IDEX.offset;
    forwardHazard(state,newState);

  printInstruction((*newState).EXMEM.instr);
    printf("\n branch %d",   (*newState).EXMEM.branchTarget);
    printf("\n branch 2 %d",   state.EXMEM.branchTarget);
    printf("\n pcplus1 %d",  state.IDEX.pcPlus1);
    printf("\n offset %d",  state.IDEX.offset);


    (*newState).EXMEM.readRegB =  state.IDEX.readRegB;
    printf("\n&&#regB %d",(*newState).IDEX.readRegB );
    ALU(state,newState);
    
     if(specSquashHazard(state, newState) == 1){
        printf("\n specSquash if called i");
        printf("\n branch target %d", (*newState).EXMEM.branchTarget);
        (*newState).pc =   (*newState).EXMEM.branchTarget - 1;
      (*newState).IFID.instr = NOOPINSTRUCTION;
     (*newState).IDEX.instr = NOOPINSTRUCTION;
     (*newState).EXMEM.instr = NOOPINSTRUCTION;
    }
}
void MEMWB(stateType  state, stateType * newState) {

    (*newState).MEMWB.instr =  state.EXMEM.instr;
    DataMemory(state,newState);
}
void WBEND(stateType  state, stateType * newState) {

    (*newState).WBEND.instr =  state.MEMWB.instr;
    (*newState).WBEND.writeData =  state.MEMWB.writeData;
    WriteBack(state, newState);
}

int stallHazard(stateType  state, stateType * newState) {
    printf("\n stallHazard called");
    int nRegA = field0( (*newState).IDEX.instr );
    int nRegB = field1( (*newState).IDEX.instr );
    int destReg = field1( state.IDEX.instr );
    int code = opcode(state.IDEX.instr);

    return(((nRegA == destReg) || (nRegB == destReg)) && (code == LW));
}

void forwardHazard(stateType  state, stateType * newState) {
    printf("\nfwd called");
    int nRegA = field0(state.IDEX.instr);
    int nRegB = field1(state.IDEX.instr);

    checkWBEND(state, newState, nRegA, nRegB);
    checkMEMWB(state, newState, nRegA, nRegB);
    checkEXMEM(state, newState, nRegA, nRegB);

}

int specSquashHazard(stateType  state, stateType * newState){
    printf("\n specSquash called");
    int code = opcode(state.EXMEM.instr);
    int aluR = state.EXMEM.aluResult;

    return (code == BEQ && aluR == 1);
}

void checkWBEND(stateType  state, stateType * newState, int nRegA, int nRegB) {
    int code =opcode(state.WBEND.instr);
    printf("\n checkWBEND called");

    if(code == LW) {
        int destReg = field1(state.WBEND.instr);
        compareSet(nRegA, destReg,
                   & (*newState).IDEX.readRegA, state.WBEND.writeData );

        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.WBEND.writeData );

        printf("\ncheck checkWBEND");

    } else if(code == ADD || code == NAND) {
        printf("\nforwarding for add or nand");
        //getting the  destination regis
        int destReg = field2(state.WBEND.instr);

        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.WBEND.writeData );

        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.WBEND.writeData );
        printf("#regB %d",(*newState).IDEX.readRegB );
    } else {

    }

}
void checkMEMWB(stateType  state, stateType * newState,  int nRegA, int nRegB) {
    printf("\n checkMEMWB called");

    int code =opcode(state.MEMWB.instr);

    if(code == LW) {
        int destReg = field1(state.MEMWB.instr);
       // printf("destReg")
        compareSet(nRegA, destReg,
                   & (*newState).IDEX.readRegA, state.MEMWB.writeData );

        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.MEMWB.writeData );

        printf("\ncheck EcheckMEMWBXMEM");

    } else if(code == ADD || code == NAND) {
        printf("\nforwarding for add or nand");
        //getting the  destination regis
        int destReg = field2(state.MEMWB.instr);

        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.MEMWB.writeData );

        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.MEMWB.writeData );
printf("#regB %d",(*newState).IDEX.readRegB );
    } else {

    }
}
void checkEXMEM(stateType  state, stateType * newState,  int nRegA, int nRegB ) {
    int code = opcode(state.EXMEM.instr);

     printf("\n checkEXMEM called");

    if(code == ADD || code == NAND) {
        printf("\ncheck EXMEM");
    printf("\nforwarding for add or nand");
        //getting the  destination register
        int destReg = field2(state.EXMEM.instr);

        compareSet(nRegA, destReg,
                   &(*newState).IDEX.readRegA, state.EXMEM.aluResult );

        compareSet(nRegB, destReg,
                   &(*newState).IDEX.readRegB, state.EXMEM.aluResult );
printf("#regB %d",(*newState).IDEX.readRegB );
    } else {

    }
}

//utilities
void clearRegisters(stateType *statePtr)
{

    int i;
    for (i = 0; i < 8; i++)
    {
        statePtr->reg[i] = 0;
    }
}

int getOpcode(int instruction)
{
    int opcode = 0;
    opcode = instruction >> 22;

    return opcode;
}

void compareSet(int Lreg, int Rreg, int * sR,  int iR ) {

    if(Lreg == Rreg) {
        printf("\nforwarding  %d = %d", (*sR),iR);
        (*sR) = iR;
    }
}


int getRegisters(int instruction, int * regA, int * regB) {

    int opcode = getOpcode(instruction);
    int extractOpcode = instruction - (opcode << 22);

    (*regA) = field0(instruction);

    (*regB) = field1(instruction);

    int offsetField = convertNum(field2(instruction));

    return offsetField;
}
int getBytes(int *instruction, int shiftAmount)
{
    int reg = (*instruction) >> shiftAmount;

    (*instruction) = (*instruction) - (reg << shiftAmount);

    return reg;
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