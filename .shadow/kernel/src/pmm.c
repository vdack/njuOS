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
#define HEADER_SIZE 16

header_t read_header(uintptr_t addr) {
    header_t header;
    lock_init(&header.mutex);
    header.next = 0;
    header.occupied = false;
    header.size = 0; 
    return header;
}
void write_header(uintptr_t addr, header_t header) {
    int x = 1;
    x--;
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

    printf("size of header: %d\nsize of mutex: %d\nsize of occupied: %d\nsize of size:%d\n size of next: %d\n",
    sizeof(header), sizeof(header.mutex), sizeof(header.occupied), sizeof(header.size), sizeof(header.next));
    printf("current header size: %d\n",header.size);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
