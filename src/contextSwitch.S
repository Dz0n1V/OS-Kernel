# Promena konteksta niti i pomocna rutina za prvo pokretanje niti.

.section .text

# void contextSwitch(void* oldContext, void* runningContext);
# Cuva registre ra, sp i s0..s11 tekuce niti u njen kontekst (a0), a potom
# restaurira iste registre iz konteksta niti kojoj se predaje procesor (a1).
# Ostali programski dostupni registri su vec sacuvani na steku niti: u
# prekidnoj rutini (asinhrona promena) ili ih cuva prevodilac po konvenciji
# o pozivu funkcije (caller-saved registri).
.global contextSwitch
.type contextSwitch, @function
contextSwitch:
    sd ra,   0(a0)
    sd sp,   8(a0)
    sd s0,  16(a0)
    sd s1,  24(a0)
    sd s2,  32(a0)
    sd s3,  40(a0)
    sd s4,  48(a0)
    sd s5,  56(a0)
    sd s6,  64(a0)
    sd s7,  72(a0)
    sd s8,  80(a0)
    sd s9,  88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    ld ra,   0(a1)
    ld sp,   8(a1)
    ld s0,  16(a1)
    ld s1,  24(a1)
    ld s2,  32(a1)
    ld s3,  40(a1)
    ld s4,  48(a1)
    ld s5,  56(a1)
    ld s6,  64(a1)
    ld s7,  72(a1)
    ld s8,  80(a1)
    ld s9,  88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)
    ret

# void popSppSpie();
# Upisuje povratnu adresu (ra) u registar sepc i izvrsava sret, cime se
# izvrsavanje nastavlja od instrukcije iza poziva ove rutine, ali u rezimu
# rada odredjenom bitom SPP i sa vrednoscu bita SIE preuzetom iz SPIE.
# Pozivalac je duzan da pre poziva postavi bite SPP i SPIE registra sstatus.
.global popSppSpie
.type popSppSpie, @function
popSppSpie:
    csrw sepc, ra
    sret
