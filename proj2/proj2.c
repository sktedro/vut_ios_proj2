#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "proj2.h"


/*
 * Elf
 */

bool checkWorkshop(Mem *shm, int elfID){
  if(!shm->workshopOpen){
    (shm->elvesWaiting)--;
    printf("%d: Elf %d: taking holidays\n", ++(shm->actionID), elfID);
    return false;
  }
  return true;
}

void elfFn(int elfIndepWork, Mem *shm, int elfID){

  printf("%d: Elf %d: started\n", ++(shm->actionID), elfID);

  while(1){
    //Calculate random value between 0 and elfIndepWork 
    elfIndepWork = rand() % (elfIndepWork + 1);
    //Elf's independent working
    usleep(1000*elfIndepWork);

    //Wait for santa to help the elf (if the workshop is open)
    printf("%d: Elf %d: need help\n", ++(shm->actionID), elfID);
    (shm->elvesWaiting)++;
    while(shm->elvesWaiting != 3 && shm->workshopOpen){}

    //Go on a vacation if the workshop is closed
    if(!checkWorkshop(shm, elfID)){
      exit(0);
    }

    //Wait for other elves to clear the line
    sem_wait(&(shm->elfMutex));

    //Go on a vacation if the workshop is closed
    if(!checkWorkshop(shm, elfID)){
      exit(0);
    }

    (shm->elvesWaiting)--;
    (shm->elvesInside)++;

    printf("%d: Elf %d: get help\n", ++(shm->actionID), elfID);

    (shm->elvesInside)--;
    sem_post(&(shm->elfMutex));
  }
}

/*
 * Reindeer
 */

void reindeerFn(int reindeerVac, Mem *shm, int reindeerID){
  //Calculate random value between reindeerVac/2 and reindeerVac
  reindeerVac = rand() % ((reindeerVac/2) + (reindeerVac/2) + 1);
  //Reindeer's vacation
  usleep(1000*reindeerVac);

  //Reindeer returns home and increments the counter
  printf("%d: RD %d: return home\n", ++(shm->actionID), reindeerID);
  shm->reindeerAway -= 1;

  //Wait for santa to hitch the reindeer
  sem_wait(&(shm->reindeerMutex));
  printf("%d: RD %d: get hitched\n", ++(shm->actionID), reindeerID);
  sem_post(&(shm->reindeerMutex));

  exit(0);
}

/*
 * Main function
 */

int main(int argc, char **argv){
  if(!validateInput(argc, argv)){
    return 1;
  }
  int elves = atoi(argv[1]);
  int reindeer = atoi(argv[2]);
  int elfIndepWork = atoi(argv[3]);
  int reindeerVac = atoi(argv[4]);

  //Miscellaneous set-ups
  time_t t;
  srand((unsigned)time(&t));

  //Setting up the shared memory and initialising the values
  Mem *shm = sharedMem_init();
  shm->actionID = 0;
  shm->workshopOpen = true;
  shm->reindeerAway = reindeer;
  shm->elvesWaiting = 0;
  shm->elvesInside = 0;

  //Setting up the semaphores
  sem_init(&(shm->reindeerMutex), 1, 0);
  sem_init(&(shm->elfMutex), 1, 3);

  int mainPID = fork();
  if(mainPID == 0){
    for(int i = 0; i < elves; i++){
      if(fork() == 0){
        elfFn(elfIndepWork, shm, i + 1);
      }
    }
    for(int i = 0; i < reindeer; i++){
      if(fork() == 0){
        reindeerFn(reindeerVac, shm, i + 1);
      }
    }
    exit(0);
  }else if(mainPID > 0){ //Santa

    while(shm->workshopOpen){
      //Wait for somebody to wake him up
      while(shm->reindeerAway != 0 && shm->elvesWaiting < 3){}

      //If Santa is woken up by the elves
      while(shm->elvesInside != 0){}

      //If Santa is woken up by reindeer, close the workshop and hitch them
      if(shm->reindeerAway == 0){
        printf("%d: Santa: closing workshop\n", ++(shm->actionID));
        shm->workshopOpen = false;
        //Hitch the reindeer by unlocking the semaphore
        sem_post(&(shm->reindeerMutex));
      }
    }
  }else if(mainPID < 0){
    printf("Could not create a child process.\n");
    return 1;
  }


  //FILE *f = fopen("./proj2.out", "rw");


  //Destroy the shared semafors
  sem_destroy(&(shm->reindeerMutex));
  sem_destroy(&(shm->elfMutex));
  //Detach the shared memory
  if(!sharedMem_detach(shm)){
    printf("Detaching the shared memory failed.\n");
    return 1;
  }

  return 0;
}
