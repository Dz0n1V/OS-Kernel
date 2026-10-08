//
// Implementacija rasporedjivaca - FIFO (FCFS) red spremnih niti.
//

#include "../h/scheduler.hpp"
#include "../h/tcb.hpp"

TCB* Scheduler::head = nullptr;
TCB* Scheduler::tail = nullptr;

void Scheduler::put(TCB* thread) {
    if (!thread) return;

    thread->nextReady = nullptr;
    if (tail) tail->nextReady = thread;
    else head = thread;
    tail = thread;
}

TCB* Scheduler::get() {
    if (!head) return nullptr;

    TCB* thread = head;
    head = head->nextReady;
    if (!head) tail = nullptr;
    thread->nextReady = nullptr;
    return thread;
}
