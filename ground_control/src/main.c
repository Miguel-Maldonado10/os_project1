#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#define PLANES_LIMIT 20
#define SHM_SIZE 3 * sizeof(int)

int planes = 0;
int takeoffs = 0;
int traffic = 0;

int shm_fd;
int* shm_ptr;

void SigTermHandler(int signum) {
    close(shm_fd);
    printf("finalization of operations...\n");
    exit(0);
  };

void SigUsr1Handler(int signum) {
  takeoffs += 5;
}

void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
  // printf("Traffic handler invoked\n");
  // printf("Planes: %d, Takeoffs: %d\n", planes, takeoffs);
  traffic += 1;
  if (planes >= 10) {
    printf("RUNWAY OVERLOADED\n");
  }
  if (planes < PLANES_LIMIT) {
    // printf("Adding 5 planes to the runway\n");
    planes += 5;
    pid_t radio_pid = shm_ptr[1];
    if (radio_pid > 0) {
      // printf("sendig Sig 2 to the raido PID to add planes");
      kill(radio_pid, SIGUSR2);
    }
  }
  // else {
  //   // printf("Cannot add more planes, limit reached\n");
  // }
}

int main(int argc, char* argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  shm_fd = shm_open("/air_control_shm", O_RDWR, 0666);
  if(ftruncate(shm_fd, SHM_SIZE) == -1) {
    perror("ftruncate failed");
    exit(1);
  }
  shm_ptr = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }
  shm_ptr[2] = getpid();

  // 3. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.

  struct sigaction sa_term;
  sa_term.sa_handler = SigTermHandler;
  sa_term.sa_flags = 0;

  struct sigaction sa_usr1;
  sa_usr1.sa_handler = SigUsr1Handler;
  sa_usr1.sa_flags = 0;

  if (sigaction(SIGTERM, &sa_term, NULL) == -1) {
    perror("sigaction SIGTERM");
    exit(1);
  }

  if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
    perror("sigaction SIGUSR1");
    exit(1);
  }

  // 2. Configure the timer to execute the Traffic function.

  //   Configure setitimer to run Traffic every 500 ms.
  struct sigaction sa_traffic;
  sa_traffic.sa_handler = Traffic;
  sa_traffic.sa_flags = 0;

  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000;

  if (sigaction(SIGALRM, &sa_traffic, NULL) == -1) {
    perror("sigaction SIGALRM");
    exit(1);
  }

  if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
    perror("setitimer");
    exit(1);
  } 

  // 4. Infinite loop to keep the program running.
  while (1) {
    pause();  // Wait for signals
  }

  // Traffic:
  // a.    Calculate the number of planes waiting.
  // b.    If planes in line >= 10, print "RUNWAY OVERLOADED".
  // c.    If planes < PLANES_LIMIT, increase planes by 5 and send SIGUSR2 to the radio.
}