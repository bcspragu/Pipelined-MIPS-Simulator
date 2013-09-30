//Make sure to compile with gcc laba.c -lm
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#define ADDRESS_SIZE 32

int C;      //Total cache size (in bytes)
int K = 2;      //Number of lines per set, also known as ways
int L = 16;  //Line length (in bytes)

int miss, hit = 0;
float missRate;


unsigned int **tagArray;
int **lruArray;

int numberOfSets(); //tested
int setIndexLength(); //tested
int whichSet(unsigned int);//tested
int offsetLength(); //tested
int tagLength(); //tested
unsigned int tagBits(unsigned int);//tested
int hitWay(unsigned int);//tested
unsigned int indexBits(unsigned int); //tested
int pow2(int); //tested
int intLog2(int);
void initializeCache(); //tested
void cacheAccess(unsigned int); //tested
void loadTrace(char*); //tested
void updateOnMiss(unsigned int); //tested
void updateOnHit(unsigned int); //tested
void updateLRU(int way, int set); //tested

int main(){
  int i,j;
  char *traceName;
  //Graph 1
  for(i = 1; i < 7; i++){
    sprintf(traceName, "trace%d.txt", i); //Generate the name of the trace file to load
    printf("%s\n",traceName);
    for(j = 7; j < 21; j++){ //We want to go from 2^7 to 2^20
      assert(pow2(j) == pow(2,j)); //Quick check to make sure our pow2 is giving good results
      C = pow2(j);
      initializeCache(tagArray, lruArray);
      loadTrace(traceName);
      assert(miss>=0);
      assert(hit>=0);
      missRate = ((float)miss/(miss+hit));
      printf("%f\n", missRate);
      hit = 0;
      miss = 0;
      free(tagArray);
      free(lruArray);
    }
  }
  C = 32768;
  L = 32;
  //Graph 2
  for(i = 1; i < 7; i++){
    sprintf(traceName, "trace%d.txt", i); //Generate the name of the trace file to load
    printf("%s\n",traceName);
    for(j = 0; j < 7; j++){ //We want to go from 2^0to 2^6
      assert(pow2(j) == pow(2,j)); //Quick check to make sure our pow2 is giving good results
      K = pow2(j);
      initializeCache(tagArray, lruArray);
      loadTrace(traceName);
      assert(miss>=0);
      assert(hit>=0);
      missRate = ((float)miss/(miss+hit));
      printf("%f\n", missRate);
      hit = 0;
      miss = 0;
      free(tagArray);
      free(lruArray);
    }
  }
  return 0;
};

//Author Zach Boynton
//Tested by Brandon Sprague
int numberOfSets(){
  return(C/(K*L));
};

//Author Zach Boynton
//Tested by Brandon Sprague
int setIndexLength(){
  return intLog2(numberOfSets());
};

//Author Brandon Sprague
//Tested by Zach Boynton && Jacquelyn Ingemi
int whichSet(unsigned int address){
  return indexBits(address) % numberOfSets();
}

//Author Zach Boynton
//Tested by Brandon Sprague
int offsetLength(){
  return intLog2(L);
};

int intLog2(int number){
  int log = 0;
  while(number > 1){
    number = (number/2);
    log++;
  }
  return log;
}

//Author Zach Boynton
//Tested by Brandon Sprague
int tagLength(){
  return (ADDRESS_SIZE-(offsetLength()+setIndexLength()));
};

//Author Jacquelyn Ingemi && Brandon Sprague
//tested by Zach Boynton
unsigned int tagBits(unsigned int address){
  int tagSize = tagLength();
  return address >> (ADDRESS_SIZE - tagSize);
}

//Author Brandon Sprague
//Tested by Zach Boynton
unsigned int indexBits(unsigned int address){
  unsigned int shftAddr = address >> offsetLength();
  //Will evaluate to a binary number with setIndexLength() 1s
  unsigned int mask = (pow2(setIndexLength()) - 1);
  //Mask it with the address, which now has the index bits in the right most positions
  return (shftAddr & mask);
}

//Author Jacquelyn Ingemi && Brandon Sprague
//Tested By Zach Boynton
int hitWay(unsigned int address){
  int set = whichSet(address);
  unsigned int *ways = tagArray[set];
  int *lruWays = lruArray[set];
  unsigned int tag = tagBits(address);
  int i = 0;
  while(i < K){
    if(ways[i] == tag && lruWays[i] != -1){
      return i;
    }
    i++;
  }
  return -1;
}

//Returns 2^exponent for integer exponent >= 0
//Author Jacquelyn Ingemi
//Tested by Zach Boynton
int pow2(int exponent){
  assert (exponent >=0);
  int base = 1;
  while(exponent > 0){
    base = base*2;
    exponent--;
  }
  return base;
}

//Author Zach Boynton && Brandon Sprague
//Tested By Zach Boynton
void initializeCache(){
  int sets = numberOfSets();
  int i,j;
  //Initialize lru to lruArray[lines][sets]
  lruArray = (int**)malloc(sets*sizeof(int*));
  tagArray = (unsigned int**)malloc(sets*sizeof(int*));
  for(i = 0; i < sets; i++){
    lruArray[i] = (int*)malloc(K*sizeof(int));
    tagArray[i] = (unsigned int*)malloc(K*sizeof(int));
  }
  for(i = 0; i < sets; i++){
    for(j = 0; j < K; j++){
      //Tag is 0 and lru is -1 so we don't have false matches
      tagArray[i][j] = 0;
      lruArray[i][j] = -1;
    }
  }
}

//Author Brandon Sprague
//Tested By Jacquelyn Ingemi && Zach Boynton
void updateOnMiss(unsigned int address){
  int set = whichSet(address);
  int *lruWays = lruArray[set];
  int i = 0;
  int leastUsed = -1;
  int leastUsedIndex = -1;
  while(i < K){
    if(lruWays[i] == -1){
      tagArray[set][i] = tagBits(address);
      updateLRU(i, set);
      miss++;
      return;
    }
    if(lruWays[i] > leastUsed){
      leastUsed = lruWays[i];
      leastUsedIndex = i;
    }
    i++;
  }
  tagArray[set][leastUsedIndex] = tagBits(address);
  updateLRU(leastUsedIndex,set);
  miss++;
}

//Nothing really needs to happen in here except to update the LRU,
//except we've opted to extract that into a separate method.
//Author Brandon Sprague
//Tested by Jacquelyn Ingemi && Zach Boynton
void updateOnHit(unsigned int address){
  updateLRU(hitWay(address),whichSet(address));
  hit++;
}

//Author Brandon Sprauge
// Tested By Zach Boynton
void cacheAccess(unsigned int address){
  int hitStatus;
  hitStatus = hitWay(address);
  if(hitStatus == -1){ //Miss
    updateOnMiss(address);
  }
  else{ //Hit
    updateOnHit(address);
  }
}

//Author Brandon Sprague
//Tested By Zach Boynton
void loadTrace(char *filename){
  unsigned int address;
  FILE *trFile;
  trFile = fopen(filename,"r");
  while(!feof(trFile)){
    fscanf(trFile,"%u",&address);
    //printf("Address: %u\n", address);
    cacheAccess(address);
  }
  fclose(trFile);
}

//Author Zach Boynton
//Tested by Brandon Sprague && Zach Boynton
void updateLRU(int way, int set){
  int tmp = lruArray[set][way];
  int i;
  for(i = 0; i < K; i++){
    if(lruArray[set][i] < tmp){
      lruArray[set][i]++;
    }
    else if(tmp==-1 && lruArray[set][i] > tmp){
      lruArray[set][i]++;
    }
  }
  lruArray[set][way] = 0;
}
