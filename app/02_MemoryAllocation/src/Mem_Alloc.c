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
#include <stddef.h>

    extern uint32_t _heap_mem_start;
    extern uint32_t _heap_mem_end;
    extern uint32_t _heap_mem_size;
    //uint32_t heapsize = (uint8_t *) &_heap_mem_end - (uint8_t *) &_heap_mem_start;

    MemHandlerType MemControl =
    {
        .MemStart = (uint8_t *) &_heap_mem_start, /* Sets the start of the heap memory */
        .MemEnd = (uint8_t *) &_heap_mem_end, /* Sets the end of the heap memory */
        .CurrAddr = (uint8_t *) &_heap_mem_start, /* Initialize the current start address */
        .FreeBytes = 0x1000//(uint8_t *) &_heap_mem_end - (uint8_t *) &_heap_mem_start /* Sets the size of the heap memory */
    };

MemReturnType Mem_Alloc ( MemSizeType Size )
{
    if(Size > MemControl.FreeBytes) /**No more free bytes on heap*/
    {
        return NULL;
    }
    else
    {
        MemReturnType ptr = MemControl.CurrAddr;
        if(Size%4 == 0) /**It is alligned with 4 bytes*/
        {
            MemControl.FreeBytes = MemControl.FreeBytes - Size;
            MemControl.CurrAddr = MemControl.CurrAddr + Size;
        }
        else /**Aligns with 4bytes*/
        {
            MemControl.FreeBytes = MemControl.FreeBytes - ( Size + ( 4 - ( Size%4 ) ) );
            MemControl.CurrAddr = MemControl.CurrAddr + ( Size + ( 4 - ( Size%4 ) ));
        }
        return ptr;

    }
}

