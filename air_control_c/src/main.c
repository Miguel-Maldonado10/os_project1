#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include"../include/functions.h"


int main() {
  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;

  sa.sa_handler = SigHandler2;
  sa.sa_flags = 0;

  if (sigaction(SIGUSR2, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  
  printf("air_control sig usr2 handler configured\n");


  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.

  pid_t radio_pid = fork();
  if (radio_pid < 0) {
    perror("fork failed");
    exit(1);
  } else if (radio_pid == 0) {
    execl("./radio", "radio", "/air_control_shm", NULL);
    perror("execl failed");
    exit(1);
  }

  Set_Radio_PID(radio_pid);
  printf("radio launched with PID %d\n", radio_pid);

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().

  pthread_t takeoff_threads[5];
  for (int i = 0; i < 5; i++) {
    pthread_create(&takeoff_threads[i], NULL, TakeOffsFunction, NULL);
    // printf("takeoff thread %d launched\n", i);
  }
  for (int i = 0; i < 5; i++) {
    pthread_join(takeoff_threads[i], NULL);
  }
  shm_unlink("/air_control_shm");
}