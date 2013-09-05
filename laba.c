#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define ADDRESS_SIZE 32

int C = 2048;     //Total cache size (in bytes)
int K = 128;      //Number of lines per set
int L = 8;        //Line length (in bytes)

unsigned int **tagArray;
int **lruArray;

int main(){
  tagArray = (unsigned int**)malloc(K*numberOfSets(L,K,C)*sizeof(unsigned int));
  //lruArray = (int**)malloc(K*numberOfSets(L,K,C)*sizeof(int));
  int Sets = numberOfSets(L,K,C);
  printf("There are %d sets\n", Sets);
  int Length = setIndexLength(K);
  printf("There are %d index bits\n", Length);
  int Offset = offsetLength(L);
  printf("There are %d offset bits\n", Offset);
  int Tag = numTagBits(Length, Offset);
  printf("There are %d Tag Bits\n", Tag);
  //Binary 11001010100110100101101001111010
  unsigned int Address = 3399113338;
  //Should return the left Tag Bits, aka 1100101010011010010110, which is 3319446
  printf("The address is %u, the left %d bits of this are %d\n", Address, Tag, tagBits(Address,Length,Offset));
  /*int i = 0;*/
  /*for(i = 0; i < 200; i++){*/
    /*int testAddress = i;*/
    /*int setNumber = whichLine(testAddress, K);*/
    /*printf("The address %d goes into line %d\n", testAddress, setNumber);*/
  /*}*/
  int lines = K;
  int sets = numberOfSets(L,K,C);
  initializeCache(tagArray, lruArray);
  /*int i,j;
  for(i = 0; i < lines; i++){
    for(j = 0; j < sets; j++){
      //printf("The tag is %d and the lru is %d\n",tagArray[i][j],lruArray[i][j]);//this is causing the segfault
    }
  }*/
  return 0;
};

//Author Zach Boynton
//Tested by Brandon Sprague
int numberOfSets(int L, int K, int C){
  return(C/(K*L));
};

//Author Zach Boynton
//Tested by Brandon Sprague
int setIndexLength(int K){
  int t = K;
  int l = 0;
  while(t>1){
    t = (t/2);
    l++;
  };
  return l;
};

//Author Brandon Sprague
int whichLine(unsigned int address, int K){
  return address % K;
}

//Author Zach Boynton
//Tested by Brandon Sprague
int offsetLength(int L){
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
int numTagBits(int i, int o){
  return (ADDRESS_SIZE-(i+o));
};

int tagBits(unsigned int address, int length, int offset){
  int numBits = numTagBits(length,offset);
  return address >> ADDRESS_SIZE - numBits;
}

int hitWay(unsigned int address, unsigned int **tagArray){
  int line = whichLine(address, K);
  int numSets = numberOfSets(L,K,C);
  int offset = offsetLength(L);
  int length = setIndexLength(K);
  unsigned int *sets = tagArray[line];
  unsigned int tag = tagBits(address, length, offset);
  int i = 0;
  while(i < numSets){
    if(sets[i] == tag){
      return i;
    }
    i++;
  }
  return -1;
}

void initializeCache(unsigned int **tagArray, int **lruArray){
  int lines = K;
  int sets = numberOfSets(L,K,C);
  int i,j;
  printf("the inticache is called\n"); //breaking past here
  //Initialize lru to lruArray[lines][sets]
  lruArray = (int**)malloc(lines*sizeof(int*));
  for(i = 0; i < lines; i++){
    lruArray[i] = (int*)malloc(sets*sizeof(int));
  }
  //Initialize tag to tagArray[lines][sets]
  tagArray = (unsigned int**)malloc(lines*sizeof(int*));
  for(i = 0; i < lines; i++){
    tagArray[i] = (int*)malloc(sets*sizeof(int));
  }

  for(i = 0; i < lines; i++){
    for(j = 0; j < sets; j++){
      //Tag is 0 and lru is -1 so we don't have false matches
      tagArray[i][j] = 0;
      lruArray[i][j] = -1;
    }
  }

  //Print out arrays
  for(i = 0; i < sets; i++){
    printf("Set %d\t", i);
  }
  printf("\n");
  for(i = 0; i < lines; i++){
    for(j = 0; j < sets; j++){
      printf("%u\t",tagArray[i][j]);
    }
    printf("\n");
  }
  for(i = 0; i < sets; i++){
    printf("Set %d\t", i);
  }
  printf("\n");
  for(i = 0; i < lines; i++){
    for(j = 0; j < sets; j++){
      printf("%d\t",lruArray[i][j]);
    }
    printf("\n");
  }
};


/*void updateOnHit(unsigned int address){
  
  };*/

