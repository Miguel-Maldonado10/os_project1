#ifndef INCLUDE_FUNCTIONS_H_
#define INCLUDE_FUNCTIONS_H_

void MemoryCreate();
void SigHandler2(int signal);
void Set_Radio_PID(int pid);
void Set_Ground_Control_PID(int pid);
void* TakeOffsFunction(void* arg);

#endif  // INCLUDE_FUNCTIONS_H_