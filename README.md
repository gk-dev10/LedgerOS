# LedgerOS

> A bootable x86-64 educational operating system built from the ground up to demonstrate core Operating Systems concepts through a trader-centric workload.

## About

LedgerOS is a custom x86-64 operating system developed as a graded Operating Systems course project.

Instead of implementing the trading workload as a normal application running on top of Linux or Windows, LedgerOS provides its own kernel-level infrastructure for managing processes, CPU scheduling, inter-process communication, synchronization, memory, system calls, and persistent data.

The system is designed to boot independently using the Limine bootloader and is developed and tested primarily inside QEMU.

The trading environment acts as the workload through which the operating-system mechanisms can be demonstrated in a practical, integrated system.

> **The trading system is the workload. LedgerOS is the project.**

---

## Project Goals

LedgerOS aims to demonstrate how fundamental operating-system mechanisms work together in a real executable system.

The major goals are:

- Build a bootable x86-64 operating system.
- Implement custom process management.
- Implement priority-based CPU scheduling.
- Implement inter-process communication.
- Demonstrate process synchronization and race conditions.
- Implement memory-management mechanisms.
- Provide a system-call interface.
- Implement lightweight filesystem and logging functionality.
- Build a native graphical interface for monitoring the OS.
- Run a realistic market-data workload on top of the operating system.

---

## Architecture

```text
                         ┌─────────────────────┐
                         │      LedgerGUI       │
                         │   Native Dashboard   │
                         └──────────┬──────────┘
                                    │
                              System Calls
                                    │
                         ┌──────────▼──────────┐
                         │     LedgerOS Kernel  │
                         ├──────────────────────┤
                         │ Process Management   │
                         │ CPU Scheduler        │
                         │ IPC                  │
                         │ Synchronization      │
                         │ Memory Management    │
                         │ System Calls         │
                         │ Filesystem / Logging │
                         └──────────┬──────────┘
                                    │
                         ┌──────────▼──────────┐
                         │     x86-64 Hardware  │
                         └──────────┬──────────┘
                                    │
                         ┌──────────▼──────────┐
                         │   Limine / QEMU      │
                         └──────────────────────┘
```

---

## Core Modules

### Process Management

Manages processes using Process Control Blocks (PCBs), process states, process creation, termination, and process metadata.

### CPU Scheduling

Implements a priority-based scheduler responsible for selecting which ready process receives CPU time.

### Inter-Process Communication

Provides communication channels between processes using controlled shared buffers and ring-buffer mechanisms.

### Synchronization

Provides synchronization primitives for protecting shared resources and demonstrates race conditions and their resolution.

### Memory Management

Provides kernel-level memory-management mechanisms and bounded buffers used by system workloads.

### System Calls

Provides a controlled interface through which user-level applications request services from the LedgerOS kernel.

### Filesystem & Logging

Provides persistent storage and logging for system events and market-related activity.

### Retrieval Engine

Processes historical logs and provides queries for retrieving information about previously recorded market activity.

### Native GUI

LedgerOS includes a lightweight native graphical interface built on top of the framebuffer subsystem.

The GUI is intended to expose OS activity such as:

* Running processes
* Scheduler state
* Memory usage
* IPC activity
* System alerts
* Logs
* Market data

---

## Trading Workload

The operating system uses a simulated market-data environment as its primary workload.

```text
                 ┌──────────────┐
                 │  Price Feed  │
                 └──────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │      IPC      │
                └──────┬────────┘
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
       Alert         Logger      Memory
       Engine        Process      Buffer
          │            │
          └────────────┘
                       │
                       ▼
                Retrieval Engine
```

Example workloads include:

* Replaying historical price data.
* Generating market events.
* Monitoring price thresholds.
* Logging system and market events.
* Retrieving historical activity.

The workload exists primarily to exercise and demonstrate the underlying operating-system mechanisms.

---

## Technology Stack

| Component          | Technology         |
| ------------------ | ------------------ |
| Architecture       | x86-64             |
| Kernel             | C                  |
| Low-level code     | x86-64 Assembly    |
| Bootloader         | Limine             |
| Emulator           | QEMU               |
| Build System       | GNU Make           |
| Compiler Toolchain | GCC / Clang + LLVM |
| Graphics           | Linear Framebuffer |
| GUI                | Native LedgerGUI   |
| Version Control    | Git / GitHub       |

---

## Repository Structure

```text
LedgerOS/
│
├── kernel/
│   ├── arch/
│   ├── interrupts/
│   ├── process/
│   ├── scheduler/
│   ├── memory/
│   ├── ipc/
│   ├── sync/
│   ├── syscall/
│   ├── drivers/
│   ├── console/
│   └── fs/
│
├── gui/
│   ├── core/
│   ├── graphics/
│   ├── widgets/
│   └── screens/
│
├── user/
│   ├── shell/
│   ├── feed/
│   ├── alert/
│   ├── logger/
│   └── retrieval/
│
├── include/
├── assets/
├── tests/
├── docs/
├── scripts/
│
├── Makefile
├── linker.ld
└── limine.conf
```

> The repository structure will evolve as individual kernel and application modules are implemented.

---

## Development Environment

LedgerOS is developed on a host operating system but runs as an independent x86-64 kernel inside QEMU.

```text
Developer Machine
       │
       ▼
      WSL
       │
       ▼
Compiler + Make
       │
       ▼
LedgerOS Kernel
       │
       ▼
Bootable ISO
       │
       ▼
      QEMU
       │
       ▼
   LedgerOS
```

No host operating-system APIs are used to provide the core scheduling, IPC, synchronization, or memory-management mechanisms being demonstrated by the project.

---

## Building

The project is currently under active development.

Once the required build dependencies are installed:

```bash
make
```

generates the bootable LedgerOS ISO.

To launch it using QEMU:

```bash
qemu-system-x86_64 -cdrom template.iso
```

---

## Development Workflow

LedgerOS is developed using feature branches so that kernel modules can be developed independently.

Example:

```text
main
 │
 ├── feature/kernel-console
 │
 ├── feature/interrupts-timer
 │
 ├── feature/process-management
 │
 └── feature/scheduler
```

Each module is developed, tested, and committed independently before integration.

---

## Team

LedgerOS is developed collaboratively by two team members with independent module ownership and feature branches.

Both developers are expected to understand the complete system architecture and be able to explain the implementation of each major OS subsystem.

---

## Project Philosophy

LedgerOS is intentionally designed as a **real bootable system rather than a conventional OS simulator**.

The objective is not to recreate an entire production operating system. Instead, the project focuses implementation effort on the operating-system mechanisms relevant to the course while reusing appropriate low-level infrastructure such as the Limine bootloader and QEMU.

This keeps the project technically meaningful while remaining achievable within the course timeline.

---

## License

This project is developed for educational purposes as part of an Operating Systems course.
