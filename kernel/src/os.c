#ifndef _OS_H_
#define _OS_H_
#include <common.h>

//local test:
#ifdef TRACE_F
void print_current() {
    while(1){
        printf("Hello From Cpu#%d\n", cpu_current());
        yield();
    }
}
void tea() {
    while (1) {
        printf("TEA\n");
        yield();
    }
}

void print_test() {
    for(int i = 0; i < cpu_count(); i += 1) {
        task_t* t = (task_t*) pmm->alloc(sizeof(task_t));
        kmt->create(t, "HELLO", print_current, NULL);
    }
}

void print_tea() {
    task_t* t = (task_t*) pmm->alloc(sizeof(task_t));
    kmt->create(t, "TEA", tea, NULL);
}

#endif


cpu_t cpu_list[CPU_MAX];
waitlist_t task_list;
typedef struct _enroll {
    int event;
    int seq;
    handler_t handler;
    struct _enroll* next;
} enroll_t;

enroll_t enroot; 
spinlock_t lk_irq;

static void os_init() {

    enroot.next = NULL;
    kmt->spin_init(&lk_irq,"irq");
    for (int i = 0; i < CPU_MAX; i += 1) {
        cpu_list[i].current_task = NULL;
        cpu_list[i].lock_counter = 0;
        cpu_list[i].i_status = true;
    }
    
    pmm->init();
    kmt->init();
}

static void os_run() {
    TRACE_ENTRY;

    // task_t* os_task = (task_t*)pmm->alloc(sizeof(task_t));
    // os_task->next = NULL;
    // os_task->status = T_RUNNING;
    // os_task->name = "os_run";
    // cpu_list[cpu_current()].current_task = os_task;

    
#ifdef TRACE_F
    // DEBUG("origin status: %d\n", ienabled());
    print_test();
    print_tea();
#endif
    iset(true);
    while (1) {
        // yield();
    } ;
    TRACE_EXIT;
}

static void os_on_irq (int seq, int event, handler_t handler) {
    enroll_t* new_irq = pmm->alloc(sizeof(enroll_t));
    new_irq->seq = seq;
    new_irq->event = event;
    new_irq->handler = handler;
    new_irq->next = NULL;
    
    enroll_t* before_irq = &enroot;
    kmt->spin_lock(&lk_irq);
    while (before_irq->next != NULL) {
        enroll_t* next_irq = before_irq->next;
        if (new_irq->seq <= next_irq->seq) {
            break;
        }
        before_irq = next_irq;
    }
    new_irq->next = before_irq->next;
    before_irq->next = new_irq;
    kmt->spin_unlock(&lk_irq);
}
static Context* os_trap (Event ev, Context *ctx) {
    Context *next = NULL;
    kmt->spin_lock(&lk_irq);
    enroll_t* h = enroot.next;
    while (h != NULL) {
        if (h->event == EVENT_NULL || h->event == ev.event) {
            Context* r = h->handler(ev, ctx);
            panic_on(r && next, "return multiple contexts");
            if (r != NULL) next = r;
        }
        h = h->next;
    }
    kmt->spin_unlock(&lk_irq);
    panic_on(!next, "return to NULL context");
    return next;
}

MODULE_DEF(os) = {
    .init   = os_init,
    .run    = os_run,
    .on_irq = os_on_irq,
    .trap   = os_trap,
};

#endif