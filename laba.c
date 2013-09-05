#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define ADDRESS_SIZE 32

int C = 2048;     //Total cache size (in bytes)
int K = 128;      //Number of lines per set
int L = 8;        //Line length (in bytes)

unsigned int **tagArray;
int **lruArray;

int numberOfWays();
int setIndexLength();
int whichSet(unsigned int);
int offsetLength();
int tagLength();
unsigned int tagBits(unsigned int);
int hitWay(unsigned int, unsigned int**);
unsigned int indexBits(unsigned int);
int pow2(int);
void initializeCache(unsigned int**, int**);
void loadTrace(char*, unsigned int**, int**);

int main(){
  initializeCache(tagArray, lruArray);
  loadTrace("trace1.txt",tagArray,lruArray);
  return 0;
};

//Author Zach Boynton
//Tested by Brandon Sprague
int numberOfWays(){
  return(C/(K*L));
};

//Author Zach Boynton
//Tested by Brandon Sprague
int setIndexLength(){
  int t = K;
  int l = 0;
  while(t > 1){
    t = (t/2);
    l++;
  };
  return l;
};

//Author Brandon Sprague
int whichSet(unsigned int address){
  return indexBits(address) % K;
}

//Author Zach Boynton
//Tested by Brandon Sprague
int offsetLength(){
  int t = L;
  int l = 0;
  while(t > 1){
    t = (t/2);
    l++;
  };
  return l;
};

//Author Zach Boynton
//Tested by Brandon Sprague
int tagLength(){
  return (ADDRESS_SIZE-(offsetLength()+setIndexLength()));
};

unsigned int tagBits(unsigned int address){
  int tagSize = tagLength();
  return address >> (ADDRESS_SIZE - tagSize);
}

unsigned int indexBits(unsigned int address){
  unsigned int index = address >> offsetLength();
  //Will evaluate to a binary number with setIndexLength() 1s
  unsigned int mask = pow2(setIndexLength()) - 1;
  //Mask it with the address, which now has the index bits in the right most positions
  return address & mask;
}

int hitWay(unsigned int address, unsigned int **tagArray){
  int set = whichSet(address);
  int numWays = numberOfWays(L,K,C);
  int offset = offsetLength(L);
  int length = setIndexLength(K);
  unsigned int *ways = tagArray[set];
  unsigned int tag = tagBits(address);
  int i = 0;
  while(i < numWays){
    if(ways[i] == tag){
      return i;
    }
    i++;
  }
  return -1;
}

//Returns 2^exponent for integer exponent >= 0
int pow2(int exponent){
  int base = 1;
  while(exponent > 0){
    base = base*2;
    exponent--;
  }
  return base;
}

void initializeCache(unsigned int **tagArray, int **lruArray){
  int sets = K;
  int ways = numberOfWays();
  int i,j;
  //Initialize lru to lruArray[lines][sets]
  lruArray = (int**)malloc(sets*sizeof(int*));
  tagArray = (unsigned int**)malloc(sets*sizeof(int*));
  for(i = 0; i < sets; i++){
    lruArray[i] = (int*)malloc(ways*sizeof(int));
    tagArray[i] = (unsigned int*)malloc(ways*sizeof(int));
  }
  for(i = 0; i < sets; i++){
    for(j = 0; j < ways; j++){
      //Tag is 0 and lru is -1 so we don't have false matches
      tagArray[i][j] = 0;
      lruArray[i][j] = -1;
    }
  }
}

void loadTrace(char *filename, unsigned int **tagArray, int **lruArray){
  int trace;
  FILE *trFile;
  trFile = fopen(filename,"r");
  int i = 0;
  while(!feof(trFile)){
    fscanf(trFile,"%d",&trace);
    if(i < 10){
      printf("%x, %d, %o\n", trace,trace,trace);
    }
    i++;
  }
  fclose(trFile);
}
