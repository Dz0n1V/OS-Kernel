//
// C++ API sloja sistemskih poziva jezgra.
// Objektno orijentisani omotac oko C API-ja; raspored i skup clanova klasa
// mora ostati identican onome iz postavke zadatka (kompatibilnost sa app.lib).
//

#ifndef _syscall_cpp
#define _syscall_cpp

#include "syscall_c.h"

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

class Thread {
public:
    Thread(void (*body)(void*), void* arg);
    virtual ~Thread();

    int start();

    static void dispatch();
    static int sleep(time_t time);

protected:
    Thread();
    virtual void run() {}

private:
    static void runWrapper(void* thread);

    thread_t myHandle;
    void (*body)(void*);
    void* arg;
};

class Semaphore {
public:
    Semaphore(unsigned init = 1);
    virtual ~Semaphore();

    int wait();
    int signal();

private:
    sem_t myHandle;
};

class PeriodicThread : public Thread {
public:
    void terminate();

protected:
    PeriodicThread(time_t period);
    virtual void periodicActivation() {}

private:
    void run() override;

    time_t period;
};

class Console {
public:
    static char getc();
    static void putc(char chr);
};

#endif // _syscall_cpp
