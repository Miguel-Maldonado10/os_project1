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
  MemoryCreate();
  struct sigaction sa;

  sa.sa_handler = SigHandler2;
  sa.sa_flags = 0;

  if (sigaction(SIGUSR2, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  printf("air_control sig usr2 handler configured\n");

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