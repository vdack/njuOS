#ifndef _OS_H_
#define _OS_H_
#include <common.h>
cpu_t cpu_list[CPU_MAX];
task_t task_root;
spinlock_t task_lk;
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
    task_root.next = NULL;

    kmt->spin_init(&task_lk, "task");

    pmm->init();
    kmt->init();

}

static void os_run() {
    
    
    while (1) {
        for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
            putch(*s == '*' ? '0' + cpu_current() : *s);
        }
        yield();
    } ;
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
        if (new_irq->seq >= next_irq->seq) {
            break;
        }
        before_irq = next_irq;
    }
    new_irq->next = before_irq->next;
    before_irq->next = new_irq;
    kmt->spin_unlock(&lk_irq);

    DEBUG("enroot next: %p \n",enroot.next);
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

    /*
    for (auto &h: handlers_sorted_by_seq) {
        if (h.event == EVENT_NULL || h.event == ev.event) {
            Context *r = h.handler(ev, ctx);
            panic_on(r && next, "return to multiple contexts");
            if (r) next = r;
        }
    }
    */
    panic_on(!next, "return to NULL context");
    // panic_on(sane_context(next), "return to invalid context");
    return next;
}

MODULE_DEF(os) = {
    .init   = os_init,
    .run    = os_run,
    .on_irq = os_on_irq,
    .trap   = os_trap,
};

#endif