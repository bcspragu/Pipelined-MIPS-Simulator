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

typedef enum {R, I, J} instruction_type;

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
int extractImmediate(char*);
bool isRType(char* opcode);
bool isIType(char* opcode);
int regValue(char*);

//Stage declarations
void IF();
void ID();
void EX();
void MEM();
void WB();

int data_mem[512];
int registers[32];
instruction instructions[512];

latch if_id_l = { .warmed_up = false };
latch id_ex_l = { .warmed_up = false };
data_latch ex_mem_l = { .warmed_up = false };
data_latch mem_wb_l = { .warmed_up = false };
bool branch_pending = false;

int program_counter = 0;

int main(){
  progScanner("prog2.asy");
  //Once we've read everything in, reset the program_counter
  program_counter = 0;
  //Then we get this thing going
  while(!instructions[program_counter].isHalt){
    WB();
    MEM();
    EX();
    ID();
    IF();
  }
  int i;
  for(i = 0; i < 5; i++){
    WB();
    MEM();
    EX();
    ID();
    IF();
  }
  return 0;
}

void progScanner(char* filename){
  char instruction_string[100];
  FILE *instFile = fopen(filename,"r");
  while(fgets(instruction_string,100,instFile)){
    parser(instruction_string);
  }
  fclose(instFile);
  //TODO add assertion that number of lines in the file equals program counter + 1
}

void parser(char* inst){
  trimInstruction(inst);
  //To hold haltSimultion, needs 15 chars
  char* opcode = extractOpcode(inst);
  //R Type as fuck
  if(isRType(opcode)){
    int rs = extractRegister(inst,1);
    int rt = extractRegister(inst,2);
    int rd = extractRegister(inst,0);
    /*instruction instantiated_instruction = {R,opcode,rs,rt,rd,-1,false};*/
    instructions[program_counter].type = R;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = rs;
    instructions[program_counter].rt = rt;
    instructions[program_counter].dest = rd;
    instructions[program_counter].i = -1;
    instructions[program_counter].isHalt = false;
  }else if(isIType(opcode)){
    int rs = extractRegister(inst,1);
    int rt = extractRegister(inst,0);
    int imm = extractImmediate(inst);
    /*instruction instantiated_instruction = {I,opcode,rs,rt,rt,imm,false};*/
    instructions[program_counter].type = I;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = rs;
    instructions[program_counter].rt = rt;
    instructions[program_counter].dest = rt;
    instructions[program_counter].i = imm;
    instructions[program_counter].isHalt = false;
  }else if(strcmp(opcode,"haltSimulation") == 0){
    //Run it through the pipeline and shut shit down
    //It doesn't matter which type of instruction it is, isHalt is true
    /*instruction instantiated_instruction = {R,opcode,-1,-1,-1,-1,true};*/
    instructions[program_counter].type = R;
    instructions[program_counter].op = opcode;
    instructions[program_counter].rs = -1;
    instructions[program_counter].rt = -1;
    instructions[program_counter].dest = -1;
    instructions[program_counter].i = -1;
    instructions[program_counter].isHalt = true;
  }else{
    assert(!"Unrecognized instruction");
  }
  program_counter++;
}

void IF(){
  //Make sure that the ID stage has invalidated the latch
  if(!if_id_l.valid){
    if_id_l.valid = true;
    if_id_l.inst = instructions[program_counter++];
    if(!if_id_l.warmed_up){
      if_id_l.warmed_up = true;
    }
  }
}

void ID(){
  bool raw_hazard = false;
  if(if_id_l.valid && if_id_l.warmed_up && !id_ex_l.valid){
    instruction inst = if_id_l.inst;
    /*Checking for RAW hazard
      Need to see if the registers we're
      currently looking at are being worked
      on in the EX or MEM stages
      */

    if(ex_mem_l.warmed_up && inst.rs == ex_mem_l.inst.dest && strcmp(ex_mem_l.inst.op,"sw") != 0){
      raw_hazard = true;
    }

    if(mem_wb_l.warmed_up && inst.rs == mem_wb_l.inst.dest){
      raw_hazard = true;
    }

    if(inst.type == R){
      //Need to check that rs and rt aren't targets of future ops
      if(ex_mem_l.warmed_up && inst.rt == ex_mem_l.inst.dest && strcmp(ex_mem_l.inst.op,"sw") != 0){
        raw_hazard = true;
      }
      if(ex_mem_l.warmed_up && inst.rt == mem_wb_l.inst.dest){
        raw_hazard = true;
      }
    }
    if(!raw_hazard){
      if_id_l.valid = false;
    }
    else{
      //Waiting, raw hazhardon
    }
    id_ex_l.inst = if_id_l.inst;
    id_ex_l.valid = true;
    if(!id_ex_l.warmed_up){
      id_ex_l.warmed_up = true;
    }
  }
}

void EX(){
  /*raise(SIGINT);*/
  if(id_ex_l.warmed_up && id_ex_l.valid){
    static int e_cycles = 0;
    if(!ex_mem_l.valid  && ((e_cycles == M && strcmp(id_ex_l.inst.op,"mul") == 0) || (e_cycles == N && strcmp(id_ex_l.inst.op,"mul") != 0)) ){
      if((strcmp(id_ex_l.inst.op,"add") * strcmp(id_ex_l.inst.op,"lw") * strcmp(id_ex_l.inst.op, "sw")) == 0){
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
          program_counter = program_counter + id_ex_l.inst.i;
        }
      }
      else if(strcmp(id_ex_l.inst.op, "haltSimulation") == 0){
        //Butt fuck it
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
  }
}

void MEM(){
  if(ex_mem_l.warmed_up && ex_mem_l.valid){
    static int m_cycles = 0;

    assert(ex_mem_l.inst.op != NULL);
    bool is_lw = strcmp(ex_mem_l.inst.op, "lw") == 0;
    bool is_sw = strcmp(ex_mem_l.inst.op, "sw") == 0;

    if(is_lw || is_sw){
      if(m_cycles == C && !mem_wb_l.valid){
        //EX_memory latch is clear to write too valid bit =1;
        m_cycles = 0;// reset when reached
        ex_mem_l.valid = false;
        mem_wb_l.valid = true; 
        mem_wb_l.inst = ex_mem_l.inst;
        if(!mem_wb_l.warmed_up){
          mem_wb_l.warmed_up = true;
        }
        if(is_lw){
          assert(&data_mem[ex_mem_l.data] != NULL);
          mem_wb_l.data = data_mem[ex_mem_l.data];
        }
        //storing sw
        if(is_sw){
          assert(&data_mem[ex_mem_l.data] != NULL);
          data_mem[ex_mem_l.data] = ex_mem_l.data;
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
  }
}  

void WB(){
  if(mem_wb_l.valid && mem_wb_l.warmed_up && strcmp(mem_wb_l.inst.op,"sw") != 0 && strcmp(mem_wb_l.inst.op,"haltSimulation") != 0 && mem_wb_l.inst.dest != 0){
    registers[mem_wb_l.inst.dest] = mem_wb_l.data;
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
    if(isAValidCharacter(instruction[i])){ //Is alphanumeric or a dollar sign or a comma
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
  return isalnum(c) || c == '$' || c == ',';
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
    assert(!"Register is invalid.");
  }
  return regVal;
}

int extractImmediate(char* instruction){
  int i;
  int regIndex = 0;
  int charIndex = 0;
  bool readOpcode = false;
  char reg[6]; //To be able to hold $zero, which is 5 characters and the null character

  for(i = 0; instruction[i] != '\0'; i++){
    if(readOpcode && instruction[i] == ','){
      regIndex++;
    }
    if(readOpcode && isdigit(instruction[i]) && regIndex == 2){
      reg[charIndex++] = instruction[i];
    }
    if(instruction[i] == ' '){
      readOpcode = true;
    }
  }
  reg[charIndex++] = '\0';
  return atoi(reg);
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


