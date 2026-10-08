//
// Implementacija apstrakcije niti (TCB).
//

#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/memoryAllocator.hpp"
#include "../h/riscv.hpp"
#include "../h/syscall_c.h"

TCB* TCB::running = nullptr;
TCB* TCB::idleThread = nullptr;
TCB* TCB::finishedHead = nullptr;
uint64 TCB::timeSliceCounter = 0;

void* TCB::operator new(size_t size) {
    return MemoryAllocator::kmalloc(size);
}

void TCB::operator delete(void* ptr) {
    MemoryAllocator::kfree(ptr);
}

TCB::TCB(Body body, void* arg, void* stackTop, bool privileged, bool detached)
        : body(body), arg(arg), ownedStack(nullptr),
          state(CREATED), privileged(privileged), detached(detached),
          timeSlice(DEFAULT_TIME_SLICE),
          nextReady(nullptr), nextBlocked(nullptr),
          nextSleeping(nullptr), nextFinished(nullptr),
          sleepRelative(0), semNeed(0), semReturnValue(0) {
    if (stackTop) {
        // stek raste ka nizim adresama; sp mora biti deljiv sa 16
        uint64 sp = ((uint64) stackTop) & ~0xFUL;
        context.sp = sp;
        context.ra = (uint64) &threadWrapper;
        // stek je alociran alokatorom jezgra sa podrazumevanom velicinom
        ownedStack = (void*) ((uint64) stackTop - DEFAULT_STACK_SIZE);
    } else {
        // zateceni (glavni) tok kontrole: kontekst ce biti upisan
        // prilikom prve promene konteksta
        context.sp = 0;
        context.ra = 0;
    }
    for (int i = 0; i < 12; i++) context.s[i] = 0;
}

TCB* TCB::createThread(Body body, void* arg, void* stackTop,
                       bool privileged, bool detached) {
    if (!body || !stackTop) return nullptr;

    TCB* thread = new TCB(body, arg, stackTop, privileged, detached);
    if (!thread) return nullptr;

    thread->state = READY;
    Scheduler::put(thread);
    return thread;
}

TCB* TCB::createMainThread() {
    TCB* thread = new TCB(nullptr, nullptr, nullptr, true, false);
    if (!thread) return nullptr;

    thread->state = RUNNING;
    running = thread;
    return thread;
}

void TCB::threadWrapper() {
    // prelazak u zadati rezim rada procesora (SPP) uz dozvoljene
    // prekide (SPIE); popSppSpie izvrsava sret cime se nastavlja
    // izvrsavanje neposredno iza njenog poziva, u novom rezimu
    if (running->privileged) Riscv::ms_sstatus(Riscv::SSTATUS_SPP);
    else Riscv::mc_sstatus(Riscv::SSTATUS_SPP);
    Riscv::ms_sstatus(Riscv::SSTATUS_SPIE);
    popSppSpie();

    running->body(running->arg);
    thread_exit();
}

void TCB::dispatch() {
    TCB* old = running;

    if (old->state == RUNNING) {
        old->state = READY;
        if (old != idleThread) Scheduler::put(old);
    }

    TCB* next = Scheduler::get();
    if (!next) next = idleThread;

    running = next;
    running->state = RUNNING;
    timeSliceCounter = 0;

    contextSwitch(&old->context, &running->context);

    // nastavak izvrsavanja niti koja je ponovo dobila procesor:
    // oslobadjaju se resursi u medjuvremenu zavrsenih niti
    reap();
}

void TCB::yield() {
    dispatch();
}

void TCB::exitRunning() {
    running->state = FINISHED;
    if (running->detached) {
        running->nextFinished = finishedHead;
        finishedHead = running;
    }
    dispatch();
}

void TCB::reap() {
    while (finishedHead) {
        TCB* thread = finishedHead;
        finishedHead = thread->nextFinished;
        if (thread->ownedStack) MemoryAllocator::kfree(thread->ownedStack);
        delete thread;
    }
}
