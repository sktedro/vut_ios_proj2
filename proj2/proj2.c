#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "proj2.h"

/*
 * Santa
 */

void santaFn(Mem *shm, int fd, int elves){
  while(shm->workshopOpen){
    //First, go to sleep
    sem_wait(&(shm->sem[resMutex]));
    dprintf(fd, "%d: Santa: going to sleep\n", ++(shm->actionID));
    fsync(fd);
    sem_post(&(shm->sem[resMutex]));

    //Wait for somebody to wake him up
    sem_wait(&(shm->sem[santaSleep]));

    //If Santa is woken up by reindeer, close the workshop and hitch them
    if(shm->reindeerAway == 0){
      //Close the workshop
      sem_wait(&(shm->sem[resMutex]));
      dprintf(fd, "%d: Santa: closing workshop\n", ++(shm->actionID));
      fsync(fd);
      shm->workshopOpen = false;
      sem_post(&(shm->sem[resMutex]));

      //Hitch the reindeer by unlocking the semaphore
      sem_post(&(shm->sem[hitchReindeer]));

      //Necessary so that elves don't have to wait for the third elf to tell
      //them the workshop is closed
      for(int i = 0; i < elves; i++){
        sem_post(&(shm->sem[elfMutex]));
        sem_post(&(shm->sem[allElvesOut]));
        sem_post(&(shm->sem[elvesReady]));
      }

      //Wait until ALL reindeer are hitched
      sem_wait(&(shm->sem[santaWait]));

      //Start Christmas
      sem_wait(&(shm->sem[resMutex]));
      dprintf(fd, "%d: Santa: Christmas started\n", ++(shm->actionID));
      fsync(fd);
      sem_post(&(shm->sem[resMutex]));
    }


    //If Santa is woken up by the elves
    else if(shm->elvesInside != 0){
      //Print that the santa is about to help the elves
      sem_wait(&(shm->sem[resMutex]));
      dprintf(fd, "%d: Santa: helping elves\n", ++(shm->actionID));
      fsync(fd);
      sem_post(&(shm->sem[resMutex]));

      //Take all of those 3 elves inside
      for(int i = 0; i < 3; i++){
        sem_post(&(shm->sem[elvesReady])); 
      }

      //Wait for the elves to leave
      sem_wait(&(shm->sem[santaWait]));
    }
  }
  exit(0);
}

/*
 * Elves
 */

//Check the workshop - if it is closed, go on a holiday
void checkWorkshop(Mem *shm, int elfID, int fd){
  if(!shm->workshopOpen){
    sem_wait(&(shm->sem[resMutex]));
    (shm->elvesWaiting)--;
    dprintf(fd, "%d: Elf %d: taking holidays\n", ++(shm->actionID), elfID);
    fsync(fd);
    sem_post(&(shm->sem[resMutex]));
    exit(0);
  }
}

void elfFn(int elfIndepWork, Mem *shm, int elfID, int fd){

  sem_wait(&(shm->sem[resMutex]));
  dprintf(fd, "%d: Elf %d: started\n", ++(shm->actionID), elfID);
  fsync(fd);
  sem_post(&(shm->sem[resMutex]));

  while(1){
    //Calculate random value between 0 and elfIndepWork and let the elf work
    if(elfIndepWork != 0){
      usleep(1000*(rand() % (elfIndepWork + 1)));
    }

    //Wait for santa to help the elf (if the workshop is open)
    sem_wait(&(shm->sem[resMutex]));
    dprintf(fd, "%d: Elf %d: need help\n", ++(shm->actionID), elfID);
    fsync(fd);
    sem_post(&(shm->sem[resMutex]));

    //Wait until there are no elves inside
    sem_wait(&(shm->sem[allElvesOut]));

    bool thirdElf = false;
    sem_wait(&(shm->sem[resMutex]));
    (shm->elvesWaiting)++;
    //If this process is the third elf in line, go inside with the other two
    //(unlocking elfMutex two times allows those two to enter)
    if(shm->elvesWaiting == 3){
      thirdElf = true;
      (shm->elvesWaiting) -= 3;
      for(int i = 0; i < 2; i++){
        sem_post(&(shm->sem[elfMutex]));
      }
    }
    sem_post(&(shm->sem[resMutex]));

    //First two elves wait for the third one
    if(!thirdElf){
      sem_wait(&(shm->sem[elfMutex]));
    }

    //Go on a vacation if the workshop is closed
    checkWorkshop(shm, elfID, fd);

    sem_wait(&(shm->sem[resMutex]));
    //Go inside and wake santa up by unlocking santaSleep
    (shm->elvesInside)++;
    if(shm->elvesInside == 3){
      sem_post(&(shm->sem[santaSleep]));
    }
    sem_post(&(shm->sem[resMutex]));

    //Wait for all three elves to be in the same state
    sem_wait(&(shm->sem[elvesReady])); 

    //Go on a vacation if the workshop is closed
    checkWorkshop(shm, elfID, fd);

    //Get help and get out
    sem_wait(&(shm->sem[resMutex]));
    dprintf(fd, "%d: Elf %d: get help\n", ++(shm->actionID), elfID);
    fsync(fd);
    (shm->elvesInside)--;

    //Allow santa to go to sleep again
    if(shm->elvesInside == 0){
      sem_post(&(shm->sem[santaWait])); 
    }
    sem_post(&(shm->sem[resMutex]));

    //Allow next three elves to get in line
    sem_post(&(shm->sem[allElvesOut]));

  }
}

/*
 * Reindeer
 */

void reindeerFn(int reindeerVac, Mem *shm, int reindeerID, int fd){

  sem_wait(&(shm->sem[resMutex]));
  dprintf(fd, "%d: RD %d: rstarted\n", ++(shm->actionID), reindeerID);
  fsync(fd);
  sem_post(&(shm->sem[resMutex]));

  //Calculate random value between reindeerVac/2 and reindeerVac and go on vac
  if(reindeerVac != 0){
    usleep(1000*(rand() % ((reindeerVac/2) + (reindeerVac/2) + 1)));
  }

  //Reindeer returns home
  sem_wait(&(shm->sem[resMutex]));
  dprintf(fd, "%d: RD %d: return home\n", ++(shm->actionID), reindeerID);
  fsync(fd);
  (shm->reindeerAway)--;
  (shm->reindeerWaiting)++;
  sem_post(&(shm->sem[resMutex]));

  //If all reindeer arrived, wake santa up
  if(shm->reindeerAway == 0){
    sem_post(&(shm->sem[santaSleep]));
  }

  //Wait for santa to hitch the reindeer
  sem_wait(&(shm->sem[hitchReindeer]));
  sem_wait(&(shm->sem[resMutex]));
  dprintf(fd, "%d: RD %d: get hitched\n", ++(shm->actionID), reindeerID);
  fsync(fd);
  (shm->reindeerWaiting)--;
  //If they are all hitched, let the santa know
  if(shm->reindeerWaiting == 0){
    sem_post(&(shm->sem[santaWait])); 
  }
  sem_post(&(shm->sem[resMutex]));
  sem_post(&(shm->sem[hitchReindeer]));

  exit(0);
}

/*
 * Main function
 */

int main(int argc, char **argv){
  //Check the arguments
  if(!validateInput(argc, argv)){
    return 1;
  }

  //Get the arguments
  int elves = atoi(argv[1]);
  int reindeer = atoi(argv[2]);
  int elfIndepWork = atoi(argv[3]);
  int reindeerVac = atoi(argv[4]);

  //Init the rand() function
  srand(time(NULL));
  //err is set to 1 if an error occured
  int err = 0; 

  //All the PIDs will be saved to this variable
  pid_t *pids = malloc((elves + reindeer + 1)*sizeof(pid_t));
  if(pids == NULL){
    fprintf(stderr, "Could not allocate necessary memory. Exiting.\n");
    return 1;
  }
  for(int i = 0; i < (elves + reindeer + 1); i++){
    pids[i] = -2; //-2 is taken as a initial value
  }

  //Setting up the shared memory and initialising the values
  Mem *shm = sharedMem_init();
  if(shm == NULL){
    fprintf(stderr, "Shared memory could not be created. Exiting.\n");
    free(pids);
    return 1;
  }
  shm->reindeerAway = reindeer;

  //Setting up the semaphores
  semaphore_init(shm);

  //Open the log file and get the file descriptor
  int fd = open("proj2.out", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  //If file could not be opened
  if(fd == -1){
    fprintf(stderr, "File could not be opened.\n");
    for(int i = 0; i < semaphoresCt; i++){
      sem_destroy(&(shm->sem[i]));
    }
    if(!sharedMem_destroy(shm)){
      fprintf(stderr, "Destroying the shared memory failed.\n");
    }
    free(pids);
    return 1;
  }

  //Create the child processes:
  pid_t child_pid;
  //Elves
  for(int i = 0; i < elves; i++){
    if((child_pid = fork()) == 0){
      elfFn(elfIndepWork, shm, i + 1, fd);
    }
    else if(child_pid < 0){
      fprintf(stderr, "Could not create a child process.\n");
      goto cleanup;
    }else{
      savePID(pids, child_pid);
    }
  }
  //Reindeer
  for(int i = 0; i < reindeer; i++){
    if((child_pid = fork()) == 0){
      reindeerFn(reindeerVac, shm, i + 1, fd);
    }
    else if(child_pid < 0){
      fprintf(stderr, "Could not create a child process.\n");
      goto cleanup;
    }else{
      savePID(pids, child_pid);
    }
  }
  //Santa
  if((child_pid = fork()) == 0){
    santaFn(shm, fd, elves);
  }else if(child_pid < 0){
    fprintf(stderr, "Could not create a child process.\n");
    goto cleanup;
  }else{
    savePID(pids, child_pid);
  }

  //Wait for all child processes to exit if no error was encountered
  if(err == 0){
    while(wait(NULL) != -1);
  }

cleanup:

  //Close the file
  if(close(fd) == -1){
    fprintf(stderr, "Could not close the log file.\n");
    err = 1;
  }

  //Destroy the semaphores
  for(int i = 0; i < semaphoresCt; i++){
    if(sem_destroy(&(shm->sem[i])) == -1){
      fprintf(stderr, "Could not destroy one or more semaphors.\n");
      err = 1;
    }
  }

  //Detach the shared memory
  if(!sharedMem_destroy(shm)){
    fprintf(stderr, "Destroying the shared memory failed.\n");
    err = 1;
  }

  //Kill all child processes if an error occured
  if(err != 0){
    killall(pids, elves + reindeer + 1);
    free(pids);
    return 1;
  }

  //Free the memory allocated for PIDs
  free(pids);

  return 0;
}
