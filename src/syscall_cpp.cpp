//
// Implementacija C++ API sloja sistemskih poziva:
// globalni operatori new/delete i klase Thread, Semaphore,
// PeriodicThread i Console kao omotaci oko C API-ja.
//

#include "../h/syscall_cpp.hpp"

void* operator new(size_t size) {
    return mem_alloc(size);
}

void* operator new[](size_t size) {
    return mem_alloc(size);
}

void operator delete(void* ptr) noexcept {
    mem_free(ptr);
}

void operator delete[](void* ptr) noexcept {
    mem_free(ptr);
}

// ---------------------------------------------------------------- Thread

Thread::Thread(void (*body)(void*), void* arg)
        : myHandle(nullptr), body(body), arg(arg) {
}

Thread::Thread() : myHandle(nullptr), body(nullptr), arg(nullptr) {
}

Thread::~Thread() {
}

int Thread::start() {
    // ukoliko je telo zadato konstruktorom, nit izvrsava njega;
    // u suprotnom se izvrsava virtuelna metoda run() ovog objekta
    if (body) return thread_create(&myHandle, body, arg);
    return thread_create(&myHandle, &runWrapper, this);
}

void Thread::runWrapper(void* thread) {
    ((Thread*) thread)->run();
}

void Thread::dispatch() {
    thread_dispatch();
}

int Thread::sleep(time_t time) {
    return time_sleep(time);
}

// ------------------------------------------------------------- Semaphore

Semaphore::Semaphore(unsigned init) : myHandle(nullptr) {
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore() {
    sem_close(myHandle);
}

int Semaphore::wait() {
    return sem_wait(myHandle);
}

int Semaphore::signal() {
    return sem_signal(myHandle);
}

// -------------------------------------------------------- PeriodicThread

PeriodicThread::PeriodicThread(time_t period) : Thread(), period(period) {
}

void PeriodicThread::terminate() {
    period = 0;
}

void PeriodicThread::run() {
    while (period > 0) {
        periodicActivation();
        if (period == 0) break;
        sleep(period);
    }
}

// --------------------------------------------------------------- Console

char Console::getc() {
    return ::getc();
}

void Console::putc(char chr) {
    ::putc(chr);
}
