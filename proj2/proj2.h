#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>


#define sharedMemKey 1717

#define semaphoresCt 7

enum semaphores {
  resMutex,           //Used to lock the shared memory or the output file
  santaSleep,         //Used to wake santa up or let him go to sleep again
  santaWait,          //Used to wait for elves to leave or RD to get hitched
  elfMutex,           //Used to only allow three elves to enter
  allElvesOut,        //Used for waiting until there are no elves inside
  elvesReady,         //Used to wait for all three elves to enter
  hitchReindeer       //Used to hitch reindeer one by one
};

typedef struct {
  int shmid;
  int actionID;
  bool workshopOpen;
  int reindeerAway;
  int reindeerWaiting;
  int elvesWaiting;
  int elvesInside;
  int fd;
  sem_t sem[semaphoresCt];
} Mem;


/*
 * Semaphores
 */

void semaphore_init(Mem *shm){
  sem_init(&(shm->sem[resMutex]), 1, 1);
  sem_init(&(shm->sem[santaSleep]), 1, 0);
  sem_init(&(shm->sem[elfMutex]), 1, 0);
  sem_init(&(shm->sem[allElvesOut]), 1, 3);
  sem_init(&(shm->sem[elvesReady]), 1, 0);
  sem_init(&(shm->sem[hitchReindeer]), 1, 0);
  sem_init(&(shm->sem[santaWait]), 1, 0);
}

/*
 * Process management
 */

//Save PIDs of all forked processes
void savePID(pid_t *pids, pid_t nextPID){
  int i = 0;
  while(pids[i] != -2){
    i++;
  }
  pids[i] = nextPID;
}

//If an error occures, kill all child processes
void killall(pid_t *pids, int amount){
  for(int i = 0; i < amount; i++){
    kill(pids[i], SIGKILL);
  }
}

/*
 * Shared memory
 */

Mem *sharedMem_init(){
  //Create a shared memory
  int shmid = shmget(sharedMemKey, sizeof(Mem), IPC_CREAT | 0666);
  if(shmid < 0){
    return NULL;
  }
  //Attach the created shared memory
  Mem *shm = shmat(shmid, NULL, 0);
  if(shm == (Mem *) -1){
    shmctl(shmid, IPC_RMID, 0);
    return NULL;
  }
  //Save the ID of the shared memory
  shm->shmid = shmid;
  //And initialise the other values
  shm->actionID = 0;
  shm->workshopOpen = true;
  shm->reindeerAway = 0;
  shm->reindeerWaiting = 0;
  shm->elvesWaiting = 0;
  shm->elvesInside = 0;

  return shm;
}

bool sharedMem_destroy(Mem *shm){
  int shmid = shm->shmid;
  //Detach the shared memory
  if(shmdt(shm)){
    return false;
  }
  //Destroy the shared memory
  if(shmctl(shmid, IPC_RMID, 0) == -1) {
    return false;
  }
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
    fprintf(stderr, "Invalid number of arguments (4 required): %d. Exiting.\n", argc);
    printUsage();
    return false;
  }
  char *p;
  int temp;
  temp = strtol(argv[1], &p, 10);
  if(p[0] || temp <= 0 || temp >= 1000){
    fprintf(stderr, "Invalid argument (argument number 1). Exiting.\n");
    printUsage();
    return false;
  }
  temp = strtol(argv[2], &p, 10);
  if(p[0] || temp <= 0 || temp >= 20){
    fprintf(stderr, "Invalid argument (argument number 2). Exiting.\n");
    printUsage();
    return false;
  }
  for(int i = 3; i < 4; i++){
    temp = strtol(argv[i], &p, 10);
    if(p[0] || temp < 0 || temp > 1000){
      fprintf(stderr, "Invalid argument (argument number %d). Exiting.\n", i);
      printUsage();
      return false;
    }
  }
  return true;
}
