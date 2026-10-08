//
// Obrada ulazaka u prekidnu rutinu: sistemski pozivi (ecall), prekid od
// tajmera (softverski prekid), prekid od konzole (spoljasnji prekid, PLIC)
// i izuzeci usled gresaka u programu.
//

#include "../h/riscv.hpp"
#include "../h/syscall_c.h"
#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/memoryAllocator.hpp"
#include "../h/ksemaphore.hpp"
#include "../h/sleeplist.hpp"
#include "../h/kconsole.hpp"

// pozicije registara aN u sacuvanom okviru (xN na pomeraju N*8)
static const int FRAME_A0 = 10;
static const int FRAME_A1 = 11;
static const int FRAME_A2 = 12;
static const int FRAME_A3 = 13;
static const int FRAME_A4 = 14;

void handleSupervisorTrapC(uint64 frame[]) {
    Riscv::handleSupervisorTrap(frame);
}

void Riscv::handleSupervisorTrap(uint64 frame[]) {
    uint64 scause = r_scause();

    switch (scause) {
        case SCAUSE_ECALL_USER:
        case SCAUSE_ECALL_SUPERVISOR:
            handleSyscall(frame);
            break;
        case SCAUSE_SOFTWARE_INTERRUPT:
            handleTimer();
            break;
        case SCAUSE_EXTERNAL_INTERRUPT:
            handleExternal();
            break;
        default:
            handleException(scause);
            break;
    }
}

void Riscv::handleSyscall(uint64 frame[]) {
    // sepc i sstatus se cuvaju u lokalnim promenljivama (na steku tekuce
    // niti), pa prezivljavaju eventualnu promenu konteksta tokom obrade;
    // sepc se uvecava za 4 da se ecall ne bi ponovo izvrsio
    uint64 sepc = r_sepc() + 4;
    uint64 sstatus = r_sstatus();

    uint64 code = frame[FRAME_A0];
    uint64 result = 0;

    switch (code) {
        case SYS_MEM_ALLOC:
            // a1 = velicina u blokovima
            result = (uint64) MemoryAllocator::alloc((size_t) frame[FRAME_A1]);
            break;

        case SYS_MEM_FREE:
            result = (uint64) MemoryAllocator::free((void*) frame[FRAME_A1]);
            break;

        case SYS_THREAD_CREATE: {
            // a1 = thread_t* handle, a2 = telo, a3 = argument, a4 = vrh steka
            thread_t* handle = (thread_t*) frame[FRAME_A1];
            TCB::Body body = (TCB::Body) frame[FRAME_A2];
            void* arg = (void*) frame[FRAME_A3];
            void* stackTop = (void*) frame[FRAME_A4];

            if (!handle || !body || !stackTop) {
                result = (uint64) -1;
                break;
            }
            TCB* thread = TCB::createThread(body, arg, stackTop, false);
            if (!thread) {
                result = (uint64) -2;
                break;
            }
            *handle = (thread_t) thread;
            result = 0;
            break;
        }

        case SYS_THREAD_EXIT:
            TCB::exitRunning();
            // ne vraca se; ukoliko bi se ipak vratilo - greska
            result = (uint64) -1;
            break;

        case SYS_THREAD_DISPATCH:
            TCB::yield();
            break;

        case SYS_SEM_OPEN: {
            // a1 = sem_t* handle, a2 = pocetna vrednost
            sem_t* handle = (sem_t*) frame[FRAME_A1];
            if (!handle) {
                result = (uint64) -1;
                break;
            }
            KSemaphore* sem = new KSemaphore((unsigned) frame[FRAME_A2]);
            if (!sem) {
                result = (uint64) -2;
                break;
            }
            *handle = (sem_t) sem;
            result = 0;
            break;
        }

        case SYS_SEM_CLOSE: {
            KSemaphore* sem = (KSemaphore*) frame[FRAME_A1];
            if (!sem) {
                result = (uint64) -1;
                break;
            }
            result = (uint64) sem->close();
            delete sem;
            break;
        }

        case SYS_SEM_WAIT: {
            KSemaphore* sem = (KSemaphore*) frame[FRAME_A1];
            result = sem ? (uint64) sem->wait() : (uint64) -1;
            break;
        }

        case SYS_SEM_SIGNAL: {
            KSemaphore* sem = (KSemaphore*) frame[FRAME_A1];
            result = sem ? (uint64) sem->signal() : (uint64) -1;
            break;
        }

        case SYS_SEM_WAIT_N: {
            KSemaphore* sem = (KSemaphore*) frame[FRAME_A1];
            result = sem ? (uint64) sem->wait((unsigned) frame[FRAME_A2])
                         : (uint64) -1;
            break;
        }

        case SYS_SEM_SIGNAL_N: {
            KSemaphore* sem = (KSemaphore*) frame[FRAME_A1];
            result = sem ? (uint64) sem->signal((unsigned) frame[FRAME_A2])
                         : (uint64) -1;
            break;
        }

        case SYS_TIME_SLEEP:
            result = (uint64) SleepList::sleepRunning((time_t) frame[FRAME_A1]);
            break;

        case SYS_GETC:
            result = (uint64) KConsole::kgetc();
            break;

        case SYS_PUTC:
            KConsole::kputc((char) frame[FRAME_A1]);
            break;

        default:
            result = (uint64) -1;
            break;
    }

    frame[FRAME_A0] = result;

    // restauracija sepc i sstatus neposredno pre povratka
    w_sstatus(sstatus);
    w_sepc(sepc);
}

void Riscv::handleTimer() {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();

    mc_sip(SIP_SSIP);

    SleepList::tick();

    // deljenje vremena: preotimanje kada tekuca nit potrosi svoj odsecak;
    // besposlena nit gubi procesor cim se pojavi neka spremna nit
    TCB::timeSliceCounter++;
    if (TCB::timeSliceCounter >= TCB::running->timeSlice ||
        (TCB::running == TCB::idleThread && !Scheduler::empty())) {
        TCB::yield();
    }

    w_sstatus(sstatus);
    w_sepc(sepc);
}

void Riscv::handleExternal() {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();

    int irq = plic_claim();
    if (irq == (int) CONSOLE_IRQ) KConsole::handleInterrupt();
    if (irq) plic_complete(irq);

    // ukoliko je prekid probudio neku nit, a izvrsavala se besposlena nit
    if (TCB::running == TCB::idleThread && !Scheduler::empty())
        TCB::yield();

    w_sstatus(sstatus);
    w_sepc(sepc);
}

void Riscv::handleException(uint64 scause) {
    KConsole::directPrint("\r\nKernel: izuzetak, scause=");
    KConsole::directPrintHex(scause);
    KConsole::directPrint(", sepc=");
    KConsole::directPrintHex(r_sepc());
    KConsole::directPrint(", stval=");
    KConsole::directPrintHex(r_stval());
    KConsole::directPrint("; nit se gasi.\r\n");

    // nit koja je izazvala izuzetak se gasi; ne vraca se ovde
    TCB::exitRunning();
}
