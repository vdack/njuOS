#include <common.h>
#include <limits.h>
#include "myCommon.h"

//type definition


Context* kmt_context_save(Event event, Context* context) {
    //TODO 
    // maybe a enqueue to capsule the task list is more fast.
    task_t* new_task = cpu_move_task();
    new_task->context = context;
    if (new_task->status == T_SLEEPING) {
        //
    } else {
        new_task->status = T_BLOCKED;
        task_t* before_task = &task_root;

        spin_lock(&task_lk);
        while (before_task->next != NULL) {
            before_task = before_task->next;
        }
        before_task->next = new_task;
        spin_unlock(&task_lk);
    }
    
    return NULL;
}

Context* kmt_schedule(Event event, Context* context) {
    task_t* new_task = NULL;

    spin_lock(&task_lk);
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
    spin_unlock(&task_lk);

    set_task(new_task);
    return new_task->context;
}

static void kmt_init() {
    //TODO 

    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
}

static int kmt_create (task_t* task, const char* name, void (*entry)(void* arg), void* arg) {
    //TODO
    task->stack = (uint8_t*)pmm->alloc(STACK_SIZE);
    Area area = RANGE(task->stack, (void*)task->stack + STACK_SIZE);
    task->context = kcontext(area, entry, arg);
    task->arg = arg;
    task->entry = entry;
    task->name = name;
    task->next = NULL;
    task->status = T_CREATE;

    spin_lock(&task_lk);
    task_t* before_task = &task_root;
    while (before_task->next != NULL) {
        before_task = before_task->next;
    }
    before_task->next = task;
    spin_unlock(&task_lk);
    return 0;
}

static void kmt_teardown (task_t* task) {
    //TODO
    pmm->free(task->stack);
    task->status = T_DEAD;
}


static void kmt_sem_init () {
    //TODO
}

static void kmt_sem_wait () {
    //TODO
}

static void kmt_sem_signal () {
    //TODO
}


MODULE_DEF(kmt) = {
    .init       =   kmt_init, 
    .create     =   kmt_create,
    .teardown   =   kmt_teardown,
    .spin_init  =   spin_init,
    .spin_lock  =   spin_lock,
    .spin_unlock=   spin_unlock,
    .sem_init   =   kmt_sem_init,
    .sem_wait   =   kmt_sem_wait,
    .sem_signal =   kmt_sem_signal,
};
