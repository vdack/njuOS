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
    panic_on(rc == MY_UNLOCKED, "release before acquire!!!");
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
#define KB_TO_BYTES(x) (x << 10)



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
static inline void *buddy_alloc(size_t size) {
    header_t* header = first_buddy_addr;
    while(1) {
        header_t* next_header = NULL;
        
        // printf("current header %p, size: %d, occupied: %d next: %p\n", header, header->size, header->occupied, header->next);

        // printf("%p try acquire lock.\n", header);
        lock_acquire(&header->mutex);
        // printf("%p acquired lock.\n", header);
        // is occupied or too small
        if ((header->occupied)) {
            // printf("%p is occupied.\n", header);
            next_header = header->next;
            // printf("next header %p, size: %d, occupied: %d next: %p\n", header->next, header->next->size, header->next->occupied, header->next->next);
        } else if ((header->size < size)) {
            // printf("%p is too small.\n", header);
            next_header = header->next;
            // printf("next header %p, size: %d, occupied: %d next: %p\n", header->next, header->next->size, header->next->occupied, header->next->next);

        } else {
            int divide_size = (header->size - HEADER_SIZE) / 2;
            if (divide_size < size) {
                // find the suitable buddy.
                printf("Find the suitable buddy!\n");
                header->occupied = true;
                // printf("%p try release lock.\n", header);
                lock_release(&header->mutex);
                // printf("%p released lock.\n", header);
                return header + HEADER_SIZE;
            }

            // divide current buddy to small ones.
            header_t* new_header_addr = (header_t*)((intptr_t)header + HEADER_SIZE + divide_size);
            header_t new_header = construct_header(divide_size, header->next);
            
            // write_header(new_header_addr, new_header);
            header->next = (header_t*)new_header_addr;
            *(new_header_addr) = new_header;
            // *(header->next) = new_header;
            // printf("new header %p, size: %d, occupied: %d next: %p\n", new_header_addr, new_header_addr->size, new_header_addr->occupied, new_header_addr->next);
            // printf("next header %p, size: %d, occupied: %d next: %p\n", header->next, header->next->size, header->next->occupied, header->next->next);
            header->size = divide_size;
            next_header = header;
            
        }
        // printf("continue search...\n");
        
        // printf("%p try release lock.\n", header);
        lock_release(&header->mutex);
        // printf("%p released lock.\n", header);

        header = next_header;
        
        // no other buddy space
        if (header == NULL) {
            return NULL;
        }
    }
}



static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    return buddy_alloc(size);
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
    
    // other init
    srand(pmsize >> 4);
    
    //67108864 

    //init the buddy segement.
    
    buddy_sum = 1;
    uintptr_t left_size = pmsize - BUDDY_SIZE;
    header_t last_buddy_header = construct_header(BUDDY_SIZE - HEADER_SIZE, NULL);
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
    printf("first buddy size: %d\n", first_buddy_addr->size);

    printf("small sum: %d, and left size: %d\n", small_sum, left_size);
    printf("first small pstr: %p\nnext small pstr: %p\n",(void*)first_small_addr + HEADER_SIZE, (void*)first_small_addr->next + HEADER_SIZE);
    printf("a random small pstr: %p \n", (void*)first_small_addr + (rand() % small_sum) * SMALL_SIZE + HEADER_SIZE);

    printf("i want a 8 MB space, and get %p \n", kalloc(MB_TO_BYTES(8)));
    printf("i want a 16 kb space, and get %p \n", kalloc(KB_TO_BYTES(16)));
    void* p1 = kalloc(17);
    printf("i want a 17 bytes space, and get %p, mod 32: %d", p1, (int)p1 % 32);
    void* p2 = kalloc(8);
    printf("i want a 8 bytes space, and get %p, mod 8: %d", p2, (int)p2 % 8);
    void* p3 = kalloc(57);
    printf("i want a 57 bytes space, and get %p, mod 64: %d", p3, (int)p3 % 64);
    void* p4 = kalloc(1000);
    printf("i want a 1000 bytes space, and get %p, mod 1024: %d", p4, (int)p4 % 1024);
    printf("\n");

}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
