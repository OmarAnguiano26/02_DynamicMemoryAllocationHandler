/*******************************************************************************/
/**
\file       Mem_Alloc.h
\brief      Dynamic Memory Allocation. Contains all the interfaces provided to the user. 
\author     Omar Anguiano / Diego Reyna
\version    0.1
\date       09/25/2023
*/

#ifndef MEM_ALLOC_H        /*prevent duplicated includes*/
#define MEM_ALLOC_H

/*-- Includes ----------------------------------------------------------------*/
#include "Mem_AllocTypes.h"

/*****************************************************************************************************
* Declaration of module wide TYPES
*****************************************************************************************************/

MemReturnType Mem_Alloc ( MemSizeType Size );


/*******************************************************************************/

#endif /* MEM_ALLOC_H */