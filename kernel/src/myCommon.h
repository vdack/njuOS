#include<common.h>
#ifndef _MY_COMMON_H_
#define _MY_COMMON_H_
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
    char* name;
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

typedef struct _cpu {
    int lock_counter;
    task_t* current_task;
    int i_status;
} cpu_t;


// global dsa

cpu_t cpu_list[CPU_MAX];
task_t task_root;
spinlock_t task_lk;

// interfaces

void lock_init(lock_t* lock) {
    lock->flag = 0;
    char* name;
} 
void lock_acquire(lock_t* lock) {
    while(atomic_xchg(&lock->flag, MY_LOCKED) == MY_LOCKED);
}
void lock_release(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_UNLOCKED);
    panic_on(rc == MY_UNLOCKED, "failed! release before acquire!!!\n");
}
bool try_lock_acquire(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_LOCKED);
    return rc == MY_UNLOCKED;
}


static void spin_init(spinlock_t* lk, const char* name) {
    lk->flag = MY_UNLOCKED;
    lk->name = name;
    lk->holder = -1;
}
static void spin_lock(spinlock_t* lk) {
    while(1){
        iset(false);
        if (atomic_xchg(&lk->flag, MY_LOCKED) == MY_LOCKED) {
            iset(true);
        } else {
            break;
        }
    }

    int current = cpu_current();
    
    lk->holder = current;

    cpu_list[current].lock_counter += 1;
    if (cpu_list[current].lock_counter == 1) {
        // cpu acquire a lock for the first time.
        cpu_list[current].i_status = ienabled();
        iset(false);
    }
    return;
}
static void spin_unlock(spinlock_t* lk) {
    int current = cpu_current();
    
    int res = atomic_xchg(&lk->flag, MY_UNLOCKED);
    panic_on(res = MY_UNLOCKED || lk->holder != current, "Error when unlock a spinlock.\n");
    lk->holder = -1;

    cpu_list[current].lock_counter -= 1;
    if (cpu_list[current].lock_counter == 0) {
        iset(cpu_list[current].i_status);
    }

}



task_t* cpu_move_task() {
    task_t* temp = cpu_list[cpu_current()].current_task;
    cpu_list[cpu_current()].current_task = NULL;
    return temp;
}

void set_task(task_t* task) {
    if (cpu_list[cpu_current()].current_task != NULL) {
        panic("there are two taskes at the same time!");
    }
    cpu_list[cpu_current()].current_task = task;
}

#endif