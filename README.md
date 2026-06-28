# Dynamic Memory Allocation Handler

> A lightweight dynamic memory allocator for embedded systems, implementing heap management with 32-bit address alignment — without relying on `malloc`.

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [File Structure](#-file-structure)
- [Memory Map](#-memory-map)
- [API Reference](#-api-reference)
- [Getting Started](#-getting-started)
- [Usage Example](#-usage-example)
- [Results](#-results)

---

## 📖 Overview

This project implements a custom dynamic memory allocation handler for embedded systems. The allocator manages a dedicated heap region defined in the linker script, with memory aligned to 4-byte (32-bit) boundaries.

It is designed as a safer and more deterministic alternative to the standard `malloc`, which is generally discouraged in embedded environments due to fragmentation and non-deterministic execution time.


---

## ✅ Features

- Allocates memory from a dedicated `heap_memalloc` region defined in the linker script
- Returns the initial address of each newly allocated memory block
- Ensures 32-bit (4-byte) address alignment after every allocation
- Tracks available heap space via `FreeBytes`
- Returns `NULL` if the requested size exceeds available memory
- Heap region is zeroed at startup via custom startup code

---

## 📁 File Structure

```
MemAlloc/
├── Mem_AllocTypes.h   # Internal type definitions (MemReturnType, MemSizeType, MemHandlerType)
├── Mem_Alloc.h        # Public interface exposed to user component modules
├── Mem_Alloc.c        # Core memory allocation handler implementation
└── sam_flash.ld       # Linker configuration file with heap_memalloc region
```

| File | Description |
|---|---|
| `Mem_AllocTypes.h` | Internal data type definitions used by the memory allocation handler |
| `Mem_Alloc.h` | Public interfaces provided to user component modules |
| `Mem_Alloc.c` | Main functionality of the memory allocation handler |

---

## 🗺 Memory Map

The heap region is defined in the linker configuration file (`sam_flash.ld`):

```ld
MEMORY
{
    rom           (rx)  : ORIGIN = 0x00400000, LENGTH = 0x00200000
    heap_memalloc (rwx) : ORIGIN = 0x20400000, LENGTH = 0x00010000  /* 64KB reserved */
    ram           (rwx) : ORIGIN = 0x20410000, LENGTH = 0x00050000
    sdram         (rwx) : ORIGIN = 0x70000000, LENGTH = 0x00200000
}

STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : 0x2000;

HEAP_SIZE = DEFINED(HEAP_SIZE) ? HEAP_SIZE : 0x1000;

heap_memsize = DEFINED(heap_memsize) ? heap_memsize : 0x1000;  /* 4KB active heap */

```

| Region | Start Address | Size |
|---|---|---|
| `heap_memalloc` | `0x20400000` | 64 KB (total reserved) |
| Active heap (`heap_memsize`) | `0x20400000` | 4 KB (configurable) |

---

## 📐 API Reference

### `Mem_Alloc`

```c
MemReturnType Mem_Alloc(MemSizeType Size);
```

| Property | Value |
|---|---|
| **Sync/Async** | Synchronous |
| **Reentrancy** | Non-Reentrant |
| **Param (in)** | `MemSizeType Size` — number of bytes to allocate |
| **Param (out)** | None |
| **Return value** | Initial address of the allocated memory block, or `NULL` if out of space |

**Behavior:**
1. On first call, initializes `FreeBytes` from the linker-defined heap boundaries.
2. If `Size > FreeBytes`, returns `NULL` — allocation is not performed.
3. Otherwise, saves the current address as the return pointer, advances `CurrAddr` by `Size`, and aligns to the next 4-byte boundary if needed.
4. Updates `FreeBytes` to reflect consumed memory.

### Internal Types (`Mem_AllocTypes.h`)

```c
#define MemSizeType    uint16_t
#define MemReturnType  void*

typedef struct {
    uint8_t  *MemStart;    // Start address of the heap region
    uint8_t  *MemEnd;      // End address of the heap region
    uint8_t  *CurrAddr;    // Current (next available) address
    uint32_t  FreeBytes;   // Remaining available bytes
} MemHandlerType;
```

---

## 🚀 Getting Started

### Prerequisites

- ARM GCC Toolchain (`arm-none-eabi-gcc`)
- Compatible board: SAMV71 (or similar Cortex-M microcontroller)
- Atmel Studio / MPLAB X / VS Code with CMake

### Setup

1. Clone this repository:
   ```bash
   git clone 
   cd your-repo
   ```

2. Add the linker script (`sam_flash.ld`) with the `heap_memalloc` region as shown in [Memory Map](#-memory-map).

3. Update your startup file to zero-initialize the heap region:
   ```c
   for (pheap = &_heap_mem_start; pheap < &_heap_mem_end;) {
       *pheap++ = 0;
   }
   ```

4. Build and flash to your target device.

---

## 💡 Usage Example

```c
#include "Mem_Alloc.h"

int main(void) {
    MemHandlerType Mem_Init
    MemReturnType *p1;

    p1 = Mem_Alloc(1);      // Allocates 4 bytes (aligned)
    p1 = Mem_Alloc(100);    // Allocates 100 bytes
    p1 = Mem_Alloc(95);     // Allocates 96 bytes (aligned to 4)
    p1 = Mem_Alloc(50);     // Allocates 52 bytes (aligned to 4)
    p1 = Mem_Alloc(3000);   // Allocates 3000 bytes
    p1 = Mem_Alloc(1000);   // Returns NULL — heap exhausted

    while (1) {}
}
```

---

## 📊 Results

Tested allocations starting at base address `0x20400000`:

| Call | Requested Size | Allocated (aligned) | Returned Address |
|---|---|---|---|
| `Mem_Alloc(1)` | 1 B | 4 B | `0x20400000` |
| `Mem_Alloc(100)` | 100 B | 100 B | `0x20400004` |
| `Mem_Alloc(95)` | 95 B | 96 B | `0x20400068` |
| `Mem_Alloc(50)` | 50 B | 52 B | `0x204000C8` |
| `Mem_Alloc(3000)` | 3000 B | 3000 B | `0x204000FC` |
| `Mem_Alloc(1)` | 1 B | 4 B | `0x20400CF4` |
| `Mem_Alloc(1000)` | 1000 B | — | `NULL` (overflow) |


---


## 📄 License

This project is for academic purposes. 
