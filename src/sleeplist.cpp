//
// Implementacija evidencije uspavanih niti.
// Lista je uredjena po vremenu budjenja; svaki element cuva vreme budjenja
// relativno u odnosu na prethodni element, pa je na svaku periodu tajmera
// dovoljno umanjiti vreme samo prvom elementu liste.
//

#include "../h/sleeplist.hpp"
#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"

TCB* SleepList::head = nullptr;

int SleepList::sleepRunning(time_t relativeTime) {
    if (relativeTime == 0) {
        // uspavljivanje na nula perioda: samo predaja procesora
        TCB::yield();
        return 0;
    }

    TCB* thread = TCB::running;

    // pronalazenje mesta u listi: preskacu se svi elementi cije je
    // kumulativno vreme budjenja manje ili jednako zadatom
    TCB** cur = &head;
    while (*cur && (*cur)->sleepRelative <= relativeTime) {
        relativeTime -= (*cur)->sleepRelative;
        cur = &(*cur)->nextSleeping;
    }

    thread->sleepRelative = relativeTime;
    thread->nextSleeping = *cur;
    *cur = thread;

    // sledbeniku se vreme koriguje tako da ostane relativno
    if (thread->nextSleeping)
        thread->nextSleeping->sleepRelative -= relativeTime;

    thread->state = TCB::SLEEPING;
    TCB::dispatch();
    return 0;
}

void SleepList::tick() {
    if (!head) return;

    if (head->sleepRelative > 0) head->sleepRelative--;

    // bude se sve niti sa pocetka liste kojima je vreme isteklo
    while (head && head->sleepRelative == 0) {
        TCB* thread = head;
        head = thread->nextSleeping;
        thread->nextSleeping = nullptr;
        thread->state = TCB::READY;
        Scheduler::put(thread);
    }
}
