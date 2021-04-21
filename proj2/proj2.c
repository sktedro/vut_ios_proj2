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

enum semaphores {reindeerMutex, elfMutex, santaSleepMutex, shmMutex,
                  elvesReady, helpElvesMutex};

/*
 * Elf
 */

void checkWorkshop(Mem *shm, int elfID){
  if(!shm->workshopOpen){
    sem_wait(&(shm->sem[shmMutex]));
    (shm->elvesWaiting)--;
    printf("%d: Elf %d: taking holidays\n", ++(shm->actionID), elfID);
    sem_post(&(shm->sem[shmMutex]));
    exit(0);
  }
}

void elfFn(int elfIndepWork, Mem *shm, int elfID){

  sem_wait(&(shm->sem[shmMutex]));
  printf("%d: Elf %d: started\n", ++(shm->actionID), elfID);
  sem_post(&(shm->sem[shmMutex]));

  while(1){
    //Calculate random value between 0 and elfIndepWork and let the elf work
    if(elfIndepWork != 0){
      usleep(1000*(rand() % (elfIndepWork + 1)));
    }

    //Wait for santa to help the elf (if the workshop is open)
    sem_wait(&(shm->sem[shmMutex]));
    printf("%d: Elf %d: need help\n", ++(shm->actionID), elfID);
    sem_post(&(shm->sem[shmMutex]));

    //Go on a vacation if the workshop is closed
    checkWorkshop(shm, elfID);

    sem_wait(&(shm->sem[shmMutex]));
    bool thirdElf = false;
    if(shm->elvesWaiting == 2){
      thirdElf = true;
      (shm->elvesWaiting) -= 3;
    }
    else{
      (shm->elvesWaiting)++;
    }
    sem_post(&(shm->sem[shmMutex]));


    if(thirdElf){
      for(int i = 0; i < 3; i++){
        sem_post(&(shm->sem[elfMutex]));
      }
    }
    sem_wait(&(shm->sem[elfMutex]));

    checkWorkshop(shm, elfID);

    sem_wait(&(shm->sem[shmMutex]));
    (shm->elvesInside)++;
    sem_post(&(shm->sem[shmMutex]));

    if(shm->elvesInside == 3){
      sem_post(&(shm->sem[santaSleepMutex])); //Wake santa up
      for(int i = 0; i < 3; i++){
        sem_post(&(shm->sem[elvesReady]));
      }
    }

    sem_wait(&(shm->sem[elvesReady])); //Wait for all three elves to enter

    sem_wait(&(shm->sem[shmMutex]));
    printf("%d: Elf %d: GET help\n", ++(shm->actionID), elfID);
    (shm->elvesInside)--;

    if(shm->elvesInside == 0){
      sem_post(&(shm->sem[santaSleepMutex])); //Allow santa to go to sleep again
    }
    sem_post(&(shm->sem[shmMutex]));



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
  sem_wait(&(shm->sem[shmMutex]));
  printf("%d: RD %d: return home\n", ++(shm->actionID), reindeerID);
  shm->reindeerAway -= 1;
  sem_post(&(shm->sem[shmMutex]));

  if(shm->reindeerAway == 0){
    sem_post(&(shm->sem[santaSleepMutex]));
  }

  //Wait for santa to hitch the reindeer
  sem_wait(&(shm->sem[reindeerMutex]));
  sem_wait(&(shm->sem[shmMutex]));
  printf("%d: RD %d: get hitched\n", ++(shm->actionID), reindeerID);
  sem_post(&(shm->sem[shmMutex]));
  sem_post(&(shm->sem[reindeerMutex]));

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
  sem_init(&(shm->sem[reindeerMutex]), 1, 0);
  sem_init(&(shm->sem[elfMutex]), 1, 0);
  sem_init(&(shm->sem[santaSleepMutex]), 1, 0);
  sem_init(&(shm->sem[shmMutex]), 1, 1);
  sem_init(&(shm->sem[elvesReady]), 1, 0);
  sem_init(&(shm->sem[helpElvesMutex]), 1, 3);





  int mainPID = fork();
  if(mainPID < 0){
    printf("Could not create a child process.\n");
    return 1;
  }else if(mainPID == 0){
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
  }else{ //Santa
    while(shm->workshopOpen){
      sem_wait(&(shm->sem[shmMutex]));
      printf("Santa going to sleep\n");
      sem_post(&(shm->sem[shmMutex]));
      //Wait for somebody to wake him up
      //while(shm->reindeerAway != 0 && shm->elvesWaiting < 3){}
      sem_wait(&(shm->sem[santaSleepMutex]));

      //If Santa is woken up by the elves, wait for them to leave
      if(shm->elvesInside != 0){
        sem_wait(&(shm->sem[santaSleepMutex]));
      }
      //If Santa is woken up by reindeer, close the workshop and hitch them
      else if(shm->reindeerAway == 0){
        sem_wait(&(shm->sem[shmMutex]));
        printf("%d: Santa: closing workshop\n", ++(shm->actionID));
        shm->workshopOpen = false;
        sem_post(&(shm->sem[shmMutex]));

        //Hitch the reindeer by unlocking the semaphore
        sem_post(&(shm->sem[reindeerMutex]));

        //Necessary so that elves don't have to wait for the third elf to tell
        //them the workshop is closed
        for(int i = 0; i < elves; i++){
          sem_post(&(shm->sem[elfMutex]));
          sem_post(&(shm->sem[elvesReady]));
        }
      }
    }
  }



  sleep(1);//TODO



  //Destroy the semafors
  for(int i = 0; i < semaphoresCt; i++){
    sem_destroy(&(shm->sem[i]));
  }
  //Detach the shared memory
  if(!sharedMem_detach(shm)){
    printf("Detaching the shared memory failed.\n");
    return 1;
  }

  return 0;
}
