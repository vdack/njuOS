#ifndef _COMMON_H_
#define _COMMON_H_
#include "typeDef.h"
// interfaces

extern cpu_t cpu_list[CPU_MAX];
extern task_t task_root;
extern spinlock_t task_lk;




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
    panic_on(res == MY_UNLOCKED || lk->holder != current, "Error when unlock a spinlock.\n");
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

static void set_task(task_t* task) {
    if (cpu_list[cpu_current()].current_task != NULL) {
        panic("there are two taskes at the same time!");
    }
    task->status = T_RUNNING;
    task->next = NULL;
    cpu_list[cpu_current()].current_task = task;
}

static void sem_init(sem_t* sem, const char* name, int value) {
    spin_init(&sem->lk, name);
    sem->value = value;
    sem->name = name;
    sem->wait_list = NULL;
}
static void sem_wait(sem_t* sem) {
    bool sleep = false;
    spin_lock(&sem->lk);
    sem->value -= 1;
    if (sem->value < 0) {
        sleep = true;
        task_t* current_task = cpu_list[cpu_current()].current_task;
        current_task->status = T_SLEEPING;
        if (sem->wait_list == NULL) {
            sem->wait_list = current_task;
        } else {
            task_t* t = sem->wait_list;
            while (t->next != NULL) {
                t = t->next;
            }
            t->next = current_task;
        }
    } else {
        sleep = false;
    }
    spin_unlock(&sem->lk);
    if (sleep) {
        yield();
    }

}
static void sem_signal(sem_t* sem) {
    spin_lock(&sem->lk);
    sem->value += 1;
    if (sem->wait_list == NULL) {
        spin_unlock(&sem->lk);
        return;
    }
    task_t* new_task = sem->wait_list;
    sem->wait_list = new_task->next;
    spin_unlock(&sem->lk);
    new_task->status = T_BLOCKED;
    spin_lock(&task_lk);
    new_task->next = task_root.next;
    task_root.next = new_task;
    spin_unlock(&task_lk);
}

#endif