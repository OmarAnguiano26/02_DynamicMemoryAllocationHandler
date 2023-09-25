/*******************************************************************************/
/**
\file       Mem_AllocTypes.h
\brief      Dynamic Memory Allocation. Contains all the internal data types used by the memory allocation handler module.
\author     Omar Anguiano / Diego Reyna
\version    0.1
\date       09/25/2023
*/

#ifndef MEM_ALLOCTYPES_H        /*prevent duplicated includes*/
#define MEM_ALLOCTYPES_H

/*-- Includes ----------------------------------------------------------------*/

/*****************************************************************************************************
* Declaration of module wide TYPES
*****************************************************************************************************/
//#define MemSizeType     uint16_t
//#define MemReturnType   void

typedef uint16_t    MemSizeType
typedef void        MemReturnType

typedef struct 
{
    uint8_t *MemStart;
    uint8_t *MemEnd;
    uint8_t *CurrAddr;
    uint32_t FreeBytes;
}MemHandlerType;


/*****************************************************************************************************
* Definition of module wide MACROS / #DEFINE-CONSTANTS 
*****************************************************************************************************/

/*****************************************************************************************************
* Definition of module wide VARIABLEs
*****************************************************************************************************/

/*****************************************************************************************************
* Declaration of module wide FUNCTIONS
*****************************************************************************************************/


/*******************************************************************************/

#endif /* MEM_ALLOCTYPES_H */