#include <common.h>

static void os_init() {
    pmm->init();
    kmt->init();
}

static void os_run() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    while (1) ;
}

static void os_on_irq (int seq, int event, handler_t handler) {
    //TODO
}
static Context* os_trap (Event, ev, Context *context) {
    //TODO
}

MODULE_DEF(os) = {
    .init   = os_init,
    .run    = os_run,
    .on_irq = os_on_irq,
    .trap   = os_trap,
};
