#ifndef _KMT_H_
#define _KMT_H_
#include <common.h>
#include <limits.h>

//helper function
static inline task_t* cpu_move_task() {
    task_t* temp = cpu_list[cpu_current()].current_task;
    // panic_on(temp == NULL, "current task none!\n");
    cpu_list[cpu_current()].current_task = NULL;
    return temp;
} 
static inline void set_task(task_t* task) {
    if (cpu_list[cpu_current()].current_task != NULL) {
        panic("there are two taskes at the same time!");
    }
    task->status = T_RUNNING;
    cpu_list[cpu_current()].current_task = task;
}

void waitlist_init(waitlist_t* wl, const char* name) {
    wl->head = NULL;
    wl->tail = NULL;
    kmt->spin_init(&wl->lk, name);
}

void waitlist_add(waitlist_t* wl,task_t* t) {
    kmt->spin_lock(&wl->lk);
    if (wl->head == NULL) {
        wl->head = t;
        kmt->spin_unlock(&wl->lk);
        return;
    }
    if (wl->tail == NULL) {
        wl->head->next = t;
        wl->tail = t;
        kmt->spin_unlock(&wl->lk);
        return;
    }
    wl->tail->next = t;
    wl->tail = t;
    kmt->spin_unlock(&wl->lk);
}

task_t* waitlist_get(waitlist_t* wl) {
    kmt->spin_lock(&wl->lk);
    if (wl->head == NULL) {
        kmt->spin_unlock(&wl->lk);
        return NULL;
    }
    task_t* t = wl->head;
    if (wl->tail == NULL) {
        wl->head = NULL;
        kmt->spin_unlock(&wl->lk);
        t->next = NULL;
        return t;
    }
    wl->head = t->next;
    if (wl->head == wl->tail) {
        wl->tail = NULL;
    }
    kmt->spin_unlock(&wl->lk);
    t->next = NULL;
    return t;
}



static void kmt_spin_init(spinlock_t* lk, const char* name) {
    lk->flag = MY_UNLOCKED;
    lk->name = name;
}
static void kmt_spin_lock(spinlock_t* lk) {
    bool istatus = ienabled();
    iset(false);
    while(atomic_xchg(&lk->flag, MY_LOCKED) == MY_LOCKED){
        //
    }

    int current = cpu_current();

    cpu_list[current].lock_counter += 1;
    if (cpu_list[current].lock_counter == 1) {
        // cpu acquire a lock for the first time.
        cpu_list[current].i_status = istatus;
    }
    return;
}
static void kmt_spin_unlock(spinlock_t* lk) {
    int current = cpu_current();
    
    int res = atomic_xchg(&lk->flag, MY_UNLOCKED);
    panic_on(res == MY_UNLOCKED, "Error when unlock an unlocked spinlock.\n");

    cpu_list[current].lock_counter -= 1;
    if (cpu_list[current].lock_counter == 0) {
        iset(cpu_list[current].i_status);
    }

}


static void kmt_sem_init(sem_t* sem, const char* name, int value) {
    kmt_spin_init(&sem->lk, name);
    sem->value = value;
    sem->name = name;
    waitlist_init(&sem->sleep_list, name);
}
static void kmt_sem_wait(sem_t* sem) {
    bool sleep = false;
    kmt_spin_lock(&sem->lk);
    sem->value -= 1;
    if (sem->value < 0) {
        sleep = true;
        task_t* current_task = cpu_list[cpu_current()].current_task;
        current_task->status = T_SLEEPING;
        waitlist_add(&sem->sleep_list, current_task);
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
    
    task_t* awake_task = waitlist_get(&sem->sleep_list);
    if (awake_task == NULL) {
        kmt_spin_unlock(&sem->lk);
        return;
    }
    kmt_spin_unlock(&sem->lk);
    awake_task->status = T_BLOCKED;
    waitlist_add(&task_list, awake_task);
    
    
}


Context* kmt_context_save(Event event, Context* context) {
    //TODO 
    TRACE_ENTRY;
    // maybe a enqueue to capsule the task list is more fast.
    task_t* new_task = cpu_move_task();
    if (new_task == NULL) {
        return NULL;
    }
    new_task->context = context;
    if (new_task->status == T_SLEEPING || new_task->status == T_DEAD) {
        DEBUG("skip current task!");
    } else {
        new_task->status = T_BLOCKED;
        waitlist_add(&task_list, new_task);
    }
    TRACE_EXIT;
    return NULL;
}

Context* kmt_schedule(Event event, Context* context) {
    TRACE_ENTRY;
    task_t* new_task = NULL;
    while (new_task == NULL) {
        new_task = waitlist_get(&task_list);
    }
    // kmt_spin_lock(&task_lk);
    // new_task = task_root.next;
    
    // while (new_task == NULL) {
    //     // panic("no task!");
    //     kmt_spin_unlock(&task_lk);
    //     kmt_spin_lock(&task_lk);
    //     new_task = task_root.next;
    // }
    // while(new_task->status == T_DEAD) {
    //     new_task = new_task->next;
    //     if (new_task == NULL) {
    //         task_root.next = NULL;
    //         while(new_task == NULL) {
    //             kmt_spin_unlock(&task_lk);
    //             kmt_spin_lock(&task_lk);
    //             new_task = task_root.next;
    //             // panic("no task!");
    //         }
            
    //     }
    // }
    // task_root.next = new_task->next;
    // // while (new_task->status == T_SLEEPING) {
    // //     new_task = new_task->next;
    // //     if (new_task == NULL) {
    // //         panic("no unsleep task!");
    // //     }
    // // }
    // kmt_spin_unlock(&task_lk);

    set_task(new_task);
    TRACE_EXIT;
    return new_task->context;
}

static void kmt_init() {
    //TODO 
    waitlist_init(&task_list, "task_list");

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

    // kmt_spin_lock(&task_lk);
    // task_t* before_task = &task_root;
    // while (before_task->next != NULL) {
    //     before_task = before_task->next;
    // }
    // before_task->next = task;
    // kmt_spin_unlock(&task_lk);
    waitlist_add(&task_list, task);
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