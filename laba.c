//Make sure to compile with gcc laba.c -lm
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#define ADDRESS_SIZE 32
#define NUMBER_OF_WAYS 2

int C;      //Total cache size (in bytes)
int K;      //Number of lines per set
int L = 8;  //Line length (in bytes)

int miss, hit = 0;
float missRate;


unsigned int **tagArray;
int **lruArray;

void assertions();
int numberOfWays(); //tested
int setIndexLength(); //tested
int whichSet(unsigned int);//tested
int offsetLength(); //tested
int tagLength(); //tested
unsigned int tagBits(unsigned int);//tested
int hitWay(unsigned int);//tested
unsigned int indexBits(unsigned int); //tested
int pow2(int); //tested
void initializeCache(); //tested
void cacheAccess(unsigned int); //tested
void loadTrace(char*); //tested
void updateOnMiss(unsigned int); //tested
void updateOnHit(unsigned int); //tested
void updateLRU(int way, int set); //tested

//Need to re-examine number of ways

int main(){
  int i,j;
  char *traceName;
  for(j = 1; j < 6; j++){
    sprintf(traceName, "trace%d.txt", j); //Generate the name of the trace file to load
    printf("%s\n",traceName);
    for(i = 7; i < 21; i++){ //We want to go from 2^7 to 2^20
      assert(pow2(i) == pow(2,i)); //Quick check to make sure our pow2 is giving good results
      C = pow2(i);
      K = C/(L*NUMBER_OF_WAYS); //We want to keep associativity and line length fixed as cache size increases, so we vary the number of lines
      initializeCache(tagArray, lruArray);
      loadTrace(traceName);
      assertions();
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
//Tested by Zach Boynton
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

//Author Brandon Sprague
//Tested By Zach Boynton
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
//Tested by Zach Boynton
int pow2(int exponent){
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
//Tested By Zach Boynton
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
//Tested by Zach Boynton
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
  int nw=numberOfWays();
  int i;
  for(i = 0; i < nw; i++){
   if(lruArray[set][i] < tmp){
	lruArray[set][i]++;
    }
    if(tmp==-1 && lruArray[set][i] > tmp){
      lruArray[set][i]++;
    }
  }
  lruArray[set][way] = 0;
}

void assertions(){
  assert((offsetLength()+setIndexLength()+tagLength()) == ADDRESS_SIZE);
  unsigned int address = 35387358;
  assert((address >> ADDRESS_SIZE-tagLength()) ==  tagBits(address));
  assert(pow2(0) == 1);
  assert(pow2(1) == 2);
  assert(pow2(2) == 4);
  assert(pow2(3) == 8);
  assert(pow2(4) == 16);
  //Testing initialization
  /* initializeCache(tagArray, lruArray);
  int j,k=0;
  for(j=0; j<K; j++){
    for(k=0; k<numberOfWays(); k++){
      printf("%d %d\t", tagArray[j][k],lruArray[j][k]);
    }
    printf("\n");
    }*/
}
