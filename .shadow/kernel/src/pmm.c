#include <common.h>


#ifndef _MY_LOCK
#define _MY_LOCK
#define MY_LOCKED 1
#define MY_UNLOCKED 0
typedef struct _lock_type {
    int flag;
} lock_t;
void lock_init(lock_t* lock) {
    lock->flag = 0;
} 
void lock_acquire(lock_t* lock) {
    while(atomic_xchg(&lock->flag, MY_LOCKED) == MY_LOCKED);
}
void lock_release(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_UNLOCKED);
    panic_on(rc == MY_LOCKED, "release before acquire!!!");
}
bool try_lock_acquire(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_LOCKED);
    return rc == MY_UNLOCKED;
}
#endif

typedef struct _header {
    lock_t mutex;
    size_t size;
    bool occupied;
    uintptr_t next;
    
} header_t;
#define HEADER_SIZE sizeof(header_t)

typedef struct _list_header {
    lock_t mutex;
    size_t size;
    uintptr_t start_addr;
    uintptr_t end_addr;
} list_header_t;

list_header_t buddy_list;

header_t read_header(void* addr) {
    header_t header = *((header_t*)addr); 
    return header;
}
void write_header(void* addr, header_t header) {
    *((header_t*)addr) = header;
}

static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.

    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
    header_t header;
    lock_init(&header.mutex);
    header.next = 0;
    header.occupied = false;
    header.size = (int)pmsize - HEADER_SIZE; 
    printf("heap start: %d and end: %d \n", heap.start, heap.end);
    write_header(heap.start, header);

}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
