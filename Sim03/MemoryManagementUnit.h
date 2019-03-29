//Header File Information//////////////////////////////////////////////////////
/*
 * @file MemoryManagementUnit.h
 *
 * @version 3.00
 *          Kristopher Moore (26 March 2019)
 *          Initial Sim03 Build.
 */
 
#ifndef MMU_H
#define MMU_H

#include "ConfigAccess.h"

//MMU structure, linked list
typedef struct MMU
{
   int pId;
   int segNumber;
   int memBase;
   int memOffset;

   struct MMU *next;
} MMU;

//function prototypes
int mmuAllocate(ConfigDataType* configDataPtr, MMU* mmuCurrentPtr, 
                                                MMU* mmuHeadPtr, MMU mmuData);
int mmuAccess(ConfigDataType* configDataPtr, MMU* mmuCurrentPtr, 
                                                MMU* mmuHeadPtr, MMU mmuData);
MMU* addNodeMMU( MMU* localPtr, MMU* newNode );
MMU* clearMMU( MMU* localPtr );
MMU fillMMU( int pid, int segNumber, int memBase, int memOffset);
#endif // MMU_H