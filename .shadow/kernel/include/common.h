#ifndef _COMMON_H_
#define _COMMON_H_

#define TRACE_F

#ifdef TRACE_F
    #define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
    #define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
    #define DEBUG(...) printf(__VA_ARGS__)
#else
    #define TRACE_ENTRY ((void)0)
    #define TRACE_EXIT ((void)0)
    #define DEBUG(...) ;
#endif


#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

// macros
#define MY_LOCKED 1
#define MY_UNLOCKED 0

#define CPU_MAX 8
#define STACK_SIZE (1<<12)

// type definition
enum task_status {T_RUNNING, T_SLEEPING, T_CREATE, T_BLOCKED, T_DEAD};
typedef struct task{
    enum task_status status;
    const char* name;
    void (*entry) (void* arg);
    void* arg;
    Context* context;
    struct task* next;
    uint8_t* stack;
} task_t;

typedef struct spinlock {
    int flag;
    const char* name;
    int holder;
} spinlock_t;

typedef struct _waitlist {
    task_t* head;
    task_t* tail;
    spinlock_t lk;
} waitlist_t;

typedef struct _cpu {
    int lock_counter;
    task_t* current_task;
    int i_status;
} cpu_t;

typedef struct semaphore {
    spinlock_t lk;
    const char* name;
    int value;
    waitlist_t sleep_list;
} sem_t;

extern cpu_t cpu_list[CPU_MAX];
extern waitlist_t task_list;



#endif