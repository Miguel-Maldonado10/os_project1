import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
shm_data = []

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
SHM_LENGTH = ctypes.sizeof(ctypes.c_int) * 3

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()
state_lock = threading.Lock()
planes_cv = threading.Condition(state_lock)

def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    shm_name = "/air_control_shm".encode("utf-8")
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    old_umask = os.umask(0o011)
    # 3. Use _libc.shm_open to create the shared memory
    fd = _libc.shm_open(shm_name, os.O_CREAT | os.O_RDWR, 0o777)
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    _libc.ftruncate(fd, SHM_LENGTH)
    # 5. Restore the original permission mask (umask)
    os.umask(old_umask)
    # 6. Use mmap to map the shared memory
    memory = mmap.mmap(fd, SHM_LENGTH, mmap.MAP_SHARED, mmap.PROT_WRITE | mmap.PROT_READ)
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    mv_var = memoryview(memory).cast('i')
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return fd, memory, mv_var

def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # increment planes and notify waiting threads
    with planes_cv:
        planes += 5
        planes_cv.notify_all()


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs

    # TODO: implement the logic to control a takeoff thread
    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    while True:
        # wait until there is a plane available or total reached
        with planes_cv:
            while planes <= 0 and total_takeoffs < TOTAL_TAKEOFFS:
                planes_cv.wait(timeout=0.1)
            if total_takeoffs >= TOTAL_TAKEOFFS:
                break
            # consume one plane safely
            planes -= 1
            total_takeoffs += 1
            takeoffs += 1

        # acquire a runway (try runway1 first, otherwise block on runway2)
        if runway1_lock.acquire(blocking=False):
            runway_lock = runway1_lock
        else:
            runway2_lock.acquire()
            runway_lock = runway2_lock
        time.sleep(1)

        # Send SIGUSR1 every 5 local takeoffs (signal radio)
        if takeoffs >= 5:
            os.kill(shm_data[1], signal.SIGUSR1)
            takeoffs = 0

        # If overall target reached, notify radio to stop
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                os.kill(shm_data[1], signal.SIGTERM)
        runway_lock.release()


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    def unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})
    radio_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "radio"))

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen([radio_path, "/air_control_shm"], 
                               preexec_fn=unblock_sigusr2)
    return process


def main():
    global shm_data

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    signal.signal(signal.SIGUSR2, HandleUSR2)
    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, memory, data = create_shared_memory()
    shm_data = data
    shm_data[0] = os.getpid()
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    radio_process = launch_radio()
    shm_data[1] = radio_process.pid
    # TODO 9: Create and start takeoff controller threads (STRIPS)
    threads = []
    for i in range(STRIPS):
        thread = threading.Thread(target=TakeOffFunction, args=(i,))
        threads.append(thread)
        thread.start()
    # TODO 10: Wait for all threads to finish their work
    for thread in threads:
        thread.join()
    # TODO 11: Release shared memory and close resources
    shm_data = None
    del data
    memory.close()
    shm_name = b"/air_control_shm"
    _libc.shm_unlink(shm_name)
    os.close(fd)
    pass


if __name__ == "__main__":
    main()