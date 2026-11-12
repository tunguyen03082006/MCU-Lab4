#include "scheduler.h"
#include "main.h"

typedef struct {
    void (*pTask)(void);
    uint32_t Delay;     // delta (tick) so với task đứng trước
    uint32_t Period;    // tick, 0 = one-shot
    uint32_t TaskID;
    int8_t   next;      // chỉ số task kế tiếp trong danh sách, -1 = hết
} sTask;

static sTask    pool[SCH_MAX_TASKS];
static int8_t   head = -1;
static uint32_t nextID = 0;
static volatile uint32_t sys_ticks = 0;
uint8_t Error_code_G = 0;

void SCH_Init(void) {
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        pool[i].pTask = 0; pool[i].Delay = 0;
        pool[i].Period = 0; pool[i].TaskID = NO_TASK_ID; pool[i].next = -1;
    }
    head = -1;
    sys_ticks = 0;
    Error_code_G = 0;
}

uint32_t get_time(void) { return sys_ticks * TICK_MS; }

/* Chèn task idx vào danh sách đã sắp xếp theo delay tương đối.
   Gọi khi đã tắt ngắt. */
static void insert(int8_t idx, uint32_t delay) {
    int8_t prev = -1, cur = head;
    while (cur != -1 && delay >= pool[cur].Delay) {
        delay -= pool[cur].Delay;
        prev = cur;
        cur = pool[cur].next;
    }
    pool[idx].Delay = delay;
    pool[idx].next  = cur;
    if (cur != -1) pool[cur].Delay -= delay;
    if (prev == -1) head = idx; else pool[prev].next = idx;
}

/* O(1): chỉ giảm delay của task đầu danh sách */
void SCH_Update(void) {
    sys_ticks++;
    if (head != -1 && pool[head].Delay > 0) {
        pool[head].Delay--;
    }
}

uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD) {
    int8_t idx = -1;
    __disable_irq();
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        if (pool[i].pTask == 0) { idx = i; break; }
    }
    if (idx == -1) {
        Error_code_G = 1;               // ERROR_SCH_TOO_MANY_TASKS
        __enable_irq();
        return NO_TASK_ID;
    }
    pool[idx].pTask  = pFunction;
    pool[idx].Period = PERIOD / TICK_MS;
    nextID++;
    if (nextID == NO_TASK_ID) nextID++;
    pool[idx].TaskID = nextID;
    insert(idx, DELAY / TICK_MS);
    __enable_irq();
    return pool[idx].TaskID;
}

void SCH_Dispatch_Tasks(void) {
    while (1) {
        void (*run)(void) = 0;

        __disable_irq();
        if (head != -1 && pool[head].Delay == 0) {
            int8_t idx = head;
            head = pool[idx].next;       // lấy task ra khỏi danh sách
            run = pool[idx].pTask;
            if (pool[idx].Period > 0) {
                insert(idx, pool[idx].Period);   // task định kỳ: chèn lại
            } else {
                pool[idx].pTask = 0;             // one-shot: giải phóng
                pool[idx].TaskID = NO_TASK_ID;
            }
        }
        __enable_irq();

        if (run == 0) break;
        run();
    }
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == NO_TASK_ID) return 0;
    int8_t prev = -1, cur;
    __disable_irq();
    cur = head;
    while (cur != -1 && pool[cur].TaskID != taskID) {
        prev = cur;
        cur = pool[cur].next;
    }
    if (cur == -1) { Error_code_G = 2; __enable_irq(); return 0; }  // không tìm thấy
    if (pool[cur].next != -1) pool[pool[cur].next].Delay += pool[cur].Delay;
    if (prev == -1) head = pool[cur].next; else pool[prev].next = pool[cur].next;
    pool[cur].pTask = 0;
    pool[cur].TaskID = NO_TASK_ID;
    pool[cur].next = -1;
    __enable_irq();
    return 1;
}
