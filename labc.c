#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum { false, true } bool;

//Function forward declarations
void progScanner(char*);
void parser(char*);
void trimInstruction(char*);
bool isAValidCharacter(char);
bool isAValidReg(char);
void extractOpcode(char*, char*);
int extractRegister(char*,int);

int main(){
  progScanner("prog1.asy");
  return 0;
}

void progScanner(char* filename){
  char instruction[100];
  FILE *instFile = fopen(filename,"r");
  while(fgets(instruction,100,instFile)){
    parser(instruction);
  }
  fclose(instFile);
}

void parser(char* instruction){
  trimInstruction(instruction);
  char opcode[10];
  extractOpcode(opcode,instruction);
  //R Type as fuck
  if(strcmp(opcode,"add") == 0 || strcmp(opcode,"addi") == 0, strcmp(opcode,"sub"), strcmp(opcode,"mult") == 0){
    int rs = extractRegister(instruction,0);
    int rt = extractRegister(instruction,1);
    int rd = extractRegister(instruction,2);
  }
}

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

void extractOpcode(char* opcode, char* instruction){
  int i;
  for(i = 0; instruction[i] != ' '; i++){
    opcode[i] = instruction[i];
  }
  opcode[i] = '\0';
}

int extractRegister(char* instruction, int index){
  int i;
  int regIndex = 0;
  int charIndex = 0;
  bool readOpcode = false;
  char reg[5];
  
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
  //TODO turn value in reg (ex $0, or $t1, etc)
}
