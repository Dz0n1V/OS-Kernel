//
// Implementacija semafora u jezgru.
// Vrednost semafora se cuva kao nenegativan broj raspolozivih jedinica
// resursa, dok red blokiranih niti pamti za svaku nit broj jedinica koje
// ona zahteva (semNeed), cime su operacije wait_n i signal_n prirodno
// podrzane (wait i signal su specijalni slucaj za n = 1).
//

#include "../h/ksemaphore.hpp"
#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/memoryAllocator.hpp"

void* KSemaphore::operator new(size_t size) {
    return MemoryAllocator::kmalloc(size);
}

void KSemaphore::operator delete(void* ptr) {
    MemoryAllocator::kfree(ptr);
}

KSemaphore::KSemaphore(unsigned init)
        : value(init), closed(false),
          headBlocked(nullptr), tailBlocked(nullptr) {
}

int KSemaphore::wait(unsigned n) {
    if (closed) return ERR_CLOSED;
    if (n == 0) return 0;

    // ukoliko niko ne ceka, a resursa ima dovoljno, operacija odmah uspeva;
    // u suprotnom se nit blokira (postuje se FIFO redosled cekanja)
    if (!headBlocked && value >= n) {
        value -= n;
        return 0;
    }
    return block(n);
}

int KSemaphore::block(unsigned n) {
    TCB* thread = TCB::running;
    thread->semNeed = n;
    thread->semReturnValue = 0;
    thread->nextBlocked = nullptr;

    if (tailBlocked) tailBlocked->nextBlocked = thread;
    else headBlocked = thread;
    tailBlocked = thread;

    thread->state = TCB::BLOCKED;
    TCB::dispatch();

    // nit je deblokirana operacijom signal (0) ili zatvaranjem semafora (<0);
    // objekat semafora se ovde ne sme koristiti jer je mogao biti unisten
    return thread->semReturnValue;
}

void KSemaphore::unblockHead(int returnValue) {
    TCB* thread = headBlocked;
    headBlocked = thread->nextBlocked;
    if (!headBlocked) tailBlocked = nullptr;
    thread->nextBlocked = nullptr;

    thread->semReturnValue = returnValue;
    thread->state = TCB::READY;
    Scheduler::put(thread);
}

int KSemaphore::signal(unsigned n) {
    if (closed) return ERR_CLOSED;

    value += n;
    // deblokiraju se niti sa pocetka reda dokle god ima dovoljno resursa
    while (headBlocked && headBlocked->semNeed <= value) {
        value -= headBlocked->semNeed;
        unblockHead(0);
    }
    return 0;
}

int KSemaphore::close() {
    if (closed) return ERR_CLOSED;

    closed = true;
    // sve niti koje su se zatekle na cekanju se deblokiraju uz gresku
    while (headBlocked) unblockHead(ERR_UNBLOCKED);
    return 0;
}
