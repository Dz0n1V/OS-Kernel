//
// Pomocna klasa za pristup sistemskim (CSR) registrima procesora RISC-V,
// konstante vezane za obradu prekida i kodovi sistemskih poziva.
//

#ifndef _RISCV_HPP_
#define _RISCV_HPP_

#include "../lib/hw.h"

extern "C" {
    // prekidna rutina - jedinstvena ulazna tacka u jezgro (asembler)
    void supervisorTrap();

    // postavlja sepc na vrednost registra ra i izvrsava sret;
    // koristi se pri prvom pokretanju niti (prelazak u zadati rezim rada)
    void popSppSpie();

    // obrada uzroka ulaska u prekidnu rutinu (poziva je asemblerska
    // prekidna rutina, sa pokazivacem na sacuvani okvir registara)
    void handleSupervisorTrapC(uint64 frame[]);
}

class Riscv {
public:
    // Biti registra sstatus
    enum BitMaskSstatus : uint64 {
        SSTATUS_SIE  = (1UL << 1),
        SSTATUS_SPIE = (1UL << 5),
        SSTATUS_SPP  = (1UL << 8),
    };

    // Biti registara sip i sie
    enum BitMaskSip : uint64 {
        SIP_SSIP = (1UL << 1),
        SIP_STIP = (1UL << 5),
        SIP_SEIP = (1UL << 9),
    };

    // Vrednosti registra scause koje jezgro obradjuje
    enum Scause : uint64 {
        SCAUSE_ILLEGAL_INSTRUCTION = 2UL,
        SCAUSE_LOAD_PAGE_FAULT     = 5UL,
        SCAUSE_STORE_PAGE_FAULT    = 7UL,
        SCAUSE_ECALL_USER          = 8UL,
        SCAUSE_ECALL_SUPERVISOR    = 9UL,
        SCAUSE_SOFTWARE_INTERRUPT  = 0x8000000000000001UL, // tajmer
        SCAUSE_EXTERNAL_INTERRUPT  = 0x8000000000000009UL, // konzola (PLIC)
    };

    // Kodovi sistemskih poziva (ABI)
    enum SyscallCode : uint64 {
        SYS_MEM_ALLOC       = 0x01,
        SYS_MEM_FREE        = 0x02,
        SYS_THREAD_CREATE   = 0x11,
        SYS_THREAD_EXIT     = 0x12,
        SYS_THREAD_DISPATCH = 0x13,
        SYS_SEM_OPEN        = 0x21,
        SYS_SEM_CLOSE       = 0x22,
        SYS_SEM_WAIT        = 0x23,
        SYS_SEM_SIGNAL      = 0x24,
        SYS_SEM_WAIT_N      = 0x25,
        SYS_SEM_SIGNAL_N    = 0x26,
        SYS_TIME_SLEEP      = 0x31,
        SYS_GETC            = 0x41,
        SYS_PUTC            = 0x42,
    };

    // citanje/upis registra scause
    static uint64 r_scause();

    // citanje/upis registra sepc
    static uint64 r_sepc();
    static void w_sepc(uint64 sepc);

    // citanje/upis registra stvec
    static uint64 r_stvec();
    static void w_stvec(uint64 stvec);

    // citanje registra stval
    static uint64 r_stval();

    // maskiranje/demaskiranje bita registra sip
    static void ms_sip(uint64 mask);
    static void mc_sip(uint64 mask);

    // maskiranje/demaskiranje bita registra sie
    static void ms_sie(uint64 mask);
    static void mc_sie(uint64 mask);

    // citanje/upis registra sstatus
    static uint64 r_sstatus();
    static void w_sstatus(uint64 sstatus);
    static void ms_sstatus(uint64 mask);
    static void mc_sstatus(uint64 mask);

    // zaustavlja emulator qemu
    static void haltEmulator();

private:
    // obrada uzroka ulaska u prekidnu rutinu (poziva se iz asemblera)
    friend void ::handleSupervisorTrapC(uint64 frame[]);
    static void handleSupervisorTrap(uint64 frame[]);

    static void handleSyscall(uint64 frame[]);
    static void handleTimer();
    static void handleExternal();
    static void handleException(uint64 scause);
};

inline uint64 Riscv::r_scause() {
    uint64 volatile scause;
    __asm__ volatile ("csrr %[scause], scause" : [scause] "=r"(scause));
    return scause;
}

inline uint64 Riscv::r_sepc() {
    uint64 volatile sepc;
    __asm__ volatile ("csrr %[sepc], sepc" : [sepc] "=r"(sepc));
    return sepc;
}

inline void Riscv::w_sepc(uint64 sepc) {
    __asm__ volatile ("csrw sepc, %[sepc]" : : [sepc] "r"(sepc));
}

inline uint64 Riscv::r_stvec() {
    uint64 volatile stvec;
    __asm__ volatile ("csrr %[stvec], stvec" : [stvec] "=r"(stvec));
    return stvec;
}

inline void Riscv::w_stvec(uint64 stvec) {
    __asm__ volatile ("csrw stvec, %[stvec]" : : [stvec] "r"(stvec));
}

inline uint64 Riscv::r_stval() {
    uint64 volatile stval;
    __asm__ volatile ("csrr %[stval], stval" : [stval] "=r"(stval));
    return stval;
}

inline void Riscv::ms_sip(uint64 mask) {
    __asm__ volatile ("csrs sip, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::mc_sip(uint64 mask) {
    __asm__ volatile ("csrc sip, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::ms_sie(uint64 mask) {
    __asm__ volatile ("csrs sie, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::mc_sie(uint64 mask) {
    __asm__ volatile ("csrc sie, %[mask]" : : [mask] "r"(mask));
}

inline uint64 Riscv::r_sstatus() {
    uint64 volatile sstatus;
    __asm__ volatile ("csrr %[sstatus], sstatus" : [sstatus] "=r"(sstatus));
    return sstatus;
}

inline void Riscv::w_sstatus(uint64 sstatus) {
    __asm__ volatile ("csrw sstatus, %[sstatus]" : : [sstatus] "r"(sstatus));
}

inline void Riscv::ms_sstatus(uint64 mask) {
    __asm__ volatile ("csrs sstatus, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::mc_sstatus(uint64 mask) {
    __asm__ volatile ("csrc sstatus, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::haltEmulator() {
    *((volatile uint32*) 0x100000UL) = 0x5555;
}

#endif // _RISCV_HPP_
