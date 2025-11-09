#include <fcntl.h>  // For O_* constants>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 3 * sizeof(int)
#define TOTAL_TAKEOFFS 20

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int shm_fd;
int *shm_ptr;
pthread_mutex_t state_lock;
pthread_mutex_t runway1_lock;
pthread_mutex_t runway2_lock;

void MemoryCreate() {
  shm_unlink("/air_control_shm");

  shm_fd = shm_open("/air_control_shm", O_CREAT | O_RDWR, 0666);
  if (ftruncate(shm_fd, SHM_SIZE) == -1) {
    perror("ftruncate failed");
    exit(1);
  }
  if (shm_fd == -1) {
    perror("shmget failed");
    exit(1);
  }
  shm_ptr = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                        shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }
  shm_ptr[0] = getpid();
  shm_ptr[1] = 0;  // radio PID
  shm_ptr[2] = 0;  // ground_control PID
}

void Set_Radio_PID(int pid) { shm_ptr[1] = pid; }

void Set_Ground_Control_PID(int pid) { shm_ptr[2] = pid; }

void SigHandler2(int signal) {
  planes += 5;
  printf("SIGUSR2 received: planes on runway increased to %d\n", planes);
}
void *TakeOffsFunction(void *arg) {
  while (total_takeoffs <= TOTAL_TAKEOFFS) {
    // printf("Thread %ld attempting to acquire runway lock.
    // Planes on runway: %d\n", pthread_self(), planes);
    if (pthread_mutex_trylock(&runway1_lock) != 0) {
      if (pthread_mutex_trylock(&runway2_lock) != 0) {
        // printf("Both runways busy, thread %ld waiting...\n", pthread_self());
        usleep(1000);
      } else {  // runway 2 locked
        // Takeoff from runway 2
        pthread_mutex_lock(&state_lock);
        if (total_takeoffs >= TOTAL_TAKEOFFS) {
          pthread_mutex_unlock(&state_lock);
          pthread_mutex_unlock(&runway2_lock);
          break;
        }
        planes--;
        takeoffs++;
        total_takeoffs++;
        printf("Takeoff from runway 2. Planes on runway: %d. Local takeoffs: "
               "%d. Total takeoffs: %d\n",
               planes, takeoffs, total_takeoffs);
        if (takeoffs % 5 == 0) {
          kill(shm_ptr[2], SIGUSR1);  // Notify ground_control
          kill(shm_ptr[1], SIGUSR1);  // Notify ground_control
        }
        pthread_mutex_unlock(&state_lock);
        sleep(1);  // Simulate takeoff time
        pthread_mutex_unlock(&runway2_lock);
      }
    } else {  // runway 1 locked
      // Takeoff from runway 1
      pthread_mutex_lock(&state_lock);
      if (total_takeoffs >= TOTAL_TAKEOFFS && planes > 0) {
        pthread_mutex_unlock(&state_lock);
        pthread_mutex_unlock(&runway1_lock);
        break;
      }
      planes--;
      takeoffs++;
      total_takeoffs++;
      printf("Takeoff from runway 1. Planes on runway: %d. Local takeoffs: %d. "
             "Total takeoffs: %d\n",
             planes, takeoffs, total_takeoffs);
      if (takeoffs % 5 == 0) {
        kill(shm_ptr[2], SIGUSR1);  // Notify ground_control
        kill(shm_ptr[1], SIGUSR1);
      }
      pthread_mutex_unlock(&state_lock);
      sleep(1);  // Simulate takeoff time
      pthread_mutex_unlock(&runway1_lock);
    }
  }
  kill(shm_ptr[1], SIGTERM);  // Notify radio that takeoffs are done
  close(shm_fd);
  munmap(shm_ptr, SHM_SIZE);
  return NULL;
}
