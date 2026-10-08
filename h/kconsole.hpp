//
// KConsole - singleton klasa koja implementira spregu ka konzoli:
// baferisan ulaz (prekidna rutina je proizvodjac, sistemski poziv getc
// potrosac) i baferisan izlaz (sistemski poziv putc je proizvodjac,
// interna nit jezgra potrosac koja prozivanjem salje znakove kontroleru).
//

#ifndef _KCONSOLE_HPP_
#define _KCONSOLE_HPP_

#include "../lib/hw.h"

class KSemaphore;

class KConsole {
public:
    // inicijalizacija: semafori i interna (privilegovana) nit za slanje
    static void initialize();

    // obrada prekida od kontrolera konzole (prijem znakova sa tastature)
    static void handleInterrupt();

    // implementacije sistemskih poziva getc i putc (izvrsavaju se u jezgru)
    static int kgetc();
    static void kputc(char chr);

    // da li u izlaznom baferu ima neposlatih znakova
    static bool outPending() { return outHead != outTail; }

    // direktan, neposredan ispis (prozivanjem), za poruke o greskama jezgra
    static void directPutc(char chr);
    static void directPrint(const char* str);
    static void directPrintHex(uint64 value);

private:
    // telo interne niti jezgra koja prenosi znakove na kontroler konzole
    static void txThreadBody(void* arg);

    static const size_t IN_BUFFER_SIZE  = 512;
    static const size_t OUT_BUFFER_SIZE = 1024;

    // kruzni baferi; head pomera potrosac, tail proizvodjac
    static char inBuffer[IN_BUFFER_SIZE];
    static volatile size_t inHead, inTail;

    static char outBuffer[OUT_BUFFER_SIZE];
    static volatile size_t outHead, outTail;

    static KSemaphore* inItemAvailable;   // broj znakova u ulaznom baferu
    static KSemaphore* outItemAvailable;  // broj znakova u izlaznom baferu
    static KSemaphore* outSpaceAvailable; // slobodan prostor izlaznog bafera
};

#endif // _KCONSOLE_HPP_
