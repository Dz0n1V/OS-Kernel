//
// Glavni program jezgra: inicijalizacija (prekidna rutina, glavna i
// besposlena nit, konzola), pokretanje korisnickog programa (userMain)
// u zasebnoj niti u korisnickom rezimu i uredno gasenje sistema.
//

#include "../h/riscv.hpp"
#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/memoryAllocator.hpp"
#include "../h/kconsole.hpp"
#include "../h/syscall_c.h"

extern void userMain();

// telo besposlene niti: izvrsava se samo kada nema spremnih niti
static void idleBody(void*) {
    for (;;) {}
}

// omotac oko userMain (potpis tela niti prima argument)
static void userMainWrapper(void*) {
    userMain();
}

int main() {
    // prekidi su maskirani tokom rada jezgra
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);

    // registracija prekidne rutine
    Riscv::w_stvec((uint64) &supervisorTrap);

    // TCB za zateceni (glavni) tok kontrole
    TCB* mainThread = TCB::createMainThread();
    if (!mainThread) return -1;

    // besposlena nit (privilegovana); ne stavlja se u red spremnih,
    // bira se samo kada je red prazan
    void* idleStack = MemoryAllocator::kmalloc(DEFAULT_STACK_SIZE);
    if (!idleStack) return -1;
    TCB::idleThread = TCB::createThread(
            &idleBody, nullptr,
            (void*) ((uint64) idleStack + DEFAULT_STACK_SIZE), true);
    if (!TCB::idleThread) return -1;
    // createThread je stavlja u red spremnih; posto je u ovom trenutku
    // jedina u redu, uklanja se odatle (bira se samo kad je red prazan)
    Scheduler::get();

    KConsole::initialize();

    // demaskiranje softverskog (tajmer) i spoljasnjeg (konzola) prekida
    Riscv::ms_sie(Riscv::SIP_SSIP | Riscv::SIP_SEIP);

    // nit nad korisnickim programom (izvrsava se u korisnickom rezimu)
    void* userStack = MemoryAllocator::kmalloc(DEFAULT_STACK_SIZE);
    if (!userStack) return -1;
    TCB* userThread = TCB::createThread(
            &userMainWrapper, nullptr,
            (void*) ((uint64) userStack + DEFAULT_STACK_SIZE),
            false, false);
    if (!userThread) return -1;

    // dozvola prekida: od ovog trenutka sistem zivi
    Riscv::ms_sstatus(Riscv::SSTATUS_SIE);

    // glavna nit ceka da se korisnicki program zavrsi
    while (!userThread->isFinished()) thread_dispatch();

    // ceka se da interna nit konzole posalje sve preostale znakove
    while (KConsole::outPending()) thread_dispatch();

    delete userThread;
    MemoryAllocator::kfree(userStack);
    Riscv::haltEmulator();
    return 0;
}
