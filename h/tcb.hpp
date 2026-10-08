//
// TCB (Thread Control Block) - apstrakcija niti u jezgru.
// Cuva kontekst niti (ra i sp; ostali registri se cuvaju na steku niti
// u prekidnoj rutini), telo niti, stanje i veze za razlicite liste jezgra.
//

#ifndef _TCB_HPP_
#define _TCB_HPP_

#include "../lib/hw.h"

extern "C" {
    // promena konteksta (asembler): cuva ra, sp i callee-saved registre
    // (s0..s11) tekuce niti u stari kontekst i restaurira iste registre
    // iz konteksta niti koja dobija procesor
    void contextSwitch(void* oldContext, void* runningContext);
}

class TCB {
public:
    using Body = void (*)(void*);

    enum State { CREATED, READY, RUNNING, BLOCKED, SLEEPING, FINISHED };

    // kreira nit; stackTop je vrh (najvisa adresa) vec alociranog steka
    // ili nullptr ako nit koristi zateceni stek (glavna nit);
    // privileged odredjuje da li se telo izvrsava u sistemskom rezimu
    static TCB* createThread(Body body, void* arg, void* stackTop,
                             bool privileged, bool detached = true);

    // kreira TCB za zateceni (glavni) tok kontrole
    static TCB* createMainThread();

    // predaje procesor sledecoj spremnoj niti (tekuca se vraca u red spremnih)
    static void yield();

    // gasi tekucu nit; nikada se ne vraca pozivaocu
    static void exitRunning();

    // oslobadja resurse zavrsenih (detached) niti
    static void reap();

    bool isFinished() const { return state == FINISHED; }
    bool isPrivileged() const { return privileged; }

    void* operator new(size_t size);
    void operator delete(void* ptr);

    static TCB* running;      // tekuca nit
    static TCB* idleThread;   // besposlena nit jezgra

    // brojanje vremenskog odsecka tekuce niti (deljenje vremena)
    static uint64 timeSliceCounter;

private:
    TCB(Body body, void* arg, void* stackTop, bool privileged, bool detached);

    struct Context {
        uint64 ra;
        uint64 sp;
        uint64 s[12]; // callee-saved registri s0..s11
    };

    // funkcija-omotac oko tela niti: prelazi u zadati rezim rada procesora,
    // izvrsava telo i po njegovom zavrsetku gasi nit
    static void threadWrapper();

    // bira sledecu nit i vrsi promenu konteksta
    static void dispatch();

    Body body;
    void* arg;
    void* ownedStack;     // pocetak steka koji treba osloboditi (ili nullptr)
    Context context;
    State state;
    bool privileged;
    bool detached;        // da li jezgro samo oslobadja resurse po zavrsetku
    time_t timeSlice;

    // veze za ulancavanje u razlicite liste (nit je u najvise jednoj od njih)
    TCB* nextReady;       // red spremnih niti (Scheduler)
    TCB* nextBlocked;     // red blokiranih na semaforu (KSemaphore)
    TCB* nextSleeping;    // lista uspavanih niti (SleepList)
    TCB* nextFinished;    // lista zavrsenih niti (za oslobadjanje resursa)

    time_t sleepRelative; // relativno vreme budjenja (u odnosu na prethodnika)
    unsigned semNeed;     // broj jedinica resursa koje nit ceka na semaforu
    int semReturnValue;   // vrednost koju wait vraca po deblokiranju

    static TCB* finishedHead; // lista niti cije resurse treba osloboditi

    friend class Scheduler;
    friend class KSemaphore;
    friend class SleepList;
    friend class Riscv;
};

#endif // _TCB_HPP_
