//Program Information///////////////////////////////////////////////////////////
/*
 * @file MemoryManagementUnit.c
 *
 *
 * @version 3.00
 *          Kristopher Moore (26 March 2019)
 *          Initial Sim03 Build.
 */
 
#include "MemoryManagementUnit.h" 

/*
Function name: mmuAllocate
Algorithm: 
Precondition: 
Postcondition: 
Exceptions: none
Notes: none
*/
int mmuAllocate(ConfigDataType* configDataPtr, MMU* mmuCurrentPtr, 
                                                   MMU* mmuHeadPtr, MMU mmuData)
{
   //initializations
   MMU* newNodePtr = NULL;
   MMU* oldNodePtr = NULL;
   int memToUse = (mmuData.memBase * 1024) + mmuData.memOffset;
   
   //check if this alloc will stay under config file limit
   if( configDataPtr->memAvailable < memToUse )
   {
      //return a segfault to calling function
      return 1;
   }      
  
   //CHECK if any same base exists anywhere in the MMU linked list:
   oldNodePtr = mmuHeadPtr;
   while( oldNodePtr != NULL )
   {
      //check if we are accessing a previously allocated base, if so segfault
      if( oldNodePtr->memBase == mmuData.memBase )
      {
         //return seg fault to calling function
         return 1;
      }
      
      oldNodePtr = oldNodePtr->next;
   }
   
   
   //Since we have an allocatation, we need to add the MMU to our linkedList.
   //allocate space, and ensure we intialize it
   newNodePtr = ( MMU * ) malloc( sizeof( MMU ) );
   newNodePtr->pId = mmuData.pId;
   newNodePtr->segNumber = mmuData.segNumber;
   newNodePtr->memBase = mmuData.memBase;
   newNodePtr->memOffset = mmuData.memOffset;
     
   //add node and adjust ptr
   mmuCurrentPtr = addNodeMMU( mmuCurrentPtr, newNodePtr );
   mmuCurrentPtr = mmuCurrentPtr->next;
      
   //Get last node position, so we can set it
   while( mmuCurrentPtr != NULL )
   {
      oldNodePtr = mmuCurrentPtr;
      mmuCurrentPtr = mmuCurrentPtr->next;
   }
   
   free( newNodePtr );
   
   return 0;
}

/*
Function name: mmuAccess
Algorithm: 
Precondition: 
Postcondition: 
Exceptions: none
Notes: none
*/
int mmuAccess(ConfigDataType* configDataPtr, MMU* mmuCurrentPtr, 
                                                   MMU* mmuHeadPtr, MMU mmuData)
{
   //initializations
   MMU* oldNodePtr = NULL;  
  
   //check through list for our memory to access
   oldNodePtr = mmuHeadPtr;
   while( oldNodePtr != NULL )
   {
      //ensure we have proper access, and we arent trying to overshoot the data
      if( oldNodePtr->segNumber == mmuData.segNumber
            && oldNodePtr->memBase == mmuData.memBase
            && oldNodePtr->memOffset >= mmuData.memOffset )
      {
         //we found our memory to access and it was safe, proceed
         return 0;
      }
      
      oldNodePtr = oldNodePtr->next;
   }
   
   //if we get here we didnt have proper access so return a segfault
   return 1;
}

/*
Function name: addNodeMMU
Algorithm: responsible for allocation of space for new node in MMU
Precondition: ptr to list, and a new node
Postcondition: last node in list
Exceptions: none
Notes: none
*/
MMU* addNodeMMU( MMU* localPtr, MMU* newNode )
{
   // check for local pointer assigned to null
   if( localPtr == NULL )
   {
      // access memory for new link/node
      localPtr = ( MMU* ) malloc( sizeof( MMU ) );
      
      // assign relevant values
      localPtr->pId = newNode->pId;
      localPtr->segNumber = newNode->segNumber;
      localPtr->memBase = newNode->memBase;
      localPtr->memOffset = newNode->memOffset;
      
      // assign next pointer to null
      localPtr->next = NULL;

      // return current local pointer
      return localPtr;
   }

   // assume end of list not found yet
   // assign recursive function to current's next link
   localPtr->next = addNodeMMU( localPtr->next, newNode );

   // return current local pointer
   return localPtr;
}

/*
Function name: clearMMU
Algorithm: responsible freeing all memory allocated by log linked list
Precondition: ptr to the head of list
Postcondition: returns null after all memory has been freed
Exceptions: none
Notes: none
*/
MMU* clearMMU( MMU* localPtr )
{
   // check for local pointer not set to null (list not empty)
   if( localPtr != NULL )
   {
      // check for local pointer's next node not null
      if( localPtr->next != NULL )
      {
         // call recursive function with next pointer
         clearMMU( localPtr->next );
      }

      // after recursive call, release memory to OS
      free( localPtr );
   }
   
   return NULL; 
}

MMU fillMMU( int pid, int segNumber, int memBase, int memOffset)
{
   MMU mmu;
   mmu.pId = pid;
   mmu.segNumber = segNumber;
   mmu.memBase = memBase;
   mmu.memOffset = memOffset;
   
   return mmu;
}








