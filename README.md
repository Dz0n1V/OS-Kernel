# OS1 – A Kernel for RISC-V

Project for the **Operating Systems 1** course (School of Electrical Engineering, University of Belgrade, 2025/2026): a small but fully functional multithreaded, time-sharing kernel for the **RISC-V RV64IMA** processor, written in C++ and assembly. It runs in the **QEMU** emulator (the host system is a modified xv6 provided in `hw.lib`).

## Features

- memory allocator (first-fit, contiguous allocation)
- threads with synchronous and asynchronous context switching (preemption on timer and console interrupts)
- semaphores (`wait`/`signal` and the `wait_n`/`signal_n` variants)
- thread sleeping (`time_sleep`) and periodic threads
- console (`getc`/`putc`) with buffering and interrupts
- three interface layers: **ABI** (`ecall`) → **C API** → **C++ API**

## Architecture

The kernel is statically linked with the user program into a single executable. The kernel's `main()` initializes the system and starts a thread in user (U) mode running `userMain()`, which is provided by the user program (e.g. the tests). Kernel code runs in supervisor (S) mode, on the stack of the current thread, with interrupts masked. A single trap handler (`supervisorTrap`) handles system calls, timer and console interrupts, and exceptions.

```
userMain (user program)
   │
C++ API  (syscall_cpp)
C API    (syscall_c)
ABI      (ecall)  ──►  supervisorTrap  ──►  kernel (S mode)
                                              │
                                         hw.lib (platform)
```

## Project structure

### `h/` – headers

| File | Description |
|---|---|
| `syscall_c.h` | Declarations of the C API (`mem_alloc`, `thread_create`, `sem_open`, `getc`, ...). |
| `syscall_cpp.hpp` | Object-oriented C++ API (`Thread`, `Semaphore`, `PeriodicThread`, `Console`). |
| `riscv.hpp` | `Riscv` class: access to control registers, system call codes, and trap cause handling. |
| `memoryAllocator.hpp` | Memory allocator interface (block allocation and internal kernel allocation). |
| `tcb.hpp` | `TCB` class: thread control block, thread states, and context switching operations. |
| `scheduler.hpp` | Scheduler interface (FIFO queue of ready threads). |
| `ksemaphore.hpp` | Kernel semaphore interface with a queue of blocked threads. |
| `sleeplist.hpp` | Interface of the sleeping-threads list, ordered by wake-up time. |
| `kconsole.hpp` | Console interface: input/output buffers and console interrupt handling. |

### `src/` – implementation

| File | Description |
|---|---|
| `main.cpp` | Kernel entry point: initialization, starting the `userMain` thread, and clean emulator shutdown. |
| `supervisorTrap.S` | Assembly trap handler: saves and restores registers and calls the trap cause dispatcher. |
| `contextSwitch.S` | Assembly context switch (`ra`, `sp`, `s0–s11`) and a helper routine for starting a thread for the first time. |
| `riscv.cpp` | Handling of system calls, timer and console interrupts, and exceptions. |
| `memoryAllocator.cpp` | First-fit allocator with coalescing of adjacent free segments. |
| `tcb.cpp` | Thread creation and termination, `dispatch`/`yield`, and stack reclamation for finished threads. |
| `scheduler.cpp` | FIFO scheduler implementation. |
| `ksemaphore.cpp` | Semaphore implementation: blocking, waking up, and closing with an error. |
| `sleeplist.cpp` | Sleeping-threads list with relative wake-up times, updated on every timer tick. |
| `kconsole.cpp` | Console: semaphore-guarded buffers, an internal kernel thread for sending, and interrupt-driven receiving. |
| `syscall_c.cpp` | C API implementation: argument preparation and `ecall` execution. |
| `syscall_cpp.cpp` | C++ API implementation as a wrapper around the C API, including global `new`/`delete`. |

## Building and running

A `riscv64` GCC toolchain and `qemu-system-riscv64` are required. The project is used together with the course's base project (`Makefile`, `kernel.ld`, `lib/hw.lib`), and the user program (`userMain`) is provided by a `test/` directory in the project root:

```
project-base/
├── Makefile
├── kernel.ld
├── lib/
├── src/
├── h/
└── test/
```

```
make clean     # clean build artifacts
make           # build
make qemu      # run in the emulator (exit with Ctrl+A, then X)
```

The interactive tests (3, 4, 6) are terminated with the **ESC** key and are best run from a real terminal.

## Notes

- The project does not use any standard libraries (`-nostdlib`); all services are implemented in the kernel itself.
- The kernel is uniprocessor, with no preemption while kernel code is executing.
- Keep the project on a case-sensitive file system (not on a shared Windows folder), because the Makefile creates temporary `.s` files from `.S` files.
