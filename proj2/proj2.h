#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define sharedMemSize 1024
#define sharedMemKey 1717

  
typedef struct {
  int shmid;
  int actionID;
  bool workshopOpen;
  int reindeerAway;
  int elvesWaiting;
  int elvesInside;
  sem_t reindeerMutex;
  sem_t elfMutex;
} Mem;


/*
 * Shared memory
 */

Mem *sharedMem_init(){
  int shmid = shmget(sharedMemKey, sharedMemSize, IPC_CREAT | 0666);
  if(shmid < 0){
    printf("Shared memory could not be created. Exiting.\n");
    return NULL;
  }

  Mem *shm = shmat(shmid, NULL, 0);
  if(shm == (Mem *) -1){
    printf("Shared memory could not be created. Exiting.\n");
    return NULL;
  }

  shm->shmid = shmid;
  return shm;
}

bool sharedMem_detach(Mem *shm){
  int shmid = shm->shmid;
  if(shmdt(shm)){
    return false;
  }
  shmctl(shmid, IPC_RMID, NULL);
  return true;
}

/*
 * Usage and input validation
 */

void printUsage(){
  printf("Usage: ./proj2 NE NR TE TR, while:\n");
  printf("\t NE = number of elves\n");
  printf("\t NR = number of reindeer\n");
  printf("\t TE = max time in ms for which the elf works independently\n");
  printf("\t TR = max time in ms of a reindeer's vacation\n");
}

bool validateInput(int argc, char **argv){
  if(argc != 5){
    printf("Invalid number of arguments (4 required): %d. Exiting.\n", argc);
    printUsage();
    return false;
  }
  char *p;
  int temp;
  temp = strtol(argv[1], &p, 10);
  if(p[0] || temp <= 0 || temp >= 1000){
    printf("Invalid argument (argument number 1). Exiting.\n");
    printUsage();
    return false;
  }
  temp = strtol(argv[2], &p, 10);
  if(p[0] || temp <= 0 || temp >= 20){
    printf("Invalid argument (argument number 2). Exiting.\n");
    printUsage();
    return false;
  }
  for(int i = 3; i < 4; i++){
    temp = strtol(argv[i], &p, 10);
    if(p[0] || temp < 0 || temp > 1000){
      printf("Invalid argument (argument number %d). Exiting.\n", i);
      printUsage();
      return false;
    }
  }
  return true;
}

