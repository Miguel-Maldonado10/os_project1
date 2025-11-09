#ifndef AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_
#define AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_

void MemoryCreate();
void SigHandler2(int signal);
void Set_Radio_PID(int pid);
void Set_Ground_Control_PID(int pid);
void* TakeOffsFunction(void* arg);

#endif  // AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_