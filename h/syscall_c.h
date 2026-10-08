//
// C API sloja sistemskih poziva jezgra.
// Deklaracije funkcija koje korisnicki program poziva kao obicne C funkcije;
// svaka od njih je omotac oko odgovarajuceg ABI sistemskog poziva (ecall).
//

#ifndef _SYSCALL_C_H_
#define _SYSCALL_C_H_

#include "../lib/hw.h"

// "Rucke" - neprozirni pokazivaci na interne strukture jezgra
class _thread;
typedef _thread* thread_t;

class _sem;
typedef _sem* sem_t;

// Alokacija memorije
void* mem_alloc(size_t size);
int mem_free(void* ptr);

// Niti
int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg);
int thread_exit();
void thread_dispatch();

// Semafori
int sem_open(sem_t* handle, unsigned init);
int sem_close(sem_t handle);
int sem_wait(sem_t id);
int sem_signal(sem_t id);
int sem_wait_n(sem_t id, unsigned n);
int sem_signal_n(sem_t id, unsigned n);

// Uspavljivanje niti
int time_sleep(time_t sleepTime);

// Konzola
const int EOF = -1;
char getc();
void putc(char chr);

#endif // _SYSCALL_C_H_
