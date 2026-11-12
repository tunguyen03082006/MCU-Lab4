#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

#define SCH_MAX_TASKS   10
#define NO_TASK_ID      0
#define TICK_MS         10      // chu kỳ ngắt timer (ms)

void     SCH_Init(void);
void     SCH_Update(void);                       // gọi trong ISR timer
void     SCH_Dispatch_Tasks(void);               // gọi trong while(1)
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD); // đơn vị: ms
uint8_t  SCH_Delete_Task(uint32_t taskID);
uint32_t get_time(void);                         // thời gian hệ thống (ms)

#endif
