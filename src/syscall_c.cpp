//
// Implementacija C API sloja sistemskih poziva.
// Svaka funkcija upisuje kod poziva u registar a0, argumente u a1..a4,
// izvrsava ecall i preuzima povratnu vrednost iz a0 (ABI konvencija).
//

#include "../h/syscall_c.h"
#include "../h/riscv.hpp"
#include "../h/memoryAllocator.hpp"

static inline uint64 syscall(uint64 code, uint64 arg1 = 0, uint64 arg2 = 0,
                             uint64 arg3 = 0, uint64 arg4 = 0) {
    register uint64 a0 __asm__("a0") = code;
    register uint64 a1 __asm__("a1") = arg1;
    register uint64 a2 __asm__("a2") = arg2;
    register uint64 a3 __asm__("a3") = arg3;
    register uint64 a4 __asm__("a4") = arg4;
    __asm__ volatile ("ecall"
                      : "+r"(a0)
                      : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                      : "memory");
    return a0;
}

void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    // ABI poziv prima velicinu izrazenu u blokovima
    size_t blocks = MemoryAllocator::bytesToBlocks(size);
    return (void*) syscall(Riscv::SYS_MEM_ALLOC, blocks);
}

int mem_free(void* ptr) {
    return (int) syscall(Riscv::SYS_MEM_FREE, (uint64) ptr);
}

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg) {
    if (!handle || !start_routine) return -1;

    // stek za nit obezbedjuje ovaj sloj, alokacijom preko mem_alloc
    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (!stack) return -2;
    void* stackTop = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);

    int status = (int) syscall(Riscv::SYS_THREAD_CREATE, (uint64) handle,
                               (uint64) start_routine, (uint64) arg,
                               (uint64) stackTop);
    if (status != 0) mem_free(stack);
    return status;
}

int thread_exit() {
    return (int) syscall(Riscv::SYS_THREAD_EXIT);
}

void thread_dispatch() {
    syscall(Riscv::SYS_THREAD_DISPATCH);
}

int sem_open(sem_t* handle, unsigned init) {
    if (!handle) return -1;
    return (int) syscall(Riscv::SYS_SEM_OPEN, (uint64) handle, (uint64) init);
}

int sem_close(sem_t handle) {
    return (int) syscall(Riscv::SYS_SEM_CLOSE, (uint64) handle);
}

int sem_wait(sem_t id) {
    return (int) syscall(Riscv::SYS_SEM_WAIT, (uint64) id);
}

int sem_signal(sem_t id) {
    return (int) syscall(Riscv::SYS_SEM_SIGNAL, (uint64) id);
}

int sem_wait_n(sem_t id, unsigned n) {
    return (int) syscall(Riscv::SYS_SEM_WAIT_N, (uint64) id, (uint64) n);
}

int sem_signal_n(sem_t id, unsigned n) {
    return (int) syscall(Riscv::SYS_SEM_SIGNAL_N, (uint64) id, (uint64) n);
}

int time_sleep(time_t sleepTime) {
    return (int) syscall(Riscv::SYS_TIME_SLEEP, (uint64) sleepTime);
}

char getc() {
    return (char) syscall(Riscv::SYS_GETC);
}

void putc(char chr) {
    syscall(Riscv::SYS_PUTC, (uint64) chr);
}
