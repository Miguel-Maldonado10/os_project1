#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>

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
}