#ifndef _KMT_H_
#define _KMT_H_
#include <common.h>
#include <limits.h>

//helper function
static inline task_t* cpu_move_task() {
    task_t* temp = cpu_list[cpu_current()].current_task;
    cpu_list[cpu_current()].current_task = NULL;
    return temp;
} 
static inline void set_task(task_t* task) {
    if (cpu_list[cpu_current()].current_task != NULL) {
        panic("there are two taskes at the same time!");
    }
    task->status = T_RUNNING;
    task->next = NULL;
    cpu_list[cpu_current()].current_task = task;
}


static void kmt_spin_init(spinlock_t* lk, const char* name) {
    lk->flag = MY_UNLOCKED;
    lk->name = name;
    lk->holder = -1;
}
static void kmt_spin_lock(spinlock_t* lk) {
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
static void kmt_spin_unlock(spinlock_t* lk) {
    int current = cpu_current();
    
    int res = atomic_xchg(&lk->flag, MY_UNLOCKED);
    panic_on(res == MY_UNLOCKED || lk->holder != current, "Error when unlock a spinlock.\n");
    lk->holder = -1;

    cpu_list[current].lock_counter -= 1;
    if (cpu_list[current].lock_counter == 0) {
        iset(cpu_list[current].i_status);
    }

}


static void kmt_sem_init(sem_t* sem, const char* name, int value) {
    kmt_spin_init(&sem->lk, name);
    sem->value = value;
    sem->name = name;
    sem->wait_list = NULL;
}
static void kmt_sem_wait(sem_t* sem) {
    bool sleep = false;
    kmt_spin_lock(&sem->lk);
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
    kmt_spin_unlock(&sem->lk);
    if (sleep) {
        yield();
    }

}
static void kmt_sem_signal(sem_t* sem) {
    kmt_spin_lock(&sem->lk);
    sem->value += 1;
    if (sem->wait_list == NULL) {
        kmt_spin_unlock(&sem->lk);
        return;
    }
    task_t* new_task = sem->wait_list;
    sem->wait_list = new_task->next;
    kmt_spin_unlock(&sem->lk);
    new_task->status = T_BLOCKED;
    kmt_spin_lock(&task_lk);
    new_task->next = task_root.next;
    task_root.next = new_task;
    kmt_spin_unlock(&task_lk);
}


Context* kmt_context_save(Event event, Context* context) {
    //TODO 
    TRACE_ENTRY;
    // maybe a enqueue to capsule the task list is more fast.
    task_t* new_task = cpu_move_task();
    new_task->context = context;
    if (new_task->status == T_SLEEPING || new_task->status == T_DEAD) {
        //
    } else {
        new_task->status = T_BLOCKED;
        task_t* before_task = &task_root;

        kmt_spin_lock(&task_lk);
        while (before_task->next != NULL) {
            before_task = before_task->next;
        }
        before_task->next = new_task;
        kmt_spin_unlock(&task_lk);
    }
    TRACE_EXIT;
    return NULL;
}

Context* kmt_schedule(Event event, Context* context) {
    TRACE_ENTRY;
    task_t* new_task = NULL;

    kmt_spin_lock(&task_lk);
    new_task = task_root.next;
    if (new_task == NULL) {
        panic("no task!");
    }
    while(new_task->status == T_DEAD) {
        new_task = new_task->next;
        if (new_task == NULL) {
            panic("no undead task!");
        }
    }
    task_root.next = new_task->next;
    // while (new_task->status == T_SLEEPING) {
    //     new_task = new_task->next;
    //     if (new_task == NULL) {
    //         panic("no unsleep task!");
    //     }
    // }
    kmt_spin_unlock(&task_lk);

    set_task(new_task);
    TRACE_EXIT;
    return new_task->context;
}

static void kmt_init() {
    //TODO 

    os->on_irq(0, EVENT_NULL, kmt_context_save);
    os->on_irq(100, EVENT_NULL, kmt_schedule);
    
    
}

static int kmt_create (task_t* task, const char* name, void (*entry)(void* arg), void* arg) {
    //TODO
    TRACE_ENTRY;
    task->stack = (uint8_t*)pmm->alloc(STACK_SIZE);
    Area area = RANGE(task->stack, (void*)task->stack + STACK_SIZE);
    task->context = kcontext(area, entry, arg);
    task->arg = arg;
    task->entry = entry;
    task->name = name;
    task->next = NULL;
    task->status = T_CREATE;

    kmt_spin_lock(&task_lk);
    task_t* before_task = &task_root;
    while (before_task->next != NULL) {
        before_task = before_task->next;
    }
    before_task->next = task;
    kmt_spin_unlock(&task_lk);
    TRACE_EXIT;
    return 0;
}

static void kmt_teardown (task_t* task) {
    //TODO
    TRACE_ENTRY;
    pmm->free(task->stack);
    task->status = T_DEAD;
    TRACE_EXIT;
}

MODULE_DEF(kmt) = {
    .init       =   kmt_init, 
    .create     =   kmt_create,
    .teardown   =   kmt_teardown,
    .spin_init  =   kmt_spin_init,
    .spin_lock  =   kmt_spin_lock,
    .spin_unlock=   kmt_spin_unlock,
    .sem_init   =   kmt_sem_init,
    .sem_wait   =   kmt_sem_wait,
    .sem_signal =   kmt_sem_signal,
};

#endif