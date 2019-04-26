//Program Information///////////////////////////////////////////////////////////
/*
 * @file SimRunner.c
 *
 *
 * @version 2.00
 *          Kristopher Moore (19 Feburary 2019)
 *          Initial Sim02 Build.
 */

#include "SimRunner.h"
 
/*
Function name: simulationRunner
Algorithm: utilizes metadata and config data to create processes with PCBs,
            then uses a scheduler to select a process, and iterates through them
            using timer functions to emulate a running. Pthreads are used for
            I/O ops.
Precondition: correctly configured config.cnf and metadata.mdf files
Postcondition: simulates in accordance with specifications of config file
Exceptions: none
Notes: none
*/
int simulationRunner(ConfigDataType* configDataPtr, OpCodeType* mdData)
{
   //initializations
   char timeString[MAX_STR_LEN];
   char completeLog[MAX_STR_LEN];
   LogLinkedList* newNodePtr;
   LogLinkedList* listHeadPtr = NULL;
   LogLinkedList* listCurrentPtr = NULL;
   MMU* mmuNewPtr;
   MMU* mmuHeadPtr = NULL;
   MMU* mmuCurrentPtr = NULL;
   char checkIfFile[STD_STR_LEN];
   OpCodeType* loopMetaDataPtr = mdData;
   OpCodeType* currentProgramCounter;
   PCB* process = NULL;
   int processCount = 0;
   int indexI = 0;
   int scheduledProcess = 0;
   int oldScheduledProcess = 0;
   int segFaultFlag = 0;
   int interruptedPid = 0;
   Boolean idleFlag = False;
   Boolean processingFlag = True;
   Boolean isFirstRun = True;
   EventData eventData;
   
   //buffer in a value for completeLog, to avoid unintialized access
   copyString( completeLog, " " );
   
   //IMPORTANT: initialization steps for our Logging linked list,
   newNodePtr = ( LogLinkedList * ) malloc( sizeof( LogLinkedList ) );
   copyString( newNodePtr->logLine, " " );                              
   listCurrentPtr = addNodeLL( listCurrentPtr, newNodePtr );
   listHeadPtr = listCurrentPtr;                             
   copyString( listCurrentPtr->logLine, " " );
   concatenateString( listCurrentPtr->logLine, "" );
   
   //IMPORTANT: initilization steps for our MMU (linked list) to be safe
   mmuNewPtr = ( MMU * ) malloc( sizeof( MMU ) );
   mmuNewPtr->pId = -1;
   mmuNewPtr->segNumber = -1;
   mmuNewPtr->memBase = -1;
   mmuNewPtr->memOffset = -1;
   mmuCurrentPtr = addNodeMMU( mmuCurrentPtr, mmuNewPtr );
   mmuHeadPtr = mmuCurrentPtr;
   
   
   //Start Event Logging
   printf( "==========================\n" );
   printf( "Begin Simulation\n\n" );
   
   
   //EVENT LOG: System Start
   accessTimer( ZERO_TIMER, timeString );
   eventData = generateEventData( OS, SystemStart, timeString, mdData, process);
   eventLogger( eventData, configDataPtr, listCurrentPtr );
   
   
   //EVENT LOG: Create PCB'S
   accessTimer( LAP_TIMER, timeString );
   eventData = generateEventData( OS, CreatePCBs, timeString, mdData, process );
   eventLogger( eventData, configDataPtr, listCurrentPtr );
   
   
   //find processCount, as we will need this to create our PCB structure
   processCount = findProcessCount( loopMetaDataPtr, mdData );
   
   
   //Now with that count, create PCB structs for each process
   //And initilize each in NEW state / attach their Program Counter
   PCB pcbArray[processCount];
   createPCBs( pcbArray, loopMetaDataPtr, processCount );
   
   //IMPORTANT: initialization of interruptQueue
   interruptQueue(INIT, 0, processCount);
   
   //EVENT LOG: All Processes init in NEW
   accessTimer( LAP_TIMER, timeString );
   eventData = generateEventData( OS, AllProcNEW, timeString, mdData, process );
   eventLogger( eventData, configDataPtr, listCurrentPtr );
   
   //EVENT LOG: All Processes init in READY
   accessTimer( LAP_TIMER, timeString );
   eventData = generateEventData( OS, AllProcREADY, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess] );
   eventLogger( eventData, configDataPtr, listCurrentPtr );
   
   
   //Initialize in READY state
   initInReady( pcbArray, processCount );
   
   
   //Calculate each of the remaining times on each Process
   calcRemainingTimes( pcbArray, configDataPtr, processCount );
   
   //init threadManager
   threadManager(tINIT, NULL);
   
   //TESTING QUEUE
   printf("\n\nDEBUG----->IQ, isEmpty %d", interruptQueue(ISEMPTY, 9, processCount));
   printf("\n\nDEBUG----->PUSH %d", interruptQueue(ENQUEUE, 2, processCount));
   printf("\n\nDEBUG----->IQ, isEmpty %d", interruptQueue(ISEMPTY, 9, processCount));
   printf("\n\nDEBUG----->PUSH %d", interruptQueue(ENQUEUE, 5, processCount));
   printf("\n\nDEBUG----->POP %d", interruptQueue(DEQUEUE, 9, processCount));
   printf("\n\nDEBUG----->POP %d", interruptQueue(DEQUEUE, 9, processCount));
   printf("\n\nDEBUG----->POP %d", interruptQueue(DEQUEUE, 9, processCount));
   printf("\n\nDEBUG----->IQ, isEmpty %d\n\n\\n\n", interruptQueue(ISEMPTY, 9, processCount));
   
   
   //MAIN SIMULATOR LOOP -- Loop while we have Processes not in EXIT
   while( processingFlag == True )
   {
      printf("\nITERATION...");
      //Select process, utilizing cpuScheduler
      oldScheduledProcess = scheduledProcess;
      scheduledProcess = cpuScheduler( pcbArray, processCount, configDataPtr );
      
      //check if we are idle, and we dont have any interrupts
      if( idleFlag == True && interruptQueue( ISEMPTY, 0, 0 ) )
      {
         printf("\nIDLE>>>");
         pthread_t runningThread = threadManager( tPOP, NULL );
         if(runningThread == NULL)
         {
            printf("\nP...........NULL");
         }
         else
         {
            pthread_join( runningThread, NULL);
            //free((void*)runningThread);
         }
         //if we are still idling, and no interrupts, skip over till we get one
         //continue;
      }
      
      //check for interupts, if our queue is not empty, then we need to process
      if( !interruptQueue( ISEMPTY, 0, 0 ) )
      {
         interruptedPid = -1;
         interruptedPid = interruptQueue( DEQUEUE, 0, 0 );
         if( interruptedPid < 0 )
         {
            //printf("\nDEBUG----> IS EMPTY QUEUE EMPTY");
         }
         else
         {
         printf("\n\n\ninterruptedPID: %d", interruptedPid);
         pcbArray[interruptedPid].pState = READY;
         
         //EVENT LOG: Interrupt Called by Process interruptedPid
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( OS, Interrupt, timeString,
                                 pcbArray[interruptedPid].programCounter, 
                                             &pcbArray[interruptedPid] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
         }
      }
      
      //check if all processes are blocked, if so SYS IDLE
      int blockCount = 0;
      idleFlag = False;
      for( indexI = 0; indexI < processCount; indexI++ )
      {
         //using if greater than equal to BLOCKED accounts for blocked and exit
         if( pcbArray[indexI].pState >= BLOCKED )
         {
            blockCount++;
         }
         
         if( blockCount == processCount )
         {
            //EVENT LOG: BEGIN SYSTEM IDLE
            accessTimer( LAP_TIMER, timeString );
            eventData = generateEventData( OS, SystemIdle, timeString, NULL, 
                                                                        NULL);
            eventLogger( eventData, configDataPtr, listCurrentPtr );
            idleFlag = True;
         }
      }
      
      //ensure our scheduler picked a valid process
      if( scheduledProcess < 0 )
      {
         continue;
      }
      
      //check if our selected process is new, otherwise ignore.
      if( oldScheduledProcess != scheduledProcess || isFirstRun == True )
      {
         isFirstRun = False;
         
         //EVENT LOG: ProcessSelected with Remaining time: 
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( OS, ProcSelected, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
      }
      
      //Set Process in RUNNING // check if already RUNNING, if so ignore.
      if( pcbArray[scheduledProcess].pState != RUNNING )
      {
         pcbArray[scheduledProcess].pState = RUNNING;
         
         //EVENT LOG: select process and set in RUNNING
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( OS, ProcSetIn, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
      }
      
      //grab our PC, and send to Operation runner to handle run types
         //IE, RUN, I/O, or MEM Operations
      currentProgramCounter = pcbArray[scheduledProcess].programCounter;
      segFaultFlag = operationRunner( scheduledProcess, currentProgramCounter, 
                                       configDataPtr, pcbArray, listCurrentPtr, 
                                                mmuCurrentPtr, mmuHeadPtr );
      
      if( segFaultFlag == 1)
      {
         //EVENT LOG: segfault, process experiences
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( OS, SegFault, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
         
         //our OS will detect the seg fault flag in next op, and handle exiting
      }
      
      
      //CHECK FOR FINISH, since we started our program Counter with an offset 
         //from start, this will only ever be A(end)0; or Segfault exit.
      if( currentProgramCounter->opLtr == 'A' || segFaultFlag == 1 )
      {
         pcbArray[scheduledProcess].pState = EXIT;
            
         //EVENT LOG: end process and set in EXIT
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( OS, ProcEnd, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
      }
      
      //After each Process is checked, move the program counter, as long as the 
         //process is not in EXIT 
      printf("\nPROCESS STATE: %d", pcbArray[scheduledProcess].pState);
      if( pcbArray[scheduledProcess].pState < EXIT )
      {
         printf("\nENTERED PROCESS STATE: %d", pcbArray[scheduledProcess].pState);
         pcbArray[scheduledProcess].programCounter = 
            pcbArray[scheduledProcess].programCounter->next;
      }
      
      processingFlag = False;
      //Check if all processing are in EXIT, if so we stop simulating
      for( indexI = 0; indexI < processCount; indexI++ )
      {
         if( pcbArray[indexI].pState != EXIT )
         {
            processingFlag = True;
         }
      }
   }
   
   
   //EVENT LOG: System Stop
   accessTimer( LAP_TIMER, timeString );
   eventData = generateEventData( OS, SystemStop, timeString,
      pcbArray[scheduledProcess].programCounter, &pcbArray[scheduledProcess] );
   eventLogger( eventData, configDataPtr, listCurrentPtr );
   
   
   //EXIT with normal operation
   printf("\nEnd Simulation - Complete\n");
   printf( "==========================\n" );
   
   //Check if we need to save of completeLog to a file
   configCodeToString( configDataPtr->logToCode, checkIfFile );
   if( compareString( checkIfFile, "Both" ) == STR_EQ  
            || compareString( checkIfFile, "File" ) == STR_EQ)
   {
      logToFile( listHeadPtr, configDataPtr );
   }
   
   //clear out our logLinkedList, and free temporary memory for newNodePtr
   listHeadPtr = clearLinkedList( listHeadPtr );
   free( newNodePtr );
   
   //clear our our MMU linkedList, and free temp memory
   mmuHeadPtr = clearMMU( mmuHeadPtr );
   free( mmuNewPtr );
   
   //clear interrupt queue
   interruptQueue(DESTRUCT, 0, processCount);
   
   //clear threadmanager
   threadManager(tDESTRUCT, NULL);
   
   //safe return from simulationRunner
   return 0;
}

/*
Function name: cpuScheduler
Algorithm: in this version, initial configuration for FCFS-N only
            the scheduler takes in a list of arrays, and selects the next
            process to be run, based on its scheduling strategy
Precondition: filled pcbArray, and a count of the processes
Postcondition: returns the pId of the next process to be run
Exceptions: none
Notes: IMPORTANT: I utilized a scheduler seperate from the interrupt features
                  so we will not see those here, see operationRunner below for
                  that functionality
*/
int cpuScheduler(PCB* pcbArray, int processCount, ConfigDataType* configDataPtr)
{
   int scheduledPid = -1;
   int indexI = 0;
   int shortestJobFound = 999999999;
   int scheduleCode = configDataPtr->cpuSchedCode;
   
   //run until we find a pid that is scheduled
   while( indexI < processCount )
   {  
      //only work on processess that are ready or running
      if( pcbArray[indexI].pState == READY 
            || pcbArray[indexI].pState == RUNNING )
      {
         //FCFS-N implementation
         if( scheduleCode == CPU_SCHED_FCFS_N_CODE && scheduledPid == -1 )
         {
            scheduledPid = pcbArray[indexI].pId;
         }
         
         //SJF-N implementation
         else if( scheduleCode == CPU_SCHED_SJF_N_CODE )
         {
            if( pcbArray[indexI].remainingTimeMs < shortestJobFound )
            {
               scheduledPid = pcbArray[indexI].pId;
               shortestJobFound = pcbArray[indexI].remainingTimeMs;
            }
         }
         
         //default to FCFS-N // still need to ensure we hadnt selected a job yet
         else if ( scheduledPid == -1 )
         {
            scheduledPid = pcbArray[indexI].pId;
         }
      }
      
      indexI++;
   }
   
   return scheduledPid;
}



/*
Function name: operationRunner
Algorithm: utilizes process information to "perform" the operations of a process
            uniquely operates based on operation type. Sends off to the
            eventLogger at each step.
Precondition: schedulePid, and process information, linked list for sending to
               logger, and linked list of the MMU
Postcondition: returns when operation has been completed
Exceptions: none
Notes: none
*/
int operationRunner( int scheduledProcess,OpCodeType* programCounter,
                                          ConfigDataType* configDataPtr, 
                                             PCB* pcbArray,
                                                LogLinkedList* listCurrentPtr,
                                                   MMU* mmuCurrentPtr,
                                                      MMU* mmuHeadPtr )
{
   //initializations
   char timeString[MAX_STR_LEN];
   char memoryValueStr[MAX_STR_LEN];
   int intConvert = 0;
   int ssCount = 2;
   int ssInt = 0;
   int bbbInt = 0;
   int aaaInt = 0;
   int timeToWaitMs = 0;
   int segFaultFlag = 0;
   EventData eventData;
   
   //pthread_setconcurrency(4);
   
   //pthread initializations
   pthread_t thread1;
   
   //RUN OPERATIONS
   if( programCounter->opLtr == 'P' )
   { 
      //EVENT LOG: run start
      accessTimer( LAP_TIMER, timeString );
      eventData = generateEventData( Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
      eventLogger( eventData, configDataPtr, listCurrentPtr );
      
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer( timeToWaitMs );
   
      //EVENT LOG: run end
      accessTimer( LAP_TIMER, timeString );
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
      eventLogger( eventData, configDataPtr, listCurrentPtr );
   }
   
   //MEMORY OPERATIONS
   else if( programCounter->opLtr == 'M' )
   {   
      //parse out our memory val in SSBBBAAA format, then break into 3 seperate
      //integers to pass along to MMU functions
   
      //acommodate for leading zero in SS, if so we modify the ssCount
      if( programCounter->opValue < 10000000 )
         ssCount = 1;
      
      //throw opValue into buffer and convert into string then loop through
      //to parse out SS, BBB, and AAA ints.
      sprintf( memoryValueStr, "%d", programCounter->opValue);
      for( int index = 0; memoryValueStr[index] != '\0'; index++)
      {
         //formula to convert chars into ints based on ASCII value - char 0
         intConvert = intConvert * 10 + ( memoryValueStr[index] - '0' );
         
         //find our SS / BBB / AAA offset ends with our count and +3 / +6
         if( index+1 == ssCount )
         {
            ssInt = intConvert;
            intConvert = 0;
         }
         
         if( index+1 == ssCount + 3 )
         {
            bbbInt = intConvert;
            intConvert = 0;
         }
         
         if( index+1 == ssCount + 6 )
         {
            aaaInt = intConvert;
            intConvert = 0;
         }
   
      }
      
      //fill our data that we just calced
      MMU mmuData = fillMMU( scheduledProcess, ssInt, bbbInt, aaaInt );
      pcbArray[scheduledProcess].mmuInfo = mmuData;
      
      //Check memory action type (allocate, access) and call to appropriate func 
      if( compareString( programCounter->opName, "allocate" ) == STR_EQ )
      {
         //EVENT LOG: mem attempt to allocate
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( Process, MMUAllocAttempt, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
         
         segFaultFlag = mmuAllocate( configDataPtr, mmuCurrentPtr, mmuHeadPtr, 
                                                                     mmuData );
         if( segFaultFlag == 0 )
         {
            //EVENT LOG: mem allocate success
            accessTimer( LAP_TIMER, timeString );
            eventData = generateEventData( Process, MMUAllocSuccess, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
            eventLogger( eventData, configDataPtr, listCurrentPtr );
         }
         else
         {
            //EVENT LOG: mem allocate failed
            accessTimer( LAP_TIMER, timeString );
            eventData = generateEventData( Process, MMUAllocFailed, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
            eventLogger( eventData, configDataPtr, listCurrentPtr );
            
            //exit operation runner returning segfault
            return 1;
         }
         
      }
      //else, runs when we need to access
      else
      {
         //EVENT LOG: mem attempt to access
         accessTimer( LAP_TIMER, timeString );
         eventData = generateEventData( Process, MMUAccessAttempt, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
         eventLogger( eventData, configDataPtr, listCurrentPtr );
         
         segFaultFlag = mmuAccess( configDataPtr, mmuCurrentPtr, mmuHeadPtr, 
                                                                     mmuData );
         
         if( segFaultFlag == 0 )
         {
            //EVENT LOG: mem access success
            accessTimer( LAP_TIMER, timeString );
            eventData = generateEventData( Process, MMUAccessSuccess, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
            eventLogger( eventData, configDataPtr, listCurrentPtr );
         }
         else
         {
            //EVENT LOG: mem acess failed
            accessTimer( LAP_TIMER, timeString );
            eventData = generateEventData( Process, MMUAccessFailed, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
            eventLogger( eventData, configDataPtr, listCurrentPtr );
            
            //exit operation runner returning segfault
            return 1;
         }
      }
   }
      
   //I/O OPERATIONS
   else if( programCounter->opLtr == 'I'
            || programCounter->opLtr == 'O')
   {
      //EVENT LOG: io start
      accessTimer( LAP_TIMER, timeString );
      eventData = generateEventData( Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
      eventLogger( eventData, configDataPtr, listCurrentPtr );
         
      //Wait out our time, utilizing pthreads
      timeToWaitMs = programCounter->opValue * configDataPtr->ioCycleRate;
      
      //allocate space for our ThreadInput dataStructure to send with create
      ThreadInput* threadInput = (ThreadInput*) malloc( sizeof(ThreadInput) );
      threadInput->timeToWait = timeToWaitMs;
      threadInput->pId = scheduledProcess;
      
      printf("THREAD CREATED , timeToWaitMS: %d", timeToWaitMs);
      
      //ensure we send the memoryLocation of timeToWaitMs as a void* cast
      threadManager( tPUSH, threadInput );
      //pthread_create( &thread1, NULL, threadRunTimer, (void*) threadInput );
      //pthread_detach(thread1);
      
      //place our process in blocked, interruptQueue pop will handle unblocking
      pcbArray[scheduledProcess].pState = BLOCKED;
      
      //TEST REMOVE OF PJOINS
      //pthread_join( thread1, NULL);
   
      //EVENT LOG: Process set in BLOCKED
      accessTimer( LAP_TIMER, timeString );
      eventData = generateEventData( OS, ProcBlocked, timeString,
                                 programCounter, &pcbArray[scheduledProcess] );
      eventLogger( eventData, configDataPtr, listCurrentPtr );
   
      //EVENT LOG: io end
      //accessTimer( LAP_TIMER, timeString );
      //eventData = generateEventData( Process, ProcOpEnd, timeString,
                                 //programCounter, &pcbArray[scheduledProcess] );
      //eventLogger( eventData, configDataPtr, listCurrentPtr );
   }
   
   //clean exit of operation runner, ie no segfaults.
   return 0;
}


/*
Function name: threadRunTimer
Algorithm: allows operationRunner to send I/O on POSIX threads
Precondition: valid pthread create call
Postcondition: returns NULL to meet pthread_create contract
Exceptions: none
Notes: none
*/
void* threadRunTimer( void* threadInput )
{
   //ensure we grab our sent integer pointer.
   int timeToWaitMs = (int) (((ThreadInput*)threadInput) -> timeToWait);
   int processId = (int) (((ThreadInput*)threadInput) -> pId);
   
   printf("\n\nTHREAD::   :::TIME TO WAIT BEFORE : %d", timeToWaitMs);
   
   //deference the pointer and send off the run timer
   runTimer( timeToWaitMs );
   
   printf("\n\nTHREAD::   :::TIME TO WAIT AFTER : %d", timeToWaitMs);
   
   printf("\n\nTHREAD ENDED, add to interrupt queue");
   interruptQueue(ENQUEUE, processId, -1);
   
   //free our allocated space
   free(threadInput);
   
   return NULL;
}

/*
Function name: threadManager
Algorithm:     
Precondition:  
Postcondition: 
Exceptions: none
Notes: none
*/
pthread_t threadManager(ThreadAction action, ThreadInput* threadInput)
{
   static pthread_t* threads;
   static int currentOffset;
   int maxCap = 100;
   pthread_t toReturn;
   
   pthread_t newThread;
   
   //check our requested action againts our enumerator possible values
   switch( action )
   {
      //initialization of queue, allocat space and point to front of it
      case tINIT:
         threads = (pthread_t*) malloc( sizeof(pthread_t) * maxCap );
         currentOffset = 0;
         break;
      
      //pushing an element into our queue, place pId inside and move offset
      case tPUSH:
         if( currentOffset < maxCap )
         {  
            pthread_create( &newThread, NULL, threadRunTimer, (void*) threadInput );
            //pthread_detach(newThread);
            *(threads + currentOffset) = newThread;
            currentOffset++;
         }
         else
         {
            printf("\n\nDEBUG-------> QUEUE FULL");
         }
         break;
      
      //popping an element off the queue
      case tPOP:
         //ensure we dont overshoot our allocated memory
         if( currentOffset == maxCap )
         {
            currentOffset--;
         }

         if( currentOffset >= 0 && currentOffset < maxCap )
         {  
            toReturn = *(threads + currentOffset);
            
            //check that we arent at left end of queue, if not then decrement.
            if( currentOffset != 0 )
            {
               currentOffset--;
            }
            return toReturn;
         }
         else
         {
            printf("DEBUG------> INTERRUPT QUEUE EMPTY");
         }
         break;
         
      //"Destruct" our intterupt array, free memory   
      case tDESTRUCT:
         free( threads );
         break;
         
      case tISEMPTY:
         //if we are at beginning and its pId is -1, we are empty
         if( currentOffset == 0 && *(threads + currentOffset))
         {
            //return that we ARE empty
            return 1;
         }
         else
         {
            //return that we are NOT empty
            return 0;
         }
         break;
   }
   
   //safe exit
   return newThread;
}

/*
Function name: interruptQueue
Algorithm:     
Precondition:  
Postcondition: 
Exceptions: none
Notes: none
*/
int interruptQueue(InterruptAction action, int pId, int processNum)
{
   //static data to persist across runtime, allows us not to have to pass ptrs.
   static int* queue;
   static int size, headOffset, rearOffset, maxCap;
   int toReturn = -1;
   
   //check our requested action againts our enumerator possible values
   switch( action )
   {
      //initialization of queue, allocat space and point to front of it
      case INIT:
         maxCap = processNum;
         queue = (int*) malloc( sizeof(int) * maxCap );
         headOffset = 0;
         size = 0;
         rearOffset = maxCap - 1;
         break;
      
      //adding an element into our queue, place pId inside and move offset
      case ENQUEUE:
         if( interruptQueue( ISFULL, 0, 0 ) )
         {
            return -1;
         }
         else
         {
            rearOffset = (rearOffset + 1) % maxCap; 
            *(queue + rearOffset) = pId;
            size++;
         }
         break;
      
      //removing an element off the queue
      case DEQUEUE:
         if( interruptQueue( ISEMPTY, 0, 0 ) )
         {
            return -1;
         }
         else
         {
            toReturn = *(queue + headOffset);
            headOffset = (headOffset + 1) % maxCap;
            size--;
            
            return toReturn;
         }
         break;
         break;
      
      //return element at the headOffset
      case HEAD:
         if( interruptQueue( ISEMPTY, 0, 0 ))
         {
            return -1;
         }
         else
         {
            return *(queue + headOffset);
         }
         break;
      
      //return element at the rearOffset
      case REAR:
         if( interruptQueue( ISEMPTY, 0, 0 ))
         {
            return -1;
         }
         else
         {
            return *(queue + rearOffset);
         }
         break;
         
      //"Destruct" our intterupt array, free memory   
      case DESTRUCT:
         free( queue );
         break;
      
      //returns true for size 0 and false otherwise
      case ISEMPTY:
         return ( size == 0 );
         break;
      
      //return true for size == maxCap, false otherwise
      case ISFULL:
         return ( size == maxCap );
         break;
   }
   
   //safe exit
   return 0;
}

/*
Function name: findProcessCount
Algorithm:     utilizes metaData to find the number of processes within
Precondition:  correctly configured metaData and ptr to it
Postcondition: returns the number of processes in metadata
Exceptions: none
Notes: none
*/
int findProcessCount( OpCodeType* loopMetaDataPtr, OpCodeType* mdData )
{
   int processCount = 0;
   
   //start at metaData head
   loopMetaDataPtr = mdData;
   
   //loop through our MetaData searching for op A letters to count proccesses
   while( loopMetaDataPtr != NULL )
   {
      //if our OP letter is A, and the opName is start, count the process
      if( loopMetaDataPtr->opLtr == 'A'
        && compareString( loopMetaDataPtr->opName, "start" ) == STR_EQ )
      {
         processCount += 1;
      }
      
      loopMetaDataPtr = loopMetaDataPtr->next;
   }
   
   //reset our loopMetaDataPtr back to start
   loopMetaDataPtr = mdData;
   
   return processCount;
}

/*
Function name: createPCBs
Algorithm:     based on the process count fills in process data in regards to
               metaData info. (Fills pcbArray with processes)
Precondition:  correctly configured metaData and ptr to it
Postcondition: modifies pcbArray to contain all program processes
Exceptions: none
Notes: none
*/
void createPCBs( PCB* pcbArray, OpCodeType* loopMetaDataPtr, int processCount )
{
   int indexI = 0;
   Boolean foundProcessFlag = False;
   
   //Now with that count, create PCB structs for each process
   //And initilize each in NEW state / attach their Program Counter
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      pcbArray[indexI].pState = NEW;
      pcbArray[indexI].pId = indexI;
      
      foundProcessFlag = False;
      
      //find ProgramCounter ptr start
      while( foundProcessFlag == False )
      {
         if( loopMetaDataPtr->opLtr == 'A'
            && compareString( loopMetaDataPtr->opName, "start" ) == STR_EQ )
         {
            foundProcessFlag = True;
         }
         
         loopMetaDataPtr = loopMetaDataPtr->next;
      }
      
      pcbArray[indexI].programCounter = loopMetaDataPtr;
   }
}

/*
Function name: initInReady
Algorithm:     simple function to iterate across processes and set them in Ready
Precondition:  correctly filled pcbArray
Postcondition: modifies state of processes in pcbArray to ready
Exceptions: none
Notes: none
*/
void initInReady( PCB* pcbArray, int processCount )
{
   int indexI = 0;
   
   //iterate across processes and set their states to ready
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      pcbArray[indexI].pState = READY;
   }
}

/*
Function name: calcRemainingTimes
Algorithm:     iterate through processes and their list of actions, tallying up
               the remaining time totals.
Precondition:  correctly filled pcbArray, configPtr
Postcondition: modifies time remaining values of processes to reflect their
               actions total tally.
Exceptions: none
Notes: none
*/
void calcRemainingTimes( PCB* pcbArray, ConfigDataType* configDataPtr, 
                                                            int processCount )
{
   int indexI = 0;
   Boolean foundProcessFlag = False;
   OpCodeType* currentProgramCounter;
   
   //iterate acrross each of the processes
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      currentProgramCounter = pcbArray[indexI].programCounter;
      
      foundProcessFlag = False;
      pcbArray[indexI].remainingTimeMs = 0;
      
      //loop until we havent found any processes and our program counter is null
      while( foundProcessFlag == False && currentProgramCounter != NULL )
      {
         if( currentProgramCounter->opLtr == 'A'
            && compareString( currentProgramCounter->opName, "end" ) == STR_EQ )
         {
            foundProcessFlag = True;
         }
         
         //ensure we pass over Memory, as SSBBBAAA format would cause issues
         else if ( currentProgramCounter->opLtr != 'M' )
         {
            //check which op type cycle rate to multiply by
            if( currentProgramCounter->opLtr == 'P' )
            {
               pcbArray[indexI].remainingTimeMs += 
                                    (currentProgramCounter->opValue 
                                                * configDataPtr->procCycleRate);
            }
            else
            {
               pcbArray[indexI].remainingTimeMs += 
                                    (currentProgramCounter->opValue 
                                                * configDataPtr->ioCycleRate);
            }
         }
         
         currentProgramCounter = currentProgramCounter->next;
      }
   }
}



