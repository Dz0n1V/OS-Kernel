//
// KSemaphore - apstrakcija semafora u jezgru, sa operacijama wait/signal
// uopstenim na n jedinica resursa i redom blokiranih niti (FIFO).
//

#ifndef _KSEMAPHORE_HPP_
#define _KSEMAPHORE_HPP_

#include "../lib/hw.h"

class TCB;

class KSemaphore {
public:
    explicit KSemaphore(unsigned init);

    // umanjuje vrednost semafora za n ili blokira tekucu nit;
    // vraca 0 u slucaju uspeha, negativnu vrednost ukoliko je semafor
    // zatvoren (ukljucujuci zatvaranje dok je nit cekala)
    int wait(unsigned n = 1);

    // uvecava vrednost semafora za n i deblokira niti kojima je to dovoljno
    int signal(unsigned n = 1);

    // zatvara semafor: deblokira sve niti koje cekaju, uz gresku u wait
    int close();

    void* operator new(size_t size);
    void operator delete(void* ptr);

    // greske operacija sa semaforom
    static const int ERR_CLOSED  = -1; // operacija nad zatvorenim semaforom
    static const int ERR_UNBLOCKED = -2; // semafor zatvoren tokom cekanja

private:
    // blokira tekucu nit koja ceka n jedinica resursa
    int block(unsigned n);

    // deblokira nit sa pocetka reda uz zadatu povratnu vrednost
    void unblockHead(int returnValue);

    uint64 value;      // broj trenutno raspolozivih jedinica resursa
    bool closed;
    TCB* headBlocked;
    TCB* tailBlocked;
};

#endif // _KSEMAPHORE_HPP_
