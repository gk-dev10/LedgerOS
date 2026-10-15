# LedgerOS — Evaluation & Live Demonstration Guide

> **Project Title**: LedgerOS: Design and Implementation of a Preemptive Scheduler, Synchronization Primitives, and IPC-Driven Market Workload on a Bare-Metal x86-64 Kernel  
> **Platform**: x86-64 Bare-Metal (Booting via Limine Protocol v6 inside QEMU)

---

## 🚀 Quick Start / Running the OS in QEMU

1. **Build the Kernel**:
   ```bash
   make -C kernel
   ```
2. **Launch QEMU**:
   ```bash
   qemu-system-x86_64 -kernel kernel/bin/kernel
   ```

---

## 📋 Step-by-Step Live Demonstration Script for Evaluation

Follow this exact sequence during your project demonstration to highlight all core OS concepts to the evaluator:

### Step 1: System Info & Boot Diagnostics
Type:
```bash
info
```
* **What it demonstrates**: Boots directly into x86-64 Long Mode via Limine Protocol v6. Shows live uptime counter driven by the Programmable Interval Timer (PIT 8254) at 100 Hz.

---

### Step 2: Memory Management, Chunk Coalescing & Fragmentation
Type:
```bash
mem
alloc 40960
mem chunks
free <address>
mem chunks
vmm stats
```
* **What it demonstrates**:
  * `mem`: Shows the 4MB kernel heap allocator (`kmalloc`/`kfree`).
  * `alloc 40960`: Dynamically carves 40 KB from the heap.
  * `mem chunks`: Dumps the heap block table, showing payload addresses, chunk sizes, and internal/external fragmentation.
  * `free <hex_address>`: Frees that block and merges adjacent free chunks back together (**coalescing**).
  * `vmm stats`: Shows the Physical Frame Allocator (PMM 4KB page blocks) and 4-level x86-64 Paging architecture (`PML4` -> `PDPT` -> `PD` -> `PT`).

---

### Step 3: Process Control Blocks & Preemptive Scheduling
Type:
```bash
ps
spawn TraderWorker
renice 2 8
ps
sched
```
* **What it demonstrates**:
  * `ps`: Displays active Process Control Blocks (PCBs), PIDs, state enums (`READY`, `RUNNING`, `BLOCKED`, `TERMINATED`), priority levels (0–10), and CPU tick counters.
  * `spawn TraderWorker`: Dynamically allocates a new PCB and 16KB stack via `kmalloc`.
  * `renice <pid> <priority>`: Dynamically alters process scheduling priority live.
  * `sched`: Triggers the assembly-level preemptive context switch (`switch.S`). Shows hardware PIT timer interrupts interrupting worker threads and swapping CPU stack pointers (`RSP`). Shows PID 0 Idle Task running `hlt` loop when idle.

---

### Step 4: Autonomous IPC Market Workload & Priority Boosting
Type:
```bash
market start
```
* **What it demonstrates**: Launches an autonomous 4-process market pipeline running concurrently on the preemptive scheduler:
  1. **Market Feed Producer (PID 1 / Priority 6)**: Generates stock/crypto ticks (`NVDA`, `BTC`, `AAPL`, `TSLA`, `ETH`) and volatility spikes.
  2. **Order Matching Engine (PID 2 / Priority 5)**: Consumes ticks via blocking IPC semaphores and executes orders.
  3. **Alert Engine (PID 3 / Priority 9)**: High-priority task that receives high-volatility tick alerts with dynamic **priority boosting** to mitigate priority inversion.
  4. **Logger Process (PID 4 / Priority 3)**: Records trade execution events.

Type:
```bash
market stop
```
to cleanly stop the market pipeline.

---

### Step 5: Concurrency Fault Injection (Race Condition Demo)
Type:
```bash
race demo
```
* **What it demonstrates**: Spawns two unsynchronized worker processes concurrently modifying a shared memory integer without mutexes or spinlocks. Demonstrates timer-driven interleaving causing lost updates and proving why synchronization primitives are required.

---

### Step 6: System Calls & Ramdisk LedgerFS Audit Storage
Type:
```bash
log view
cat /var/log/market.log
```
* **What it demonstrates**: Shows records written via x86-64 `SYSCALL`/`SYSRET` (`MSR_LSTAR = 0xC0000082`) into Ramdisk `LedgerFS` persistent log storage (`/var/log/market.log`).

---

### Step 7: Native LedgerGUI Graphical Dashboard
Type:
```bash
gui
```
* **What it demonstrates**: Renders a live 4-window graphical dashboard directly on the linear framebuffer:
  1. **CPU & Task Monitor**: Process state cards and priority bars.
  2. **Memory Allocation Gauges**: Visual bar graphs for Heap & 4KB Page Frame allocation.
  3. **IPC Mailbox Meter**: Bounded buffer ring-buffer capacity meter.
  4. **Live Market Price Chart**: Real-time ticker line graph plotting stock/crypto prices.
* **To Return to Shell**: Press **`ESC`** or **`q`** (or in the QEMU terminal).

#### Stream Binance prices into the dashboard
Start QEMU with `make run`, enter `gui` in LedgerOS, then run this in a separate host terminal:
```powershell
python scripts/market_feed_bridge.py
```
The bridge fetches BTC and ETH prices from Binance and sends them over QEMU's COM2 TCP serial port (`127.0.0.1:4444`). The chart marks incoming values as **LIVE API**. If the bridge is stopped or stops sending updates, the dashboard returns to simulated prices. Keep QEMU running while the bridge is connected.

---

## 🎯 Quick Technical Viva / Question Cheatsheet

| Question by Evaluator | Technical Answer to Give | Key Source File |
| :--- | :--- | :--- |
| **How is context switching implemented?** | Implemented in assembly in `cpu_switch_to`. Saves callee-saved registers (`rbx`, `rbp`, `r12`-`r15`) onto the old task stack and updates `RSP` to the new task's `rsp`. | [switch.S](file:///c:/Users/yoges/Downloads/OS/LedgerOS/kernel/src/process/switch.S) |
| **How are race conditions prevented?** | Using atomic spinlocks (`__atomic_test_and_set`), blocking `mutex_t` (sleep queues), and counting `semaphore_t`. | [mutex.c](file:///c:/Users/yoges/Downloads/OS/LedgerOS/kernel/src/sync/mutex.c) |
| **How do system calls work on x86-64?** | Configured `MSR_STAR`, `MSR_SFMASK`, and `MSR_LSTAR` (0xC0000082) pointing to assembly stub `syscall_entry.S` which invokes `syscall_handler`. | [syscall.c](file:///c:/Users/yoges/Downloads/OS/LedgerOS/kernel/src/syscall/syscall.c) |
| **How does priority inversion prevention work?** | When a high-volatility market spike arrives, the Alert Engine receives temporary **priority boosting** (boosted to priority 10) to prevent starvation. | [market.c](file:///c:/Users/yoges/Downloads/OS/LedgerOS/kernel/src/workload/market.c) |
