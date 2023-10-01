/*******************************************************************************/
/**
\file       Mem_Malloc.c
\brief      Dynamic Memory Allocation.
\author     Omar Anguiano / Diego Reyna
\version    0.1
\date       09/25/2023
*/

/** Variable types and common definitions */
#include "system_samv71.h"
#include "Mem_Alloc.h"

    extern uint32_t _heap_mem_start;
    extern uint32_t _heap_mem_end;

   static const MemHandlerType MemControl =
    {
        .MemStart = (uint8_t *) &_heap_mem_start, /* Sets the start of the heap memory */
        .MemEnd = (uint8_t *) &_heap_mem_end, /* Sets the end of the heap memory */
        .CurrAddr = (uint8_t *) &_heap_mem_start, /* Initialize the current start address */
        .FreeBytes = (uint8_t *) &_heap_mem_end - (uint8_t *) &_heap_mem_start /* Sets the size of the heap memory */
    };

MemReturnType Mem_Alloc ( MemSizeType Size )
{
    if(Size > MemControl.FreeBytes)/**No more free bytes on heap*/
    {
        return NULL;
    }
    MemReturnType *ptr = .CurrAddr;
}

