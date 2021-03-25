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
void performTask(stateType *statePtr);
void addOp(stateType *state);
int getBytes(int *instruction, int shiftAmount);
int convertNum(int num);
void nandOp(int instruction, stateType *stateType);
void lwOp(int instruction, stateType *stateType);
void swOp(int instruction, stateType *stateType);
int beqOp(int instruction, stateType *stateType, int pc);
int jalrOp(int instruction, stateType *stateType);

void clearRegisters(stateType *statePtr);
int getRegisters(int instruction, int * regA, int * regB);
int getOpcode(int instruction);
int getBytes(int *instruction, int shiftAmount);
void ALU(stateType * state);
void DataMemory(stateType * state);
void WriteBack(stateType * state);
void setInitialState(stateType *statePtr);

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

    for(int i = 0; i < state.numMemory; i++ ){
         printf("\t\t\tmemory[%d]=%d\n", state.numMemory, state.instrMem[state.numMemory]);
    
    }

   // performTask(&state);

   run(state);

    return (0);
}

void run(stateType state){

    stateType newState;
    setInitialState(&state);

    while (1) {

        printState(&state);
        
        printf("\nopcode : %d", state.MEMWB.instr);
        /* check for halt */
        if (opcode(state.MEMWB.instr) == HALT) {
            printf("machine halted\n");
            printf("total of %d cycles executed\n", state.cycles);
            exit(0);
        }

        newState = state;
        newState.cycles++;

        /* --------------------- IF stage --------------------- */
        //newState.IFID.pcPlus1++;
        //newState.IFID.instr = newState.instrMem[newState.pc];
        // printf("\nopcode : %d", newState.IFID.instr);
        //read register file and store it regs
        IFID(&newState);
        /* --------------------- ID stage --------------------- */

        newState.IFID.pcPlus1++;
        newState.IDEX.instr =  newState.IFID.instr;
        int offset = getRegisters(newState.IDEX.instr, & newState.IDEX.readRegA,& newState.IDEX.readRegB );

        //read register file and store it IDEX ref
         newState.IDEX.readRegA = newState.reg[newState.IDEX.readRegA];
         newState.IDEX.readRegB = newState.reg[newState.IDEX.readRegB];
         newState.IDEX.offset = offset;
            //  printf("\nopcode : %d", newState.IDEX.instr);
        /* --------------------- EX stage --------------------- */
        newState.EXMEM.branchTarget =  newState.IFID.pcPlus1 + newState.IDEX.offset;
        newState.EXMEM.instr =  newState.IDEX.instr;
        newState.EXMEM.readRegB = newState.IDEX.readRegB;
        ALU(&newState);
             // printf("\nopcode : %d", newState.EXMEM.instr);
      
        /* --------------------- MEM stage --------------------- */

        newState.MEMWB.instr = newState.EXMEM.instr;
        DataMemory(&newState);
            //  printf("\nopcode : %d", newState.MEMWB.instr);

        /* --------------------- WB stage --------------------- */
        newState.WBEND.instr = newState.MEMWB.instr;
        WriteBack(&newState);

             // printf("\nopcode : %d", newState.WBEND.instr);
        //for now
        state = newState; /* this is the last statement before end of the loop.
                    It marks the end of the cycle and updates the
                    current state with the values calculated in this
                    cycle */
        state.pc++;
       //printf("\nnew opcode : %d", state.MEMWB.instr);
    }
}


void ALU(stateType * state){

     int opcode = getOpcode((*state).IDEX.instr);
      
    if (ADD == opcode)
    {
        (*state).EXMEM.aluResult = (*state).IDEX.readRegA + (*state).IDEX.readRegB;

    }
    else if (NAND == opcode)
    {
         (*state).EXMEM.aluResult =~((*state).IDEX.readRegA & (*state).IDEX.readRegB);
    }
    else if (LW == opcode || SW == opcode )
    {
          (*state).EXMEM.aluResult =(*state).IDEX.offset + (*state).IDEX.readRegA;
    }else{

    }
}

void DataMemory(stateType * state){

    int opcode = getOpcode((*state).EXMEM.instr);
  
    if (LW == opcode)
    {
        (*state).MEMWB.writeData = (*state).dataMem[(*state).EXMEM.aluResult];

    }else if(SW == opcode){

        (*state).dataMem[(*state).EXMEM.aluResult] = (*state).EXMEM.readRegB;

    }else{

    }
}

void WriteBack(stateType * state){

     int opcode = getOpcode((*state).WBEND.instr);

    int extractOpcode = (*state).WBEND.instr - (NAND << 22);

    getBytes(&extractOpcode, 19);

    int lwDes = getBytes(&extractOpcode, 16);

    int addDes = extractOpcode;


    if (LW == opcode)
    {
        (*state).reg[lwDes] =  (*state).WBEND.writeData;
    }else if(ADD == opcode){
        //print
       (*state).reg[addDes] =  (*state).EXMEM.aluResult;

    }else if(NAND == opcode){


    }else{

    }
}

void IFID(stateType * newState){
    newState.IFID.pcPlus1++;
    newState.IFID.instr = newState.instrMem[newState.pc];
}
void IDEX(stateType * newState){

}
void EXMEM(stateType * state){

}
void MEMWB(stateType * state){

}
void WBEND(stateType * state){

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

int getRegisters(int instruction, int * regA, int * regB){

    int opcode = getOpcode(instruction);
    printf("\n --------------opcode :  ");
    printf("%d --------------",opcode);
    int extractOpcode = instruction - (opcode << 22);

    (*regA) = getBytes(&extractOpcode, 19);
    
    (*regB) = getBytes(&extractOpcode, 16);
    
    printf("\n-------------- rega %d, regb %d --------------", (*regA), (*regB));

    int offsetField = convertNum(extractOpcode);

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

void setInitialState(stateType *state){
    (*state).IFID.instr = NOOPINSTRUCTION;
    (*state).IDEX.instr  = NOOPINSTRUCTION;
    (*state).EXMEM.instr  = NOOPINSTRUCTION;
    (*state).MEMWB.instr  = NOOPINSTRUCTION;
    (*state).WBEND.instr  = NOOPINSTRUCTION;
}

//default method
void printState(stateType *statePtr){
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

int field0(int instruction){
    return( (instruction>>19) & 0x7);
}

int field1(int instruction){
    return( (instruction>>16) & 0x7);
}

int field2(int instruction){
    return(instruction & 0xFFFF);
}

int opcode(int instruction){
    return(instruction>>22);
}

void printInstruction(int instr){
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