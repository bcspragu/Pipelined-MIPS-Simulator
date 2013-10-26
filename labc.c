#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Function forward declarations
void progScanner(char*);
void parser();

int main(){
  progScanner("prog1.asy");
  return 0;
}

void progScanner(char* filename){
  char instruction[100];
  FILE *instFile = fopen(filename,"r");
  while(fgets(instruction,100,instFile)){
    printf("%s",instruction);
  }
  fclose(instFile);
}
