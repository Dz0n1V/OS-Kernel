//
// SleepList - evidencija uspavanih niti: ulancana lista uredjena po vremenu
// budjenja, u kojoj svaki element cuva relativno vreme budjenja u odnosu
// na prethodni element (prvi element - u odnosu na sadasnji trenutak).
//

#ifndef _SLEEP_LIST_HPP_
#define _SLEEP_LIST_HPP_

#include "../lib/hw.h"

class TCB;

class SleepList {
public:
    // uspavljuje tekucu nit na zadati broj perioda tajmera
    static int sleepRunning(time_t relativeTime);

    // azurira evidenciju na svaku periodu tajmera i budi dospele niti
    static void tick();

private:
    static TCB* head;
};

#endif // _SLEEP_LIST_HPP_
