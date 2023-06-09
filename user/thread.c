#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "thread.h"
#include "user.h"

#define PGSIZE 4096

int 
thread_create(void *(start_routine)(void*), void *arg)
{
     // Allocate user stack
    void *stack = (void*)malloc(PGSIZE);
    if (!stack) {
        //printf("Failed to allocate user stack");
        return -1;
    }

    // Create child thread
    int tid = clone(stack);
    if (tid < 0 || tid > 20) {
        //printf("Failed to create child thread");
        free(stack);
        return -1;
    }
    else if (tid == 0) {
        (*start_routine)(arg);
        exit(0);
    }

    return 0;
}

void 
lock_init(struct lock_t* lock)
{
    lock->locked = 0;
}

void 
lock_acquire(struct lock_t* lock)
{
    // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
    //   a5 = 1
    //   s1 = &lock->locked
    //   amoswap.w.aq a5, a5, (s1)
    while(__sync_lock_test_and_set(&lock->locked, 1) != 0)
    ;

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();
}
void 
lock_release(struct lock_t* lock)
{
    // Tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the critical section occur strictly before
    // the lock is released.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = &lk->locked
    //   amoswap.w zero, zero, (s1)
    __sync_lock_release(&lock->locked);
}