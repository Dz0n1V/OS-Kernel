//
// Implementacija sprege ka konzoli.
// Ulaz: prekidna rutina prima znakove od kontrolera i stavlja ih u ulazni
// bafer (proizvodjac), a sistemski poziv getc ih uzima (potrosac).
// Izlaz: sistemski poziv putc stavlja znakove u izlazni bafer (proizvodjac),
// a interna privilegovana nit jezgra ih prozivanjem salje kontroleru
// (potrosac). Sinhronizacija je zasnovana na semaforima jezgra.
//

#include "../h/kconsole.hpp"
#include "../h/ksemaphore.hpp"
#include "../h/tcb.hpp"
#include "../h/memoryAllocator.hpp"
#include "../h/syscall_c.h"

char KConsole::inBuffer[KConsole::IN_BUFFER_SIZE];
volatile size_t KConsole::inHead = 0;
volatile size_t KConsole::inTail = 0;

char KConsole::outBuffer[KConsole::OUT_BUFFER_SIZE];
volatile size_t KConsole::outHead = 0;
volatile size_t KConsole::outTail = 0;

KSemaphore* KConsole::inItemAvailable = nullptr;
KSemaphore* KConsole::outItemAvailable = nullptr;
KSemaphore* KConsole::outSpaceAvailable = nullptr;

void KConsole::initialize() {
    inItemAvailable = new KSemaphore(0);
    outItemAvailable = new KSemaphore(0);
    outSpaceAvailable = new KSemaphore(OUT_BUFFER_SIZE);

    // interna nit jezgra koja prozivanjem salje znakove kontroleru;
    // privilegovana je (izvrsava se u sistemskom rezimu)
    void* stack = MemoryAllocator::kmalloc(DEFAULT_STACK_SIZE);
    void* stackTop = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);
    TCB::createThread(&txThreadBody, nullptr, stackTop, true);
}

void KConsole::handleInterrupt() {
    // prijem: prazni se prijemni registar kontrolera dokle god ima znakova
    // (uz ogranicenje, da obrada prekida ne bi trajala predugo)
    for (int i = 0; i < 256; i++) {
        if (!(*((volatile uint8*) CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT))
            break;
        char chr = *((volatile uint8*) CONSOLE_RX_DATA);

        size_t nextTail = (inTail + 1) % IN_BUFFER_SIZE;
        if (nextTail == inHead) break; // bafer pun - znak se odbacuje

        inBuffer[inTail] = chr;
        inTail = nextTail;
        inItemAvailable->signal();
    }
}

int KConsole::kgetc() {
    // ceka se da se u ulaznom baferu pojavi znak
    if (inItemAvailable->wait() < 0) return -1;

    char chr = inBuffer[inHead];
    inHead = (inHead + 1) % IN_BUFFER_SIZE;
    return (int) (unsigned char) chr;
}

void KConsole::kputc(char chr) {
    // ceka se slobodno mesto u izlaznom baferu
    if (outSpaceAvailable->wait() < 0) return;

    outBuffer[outTail] = chr;
    outTail = (outTail + 1) % OUT_BUFFER_SIZE;
    outItemAvailable->signal();
}

void KConsole::txThreadBody(void*) {
    // nit je privilegovana, pa se C API pozivi (ecall iz sistemskog rezima)
    // uredno obradjuju u prekidnoj rutini (scause = 9)
    for (;;) {
        sem_wait((sem_t) outItemAvailable);

        // ceka se spremnost kontrolera za slanje, bez zauzimanja procesora
        while (!(*((volatile uint8*) CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT))
            thread_dispatch();

        char chr = outBuffer[outHead];
        outHead = (outHead + 1) % OUT_BUFFER_SIZE;
        *((volatile uint8*) CONSOLE_TX_DATA) = chr;

        sem_signal((sem_t) outSpaceAvailable);
    }
}

void KConsole::directPutc(char chr) {
    while (!(*((volatile uint8*) CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT)) {}
    *((volatile uint8*) CONSOLE_TX_DATA) = chr;
}

void KConsole::directPrint(const char* str) {
    while (*str) directPutc(*str++);
}

void KConsole::directPrintHex(uint64 value) {
    directPutc('0'); directPutc('x');
    bool started = false;
    for (int shift = 60; shift >= 0; shift -= 4) {
        uint64 digit = (value >> shift) & 0xFUL;
        if (digit != 0) started = true;
        if (started || shift == 0)
            directPutc(digit < 10 ? (char) ('0' + digit)
                                  : (char) ('a' + digit - 10));
    }
}
