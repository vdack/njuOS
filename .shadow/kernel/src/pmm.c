#include <common.h>

#define BUDDY_RANDOM

// lock part.
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
    panic_on(rc == MY_UNLOCKED, "failed! release before acquire!!!\n");
}
bool try_lock_acquire(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_LOCKED);
    return rc == MY_UNLOCKED;
}
#endif

// tools funtion and macro 
#define MB_TO_BYTES(x) (x << 20)
#define BYTES_TO_MB(x) (x >> 20)
#define KB_TO_BYTES(x) (x << 10)
#define RAND_SEED 1231238888


// header def
typedef struct _header {
    lock_t mutex;
    bool occupied;
    size_t size;
    struct _header* next;
    
} header_t;
#define HEADER_SIZE sizeof(header_t)
#define NONE_NEXT 0

inline static header_t* read_header(void* addr) {
    header_t* header = ((header_t*)addr); 
    return header;
}
inline static void write_header(void* addr, header_t header) {
    *((header_t*)addr) = header;
}
inline static header_t construct_header(size_t size, header_t* next) {
    header_t header;
    header.occupied = false;
    header.size = size;
    header.next = next;
    lock_init(&header.mutex);
    return header;
}

#define BUDDY_SIZE (32 << 20)
#define SMALL_SIZE 256
static header_t* first_buddy_addr;
static header_t* first_small_addr;
static int buddy_sum;
static int small_sum;


// helper function 
static inline void *buddy_alloc(size_t size) {

#ifdef BUDDY_RANDOM
    int offset = rand() % buddy_sum;
    header_t* header = (header_t*)((uintptr_t)first_buddy_addr + offset * BUDDY_SIZE);
#else
    header_t* header = first_buddy_addr;
#endif
    
    while(1) {
        header_t* next_header = NULL;
        
        lock_acquire(&header->mutex);
        // is occupied or too small
        if ((header->occupied)) {
            next_header = header->next;
        } else if ((header->size < size)) {
            next_header = header->next;
        } else {
            int divide_size = (header->size - HEADER_SIZE) / 2;
            if (divide_size < size) {
                // find the suitable buddy.
                header->occupied = true;
                lock_release(&header->mutex);
                return (void*)((uintptr_t)header + HEADER_SIZE);
            }
            // divide current buddy to small ones.
            header_t* new_header_addr = (header_t*)((intptr_t)header + HEADER_SIZE + divide_size);
            header_t new_header = construct_header(divide_size, header->next);
            
            // write_header(new_header_addr, new_header);
            header->next = (header_t*)new_header_addr;
            *(new_header_addr) = new_header;
            header->size = divide_size;
            next_header = header;            
        }        
        lock_release(&header->mutex);
        header = next_header;
        // no other buddy space
        if (header == NULL) {
            return NULL;
        }
    }
}

static inline void buddy_free(header_t* header) {
    lock_acquire(&header->mutex);
    header->occupied = false;
    while(header->size +HEADER_SIZE < KB_TO_BYTES(8)) {
        header_t* buddy_addr = (header_t*)((((uintptr_t)header + HEADER_SIZE) ^ ((uintptr_t)(header->size) + HEADER_SIZE)) - HEADER_SIZE); 
        bool rc = try_lock_acquire(&buddy_addr->mutex);
        if (!rc) {
            break;
        }
        if ( (buddy_addr->size == header->size) && (!buddy_addr->occupied) ) {
            //find a buddy to merge.
            if ((uintptr_t)header > (uintptr_t)buddy_addr) {
                header_t* temp = header;
                header = buddy_addr;
                buddy_addr = temp;
            } 
            header->size = header->size + HEADER_SIZE + header->size;
            header->next = (header_t*)((uintptr_t)header + HEADER_SIZE + header->size);
            lock_release(&buddy_addr->mutex);
        } else {
            lock_release(&buddy_addr->mutex);
            break;
        }
    }

    lock_release(&header->mutex);
}

// small space
static inline void* small_alloc() {
    int offset = rand() % small_sum;
    void* target_addr = (void*)((uintptr_t)first_small_addr + SMALL_SIZE * offset);
    while(1) {
        bool rc = try_lock_acquire(&((header_t*)target_addr)->mutex);
        if (rc) {
            if (((header_t*)target_addr)->occupied) {
                lock_release(&((header_t*)target_addr)->mutex);
                target_addr = (void*)(((header_t*)target_addr)->next);
            } else {
                ((header_t*)target_addr)->occupied = true;
                lock_release(&((header_t*)target_addr)->mutex);
                return (void*)((uintptr_t)target_addr + HEADER_SIZE);
            }
        } else {
            target_addr = (void*)(((header_t*)target_addr)->next);
        }
    }
} 


static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    if (size <= 129) {
        return small_alloc();
    }
    return buddy_alloc(size);
    
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
    if ((intptr_t)ptr < (intptr_t)first_buddy_addr) {
        header_t* h_addr = (header_t*)((intptr_t)ptr - HEADER_SIZE);
        lock_acquire(&h_addr->mutex);
        h_addr->occupied = false;
        lock_release(&h_addr->mutex);

    } else {
        // free buddy. 
        header_t* h_addr = (header_t*)((intptr_t)ptr - HEADER_SIZE);
        lock_acquire(&h_addr->mutex);
        h_addr->occupied = false;
        lock_release(&h_addr->mutex);
        // buddy_free(h_addr);
    }

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
    
    // other init
    srand(RAND_SEED);
    uintptr_t virtual_end = (uintptr_t)heap.end & (~((uintptr_t)BUDDY_SIZE - 1));
    //init the buddy segement.
    
    buddy_sum = 1;
    uintptr_t left_size = virtual_end - (uintptr_t)heap.start - BUDDY_SIZE;
    header_t last_buddy_header = construct_header(BUDDY_SIZE - HEADER_SIZE, NULL);
    void* last_addr = (void*)(virtual_end - BUDDY_SIZE - HEADER_SIZE);
    write_header(last_addr, last_buddy_header);

    while (left_size >= (2 * BUDDY_SIZE)) {
        buddy_sum += 1;
        header_t buddy_header = construct_header(BUDDY_SIZE - HEADER_SIZE, last_addr);
        last_addr = (void*)((uintptr_t)last_addr - BUDDY_SIZE);
        write_header(last_addr, buddy_header);
        left_size -= BUDDY_SIZE;
    }
    first_buddy_addr = (header_t*)last_addr;        
    // init small 
    
    small_sum = 0;
    while (left_size > SMALL_SIZE) {
        small_sum += 1;
        header_t small_header = construct_header(SMALL_SIZE - HEADER_SIZE, last_addr);
        last_addr = (void*)((uintptr_t)last_addr - SMALL_SIZE);
        write_header(last_addr, small_header);
        left_size -= SMALL_SIZE;
    }
    // printf("after small \n");
    header_t small_end_header = construct_header(SMALL_SIZE - HEADER_SIZE, last_addr);
    write_header((first_buddy_addr - SMALL_SIZE), small_end_header);
    first_small_addr = (header_t*)last_addr;

    // void* p1 = kalloc(KB_TO_BYTES(2));
    // printf("p1: %p\n", p1);
    // kfree(p1);

}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
