#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

// macros
#define MY_LOCKED 1
#define MY_UNLOCKED 0

#define CPU_MAX 8
#define STACK_SIZE (1<<12)

// type definition

typedef struct _lock_type {
    int flag;
} lock_t;

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

typedef struct semaphore {
    spinlock_t lk;
    const char* name;
    int value;
    task_t* wait_list;
} sem_t;

typedef struct _cpu {
    int lock_counter;
    task_t* current_task;
    int i_status;
} cpu_t;


// global dsa


#endif