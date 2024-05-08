#include <common.h>

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
    panic_on(rc == MY_LOCKED, "release before acquire!!!");
}
bool try_lock_acquire(lock_t* lock) {
    int rc = atomic_xchg(&lock->flag, MY_LOCKED);
    return rc == MY_UNLOCKED;
}
#endif

// tools funtion and macro 
inline static int get_max_in(int max) {
    int t = 1;
    while(t < max) {
        t = t << 1;
    }
    t = t >> 1;
    return t;
}
#define MB_TO_BYTES(x) (x << 20)
#define BYTES_TO_MB(x) (x >> 20)



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
inline static header_t construct_header(size_t size, void* next) {
    header_t header;
    header.occupied = false;
    header.size = size;
    header.next = next;
    lock_init(&header.mutex);
    return header;
}



// // list def
// typedef struct _list_header {
//     lock_t mutex;
//     size_t size;
//     uintptr_t start_addr;
// } list_header_t;

// static list_header_t buddy_list;

#define BUDDY_SIZE (32 << 20)
#define SMALL_SIZE 256
static header_t* first_buddy_addr;
static header_t* first_small_addr;
static int buddy_sum;
static int small_sum;


// helper function 

// static inline intptr_t buddy_search(size_t size, intptr_t node_h) {
//     header_t* header = read_header((void*) node_h);
//     lock_acquire(&header->mutex);

//     lock_release(&header->mutex);
// }
// static void *buddy_alloc(size_t size) {
//     header_t* header = read_header((void*) buddy_list.start_addr);
//     while(1) {
//         header* next_header = NULL;
//         lock_acquire(&header->mutex);
//         if (header->size < size) {
//             next_header = (header*)header->next;
//         }
//         lock_release(&header->mutex);
//     }
// }



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
    //67108864 

    //init the buddy segement.
    
    buddy_sum = 1;
    uintptr_t left_size = pmsize - BUDDY_SIZE;
    header_t last_buddy_header = construct_header(BUDDY_SIZE - HEADER_SIZE, NONE_NEXT);
    void* last_addr = heap.end - BUDDY_SIZE - HEADER_SIZE;
    write_header(last_addr, last_buddy_header);

    while (left_size >= (2 * BUDDY_SIZE)) {
        buddy_sum += 1;
        header_t buddy_header = construct_header(BUDDY_SIZE - HEADER_SIZE, last_addr);
        last_addr = last_addr - BUDDY_SIZE;
        write_header(last_addr, buddy_header);
        left_size -= BUDDY_SIZE;
    }
    first_buddy_addr = last_addr;        
    
    printf("current buddy num: %d and left size: %d\n", buddy_sum, left_size);
    
    // init small 
    
    small_sum = 0;
    while (left_size > SMALL_SIZE) {
        small_sum += 1;
        header_t small_header = construct_header(SMALL_SIZE - HEADER_SIZE, last_addr);
        last_addr = last_addr - SMALL_SIZE;
        write_header(last_addr, small_header);
        left_size -= SMALL_SIZE;
    }
    header_t small_end_header = construct_header(SMALL_SIZE - HEADER_SIZE, last_addr);
    write_header((first_buddy_addr - SMALL_SIZE), small_end_header);
    first_small_addr = last_addr;

    
    
    printf("buddy first address: %p\n", (void*)first_buddy_addr + HEADER_SIZE);
    printf("next buddy pstr: %p\n", (void*)first_buddy_addr->next + HEADER_SIZE);   
    
    printf("small sum: %d, and left size: %d\n", small_sum, left_size);
    printf("first small pstr: %p\nnext small pstr: %p\n",(void*)first_small_addr + HEADER_SIZE, (void*)first_small_addr->next + HEADER_SIZE);


}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
