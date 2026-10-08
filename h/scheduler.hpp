//
// Scheduler - singleton klasa koja implementira FIFO (FCFS) algoritam
// rasporedjivanja nad redom spremnih niti.
//

#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

class TCB;

class Scheduler {
public:
    // dodaje nit na kraj reda spremnih
    static void put(TCB* thread);

    // skida nit sa pocetka reda spremnih; nullptr ako je red prazan
    static TCB* get();

    static bool empty() { return head == nullptr; }

private:
    static TCB* head;
    static TCB* tail;
};

#endif // _SCHEDULER_HPP_
