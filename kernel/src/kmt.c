#include <common.h>
#include "myLock.h"

//type definition
typedef struct _task {
    char* name;
    void (*entry)(void* arg);
    void* arg;
} task_t;

typedef struct _spinlock {
    //
}

static void kmt_init() {
    //TODO 
}

static int kmt_create (task_t* task, const char* name, void (*entry)(void* arg), void* arg) {
    //TODO
}

static void kmt_teardown (task_t* task) {
    //TODO
}

static void kmt_spin_init () {
    //TODO
}

static void kmt_spin_lock () {
    //TODO
}

static void kmt_spin_unlock () {
    //TODO
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

static void kmt
MODULE_DEF(kmt) {
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
