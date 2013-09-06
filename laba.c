#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#define ADDRESS_SIZE 32

int C = 2048;     //Total cache size (in bytes)
int K = 128;      //Number of lines per set
int L = 8;        //Line length (in bytes)
int miss, hit=0;

unsigned int **tagArray;
int **lruArray;

int numberOfWays();
int setIndexLength();
int whichSet(unsigned int);
int offsetLength();
int tagLength();
unsigned int tagBits(unsigned int);
int hitWay(unsigned int);
unsigned int indexBits(unsigned int);
int pow2(int);
void initializeCache();
void loadTrace(char*);
void updateOnMiss(unsigned int);
void updateOnHit(unsigned int);
void updateLRU(int way, int set);

int main(){
  //  assert(whichSet(128)==16);
  initializeCache(tagArray, lruArray);
  loadTrace("trace1.txt");
  printf("There are %d hits and %d misses\n", hit, miss);
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

//Author Brandon Sprague
unsigned int tagBits(unsigned int address){
  int tagSize = tagLength();
  return address >> (ADDRESS_SIZE - tagSize);
}

//Author Brandon Sprague
unsigned int indexBits(unsigned int address){
  unsigned int index = address >> offsetLength();
  //Will evaluate to a binary number with setIndexLength() 1s
  unsigned int mask = pow2(setIndexLength()) - 1;
  //Mask it with the address, which now has the index bits in the right most positions
  return address & mask;
}

//Author Brandon Sprague
int hitWay(unsigned int address){
  int set = whichSet(address);
  int numWays = numberOfWays();
  unsigned int *ways = tagArray[set];
  int *lruWays = lruArray[set];
  unsigned int tag = tagBits(address);
  int i = 0;
  while(i < numWays){
    if(ways[i] == tag && lruWays[i] != -1){
      return i;
    }
    i++;
  }
  return -1;
}

//Returns 2^exponent for integer exponent >= 0
//Author Brandon Sprague
int pow2(int exponent){
  int base = 1;
  while(exponent > 0){
    base = base*2;
    exponent--;
  }
  return base;
}

//Author Zach Boynton && Brandon Sprague
void initializeCache(){
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

//Author Brandon Sprague
void updateOnMiss(unsigned int address){
  int set = whichSet(address);
  int numWays = numberOfWays();
  int *lruWays = lruArray[set];
  int i = 0;
  int leastUsed = -1;
  int leastUsedIndex = -1;
  while(i < numWays){
    if(lruWays[i] == -1){
      tagArray[set][i] = tagBits(address);
      updateLRU(i, set);
      return;
    }
    if(lruWays[i] > leastUsed){
      leastUsed = lruWays[i];
      leastUsedIndex = i;
    }
    i++;
  }
  tagArray[set][leastUsedIndex] = tagBits(address);
  miss++;
  updateLRU(leastUsedIndex,set);
}

//Nothing really needs to happen in here except to update the LRU,
//except we've opted to extract that into a separate method.
void updateOnHit(unsigned int address){
  hit++;
  updateLRU(hitWay(address),whichSet(address));
}

//Author Brandon Sprague
void loadTrace(char *filename){
  unsigned int address; 
  int hitStatus;
  FILE *trFile;
  trFile = fopen(filename,"r");
  while(!feof(trFile)){
    fscanf(trFile,"%u",&address);
    //printf("Address is %u\n", address);
    hitStatus = hitWay(address);
    if(hitStatus == -1){ //Miss
      //printf("Ya missed son\n");
      updateOnMiss(address);
    }
    else{ //Hit
      //printf("Hit right in the panus\n");
      updateOnHit(address);
    }
    //    updateLRU(hitStatus, whichSet(address));
  }
  fclose(trFile);
}


void updateLRU(int way, int set){
  int tmp=lruArray[set][way];
  lruArray[set][way]=0;
  int i;
  for(i=0; i<way; i++){
    if(lruArray[set][i]<tmp){
      lruArray[set][i]++;
    }
  }
}
