#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define C 10
#define N 10
#define M 15

typedef enum { false, true } bool;

//Bubble type B
typedef enum {R, I, B} instruction_type;

typedef struct {
  instruction_type type;
  char* op;
  int rs;
  int rt;
  int dest;
  int i;
  bool isHalt;
} instruction;

typedef struct{
  bool valid;
  bool warmed_up;
  instruction inst;
} latch;

typedef struct{
  bool valid;
  bool warmed_up;
  int data;
  instruction inst;
} data_latch;

//Function forward declarations
void progScanner(char*);
void parser(char*);
void trimInstruction(char*);
bool isAValidCharacter(char);
bool isAValidReg(char);
char* extractOpcode(char*);
int extractRegister(char*,int);
int extractImmediate(char*,int);
int extractBase(char*);
bool isRType(char* opcode);
bool isIType(char* opcode);
int regValue(char*);
int rawHazard();
void printStatistics();
void printRegisters();

//Stage declarations
void IF();
void ID();
void EX();
void MEM();
void WB();

int data_mem[512];
int registers[32];
instruction instructions[512];
instruction bubble = {B,"bubble",0,0,0,0,false};

latch if_id_l = { .warmed_up = false };
latch id_ex_l = { .warmed_up = false };
data_latch ex_mem_l = { .warmed_up = false };
data_latch mem_wb_l = { .warmed_up = false };

bool branch_pending = false;
bool totally_done = false;
bool single_cycle_mode = true;

int program_counter = 0;
int haltIndex = 0;
int totalCycles = 0;
int ifUtil = 0;
int idUtil = 0;
int exUtil = 0;
int memUtil = 0;
int wbUtil = 0;

char blank[2];

int main(){
  progScanner("prog1.asy");
  //Once we've read everything in, reset the program_counter
  program_counter = 0;
  //Then start iterating over the pipelined stages in reverse
  while(!totally_done){
    WB();
    MEM();
    EX();
    ID();
    IF();
    totalCycles++;
    if(single_cycle_mode){
      printRegisters();
      fgets(blank, sizeof blank, stdin);
    }
  }
  printStatistics();
  return 0;
}

void progScanner(char* filename){
  char instruction_string[100];
  FILE *instFile = fopen(filename,"r");
  while(fgets(instruction_string,100,instFile)){
    parser(instruction_string);
  }
  fclose(instFile);
}

void parser(char* inst){
  trimInstruction(inst);
  //To hold haltSimultion, needs 15 chars
  char* opcode = extractOpcode(inst);
  if(isRType(opcode)){
    int rs = extractRegister(inst,1);
    int rt = extractRegister(inst,2);
    int rd = extractRegister(inst,0);
    instructions[program_counter].type = R;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = rs;
    instructions[program_counter].rt = rt;
    instructions[program_counter].dest = rd;
    instructions[program_counter].i = -1;
    instructions[program_counter].isHalt = false;
  }else if(isIType(opcode)){
    int rs,imm;
    int rt = extractRegister(inst,0);
    if(strcmp(opcode,"lw") != 0 && strcmp(opcode,"sw") != 0){
      rs = extractRegister(inst,1);
      imm = extractImmediate(inst,2);
    }
    else{
      rs = extractBase(inst);
      imm = extractImmediate(inst,1);
    }
    instructions[program_counter].type = I;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = rs;
    instructions[program_counter].rt = rt;
    instructions[program_counter].dest = rt;
    instructions[program_counter].i = imm;
    instructions[program_counter].isHalt = false;
  }else if(strcmp(opcode,"haltSimulation") == 0){
    //Run the haltSimulation instruction through the pipeline and close it down
    instructions[program_counter].type = B;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = -1;
    instructions[program_counter].rt = -1;
    instructions[program_counter].dest = -1;
    instructions[program_counter].i = -1;
    instructions[program_counter].isHalt = true;
    haltIndex = program_counter;
  }else{
    assert(!"Illegal opcode");
  }
  program_counter++;
}

void IF(){
  //Make sure that the ID stage has invalidated the latch
  if(branch_pending){
    if(!if_id_l.valid){
      if_id_l.valid = true;
      if_id_l.inst = bubble;
    }
  }else{
    if(!if_id_l.valid){
      if_id_l.valid = true;
      if_id_l.inst = instructions[program_counter];
      if(program_counter < haltIndex){
        program_counter++;
      }
      ifUtil++;
      branch_pending = strcmp(if_id_l.inst.op,"beq") == 0;
      if(!if_id_l.warmed_up){
        if_id_l.warmed_up = true;
      }
    }
  }
}

void ID(){
  if(if_id_l.valid && if_id_l.warmed_up && !id_ex_l.valid){
    if(rawHazard() == -1){
      if_id_l.valid = false;
      id_ex_l.valid = true;
      id_ex_l.inst = if_id_l.inst;
      if(id_ex_l.inst.type != B){
        idUtil++;
      }
      if(!id_ex_l.warmed_up){
        id_ex_l.warmed_up = true;
      }
    }
    else{
      id_ex_l.valid = true;
      id_ex_l.inst = bubble;
    }
  }
}

void EX(){
  if(id_ex_l.warmed_up && id_ex_l.valid){
    static int e_cycles = 0;
    if(id_ex_l.inst.type == B){
      if(!ex_mem_l.valid){
        //Pass down the bubble
        id_ex_l.valid = false;
        ex_mem_l.valid = true;
        ex_mem_l.inst = id_ex_l.inst;
      }
    }
    else{
      if(!ex_mem_l.valid  && ((e_cycles == M && strcmp(id_ex_l.inst.op,"mul") == 0) || (e_cycles == N && strcmp(id_ex_l.inst.op,"mul") != 0)) ){
        if(strcmp(id_ex_l.inst.op,"add") == 0){
          ex_mem_l.data = registers[id_ex_l.inst.rs] + registers[id_ex_l.inst.rt];
        }
        else if(strcmp(id_ex_l.inst.op,"addi") == 0){
          ex_mem_l.data = registers[id_ex_l.inst.rs] + id_ex_l.inst.i;
        }
        else if(strcmp(id_ex_l.inst.op,"sub") == 0){
          ex_mem_l.data = registers[id_ex_l.inst.rs] - registers[id_ex_l.inst.rt];
        }
        else if(strcmp(id_ex_l.inst.op,"mul") == 0){
          ex_mem_l.data = registers[id_ex_l.inst.rs] * registers[id_ex_l.inst.rt];
        }
        else if(strcmp(id_ex_l.inst.op, "beq") == 0){
          if(registers[id_ex_l.inst.rs] == registers[id_ex_l.inst.rt]){
            program_counter = program_counter + id_ex_l.inst.i + 1;
            if(program_counter > haltIndex){
              assert(!"Branched out of program");
            }
          }
          branch_pending = false;
        }
        else if(strcmp(id_ex_l.inst.op,"lw") * strcmp(id_ex_l.inst.op, "sw") == 0){
          if(id_ex_l.inst.i % 4 == 0){
            ex_mem_l.data = registers[id_ex_l.inst.rt];
            ex_mem_l.inst.dest = registers[id_ex_l.inst.rs] + id_ex_l.inst.i/4;
          }else{
            assert(!"Misaligned memory access");
          }
        }
        else {
          assert(!"Unrecognized instruction");
        }
        e_cycles = 0;
        id_ex_l.valid = false;
        ex_mem_l.valid = true;
        ex_mem_l.inst = id_ex_l.inst;
        if(!ex_mem_l.warmed_up){
          ex_mem_l.warmed_up = true;
        }
      }
      else if(e_cycles < M){
        e_cycles++;
      }
      exUtil++;
    }
  }
}

void MEM(){
  if(ex_mem_l.warmed_up && ex_mem_l.valid){
    if(ex_mem_l.inst.type == B){
      if(!mem_wb_l.valid){
        //Pass down the bubble
        ex_mem_l.valid = false;
        mem_wb_l.valid = true;
        mem_wb_l.inst = ex_mem_l.inst;
      }
    }
    else{
      static int m_cycles = 0;

      assert(ex_mem_l.inst.op != NULL);
      bool is_lw = strcmp(ex_mem_l.inst.op, "lw") == 0;
      bool is_sw = strcmp(ex_mem_l.inst.op, "sw") == 0;

      if(is_lw || is_sw){
        if(m_cycles == C && !mem_wb_l.valid){
          m_cycles = 0;
          ex_mem_l.valid = false;
          mem_wb_l.valid = true; 
          mem_wb_l.inst = ex_mem_l.inst;
          if(!mem_wb_l.warmed_up){
            mem_wb_l.warmed_up = true;
          }
          if(is_lw){
            assert(&data_mem[ex_mem_l.data] != NULL);
            mem_wb_l.data = data_mem[ex_mem_l.inst.dest];
          }
          if(is_sw){
            assert(&data_mem[ex_mem_l.data] != NULL);
            data_mem[ex_mem_l.inst.dest] = ex_mem_l.data;
          }
        }
        else if(m_cycles < C){
          m_cycles++;
        }
      }
      else{
        assert(&ex_mem_l.inst.dest != NULL);
        ex_mem_l.valid = false;
        mem_wb_l.valid = true; 
        mem_wb_l.inst = ex_mem_l.inst;
        mem_wb_l.data = ex_mem_l.data;
        if(!mem_wb_l.warmed_up){
          mem_wb_l.warmed_up = true;
        }
      }
      if(ex_mem_l.inst.type != B){
        memUtil++;
      }
    }
  }
}  

void WB(){
  if(mem_wb_l.valid && mem_wb_l.warmed_up){
    if(strcmp(mem_wb_l.inst.op,"sw") != 0 && strcmp(mem_wb_l.inst.op,"beq") != 0 && strcmp(mem_wb_l.inst.op,"haltSimulation") != 0 && mem_wb_l.inst.dest != 0 && mem_wb_l.inst.type != B){
      registers[mem_wb_l.inst.dest] = mem_wb_l.data;
      wbUtil++;
    }   
    if(mem_wb_l.inst.type == B && mem_wb_l.inst.isHalt){
      totally_done = true;
    }
    mem_wb_l.valid = false; 
  }
}

//All of the helper methods used by parser()
void trimInstruction(char* instruction){
  int stage = 0; //How many times we've gone from not a space to a space
  int newIndex = 0;
  char temp[100];
  int i;
  bool lastCharWasSpace = true;
  for(i = 0; instruction[i] != '\0'; i++){
    if(isAValidCharacter(instruction[i])){ //Is alphanumeric or a dollar sign or a comma or a dash or a paren 
      temp[newIndex++] = instruction[i];
      lastCharWasSpace = false;
    }else{ //Basically is a space
      if(!lastCharWasSpace){
        stage++;
        if(stage == 1){
          temp[newIndex++] = ' ';
        }
      }
      lastCharWasSpace = true;
    }
  }
  temp[newIndex++] = '\0';
  strcpy(instruction,temp);
}

bool isAValidCharacter(char c){
  return isalnum(c) || c == '$' || c == ',' || c == '-' || c == '(' || c == ')';
}

bool isAValidReg(char c){
  return isalnum(c) || c == '$';
}

char* extractOpcode(char* instruction){
  char* opcode = (char *)malloc(sizeof(char) * 15);
  int i;
  for(i = 0; instruction[i] != ' '; i++){
    opcode[i] = instruction[i];
  }
  opcode[i] = '\0';
  return opcode;
}

int extractRegister(char* instruction, int index){
  int i;
  int regIndex = 0;
  int charIndex = 0;
  bool readOpcode = false;
  char reg[6]; //To be able to hold $zero, which is 5 characters and the null character

  for(i = 0; instruction[i] != '\0'; i++){
    if(readOpcode && instruction[i] == ','){
      regIndex++;
    }
    if(readOpcode && isAValidReg(instruction[i]) && index == regIndex){
      reg[charIndex++] = instruction[i];
    }
    if(instruction[i] == ' '){
      readOpcode = true;
    }
  }
  reg[charIndex++] = '\0';
  //
  //Checking if it's a valid register
  if(reg[0] != '$'){
    assert(!"Register didn't start with a dollar sign");
  }
  //Set regValue to  the string without the dollar sign
  int regVal = regValue(reg+1);
  //Register is invalid
  if(regVal == -1){
    assert(!"Unrecognized register name");
  }
  return regVal;
}

int extractBase(char* instruction){
  int i;
  int regIndex = 0;
  int charIndex = 0;
  bool readOpenParen = false;
  bool readCloseParen = false;
  char reg[6]; //To be able to hold $zero, which is 5 characters and the null character

  for(i = 0; instruction[i] != '\0'; i++){
    if(readOpenParen && !readCloseParen){
      reg[charIndex++] = instruction[i];
    }
    if(instruction[i] == '('){
      readOpenParen = true;
    }
    if(instruction[i] == ')'){
      readCloseParen = true;
    }
  }

  if(!readCloseParen || !readOpenParen){
    assert(!"Invalid parentheses");
  }
  if(reg[0] != '$'){
    assert(!"Missing dollar sign on offset");
  }
  reg[charIndex++] = '\0';

  return regValue(reg+1);
}

int extractImmediate(char* instruction,int index){
  int i;
  int regIndex = 0;
  int charIndex = 0;
  bool readOpcode = false;
  char reg[6]; //To be able to hold $zero, which is 5 characters and the null character

  for(i = 0; instruction[i] != '\0' && instruction[i] != '('; i++){
    if(readOpcode && instruction[i] == ','){
      regIndex++;
    }
    if(readOpcode && instruction[i] != ',' && regIndex == index){
      reg[charIndex++] = instruction[i];
    }
    if(instruction[i] == ' '){
      readOpcode = true;
    }
  }
  reg[charIndex++] = '\0';
  //Make sure everything is a digit, or its the first character and its -
  for(i = 0; reg[i] != '\0'; i++){
    if(!isdigit(reg[i])){
      if(!(i == 0 && reg[i] == '-')){
        assert(!"Immediate field contained incorrect value");
      }
    }
  }
  int imm = atoi(reg);
  if(imm > 32767 || imm < -32768){
    assert(!"Immediate field too large");
  }
  return imm;
}

bool isRType(char* opcode){
  return strcmp(opcode,"add") == 0 || strcmp(opcode,"sub") == 0 || strcmp(opcode,"mul") == 0;
}

bool isIType(char* opcode){
  return strcmp(opcode,"addi") == 0 || strcmp(opcode,"lw") == 0 || strcmp(opcode,"sw") == 0 || strcmp(opcode,"beq") == 0;
}

//Returns the location (0-31) of the register from the name
//Note: This takes in a register without the dollar sign, ex. s3, v0, 20, 5, zero
int regValue(char* c){
  //Register indexes via http://msdn.microsoft.com/en-us/library/ms253512(v=vs.80).aspx
  if(isalpha(c[0])){
    int regIndex = c[1] - '0';
    switch(c[0]){
      case 'a':
        if(regIndex >= 0 && regIndex <= 3 ){
          return 4 + regIndex;
        }else if(c[1] == 't'){
          return 1;
        }
        break;
      case 'g':
        if(c[1] == 'p'){
          return 28;
        }
        break;
      case 'k':
        if(regIndex >= 0 && regIndex <= 1 ){
          return 26 + regIndex;
        }
        break;
      case 'r':
        if(c[1] == 'a'){
          return 31;
        }
        break;
      case 's':
        if(regIndex >= 0 && regIndex <= 7 ){
          return 16 + regIndex;
        }else if(regIndex == 8){
          return 30;
        }else if(c[1] == 'p'){
          return 29;
        }
        break;
      case 't':
        if(regIndex >= 0 && regIndex <= 7 ){
          return 8 + regIndex;
        }else if(regIndex >= 8 && regIndex <= 9 ){
          return 16 + regIndex;
        }
        break;
      case 'v':
        if(regIndex >= 0 && regIndex <= 1 ){
          return 2 + regIndex;
        }
        break;
      case 'z':
        if(strcmp(c,"zero") == 0){
          return 0;
        }
        break;
    }
  }else{
    /*If it doesn't have a letter in it's first position, we have to check if it uses the $# format
      First we check if all the characters after the dollar sign are numbers
      If they are, then we convert it to a number and check if its between 0 and 31
      Otherwise we just fail it (return -1)
      */
    int i = 0;
    char currentChar = c[i];
    while(currentChar != '\0'){
      if(!isdigit(currentChar)){
        return -1;
      }
      currentChar = c[++i];
    }
    //If we've made it here, the string is a number
    int regIndex = atoi(c);
    if(regIndex >= 0 && regIndex <= 31){
      return regIndex;
    }else{
      assert(!"Register number out of bounds");
    }
  }

  return -1;
}


int rawHazard(){
  instruction inst = if_id_l.inst;
  if(if_id_l.inst.type != B){
    if(id_ex_l.warmed_up && inst.rs == id_ex_l.inst.dest && strcmp(id_ex_l.inst.op,"sw") != 0){
      return inst.rs;
    }

    if(ex_mem_l.warmed_up && inst.rs == ex_mem_l.inst.dest && strcmp(ex_mem_l.inst.op,"sw") != 0){
      return inst.rs;
    }

    if(mem_wb_l.warmed_up && inst.rs == mem_wb_l.inst.dest && strcmp(mem_wb_l.inst.op,"sw") != 0){
      return inst.rs;
    }

    if(inst.type == R || strcmp(inst.op,"beq") == 0){
      //Need to check that rs and rt aren't targets of future ops
      if(id_ex_l.warmed_up && inst.rt == id_ex_l.inst.dest && strcmp(id_ex_l.inst.op,"sw") != 0){
        return inst.rt;
      }

      if(ex_mem_l.warmed_up && inst.rt == ex_mem_l.inst.dest && strcmp(ex_mem_l.inst.op,"sw") != 0){
        return inst.rt;
      }

      if(mem_wb_l.warmed_up && inst.rt == mem_wb_l.inst.dest && strcmp(mem_wb_l.inst.op,"sw") != 0){
        return inst.rt;
      }
    }
  }
  return -1;
}

void printStatistics(){
  printf("IF Utilization: %.2f%%\n",1.0*ifUtil/totalCycles*100);
  printf("ID Utilization: %.2f%%\n",1.0*idUtil/totalCycles*100);
  printf("EX Utilization: %.2f%%\n",1.0*exUtil/totalCycles*100);
  printf("MEM Utilization: %.2f%%\n",1.0*memUtil/totalCycles*100);
  printf("WB Utilization: %.2f%%\n",1.0*wbUtil/totalCycles*100);
  printf("Execution Time (Cycles): %d\n",totalCycles);
}

void printRegisters(){
  int i;
  for(i = 0; i < 32; i++){
    printf("Register $%d: %d\n",i,registers[i]);
  }
  printf("Press ENTER to continue.\n");
}
