//Program Information///////////////////////////////////////////////////////////
/*
 * @file SimRunner.c
 *
 *
 * @version 2.00
 *          Kristopher Moore (19 Feburary 2019)
 *          Initial Sim02 Build.
 */
 
#include <stdio.h>
#include <pthread.h>
#include "ConfigAccess.h"
#include "MetaDataAccess.h"
#include "SimRunner.h"
#include "simtimer.h"
#include "StringUtils.h"
 
int simulationRunner(ConfigDataType* configDataPtr, OpCodeType* mdData)
{
   //initializations
   char timeString[MAX_STR_LEN];
   char completeLog[MAX_STR_LEN];
   LogLinkedList* newNodePtr;
   LogLinkedList* listHeadPtr = NULL;
   LogLinkedList* listCurrentPtr = NULL;
   char checkIfFile[STD_STR_LEN];
   OpCodeType* loopMetaDataPtr;
   OpCodeType* currentProgramCounter;
   PCB* process = NULL;
   int processCount = 0;
   int indexI = 0;
   int scheduledProcess = 0;
   int oldScheduledProcess = 0;
   Boolean processingFlag = True;
   Boolean foundProcessFlag = False;
   Boolean isFirstRun = True;
   EventData eventData;
   
   //buffer in a value for completeLog, to avoid unintialized access
   copyString( completeLog, " " );
   
   
                              //TESTING CODE:////////////////////////////////////////////////////
                              newNodePtr = ( LogLinkedList * ) malloc( sizeof( LogLinkedList ) );
                              copyString(newNodePtr->logLine, " ");
                              
                              listCurrentPtr = addNodeLL( listCurrentPtr, newNodePtr );
                              listHeadPtr = listCurrentPtr;
                              
                              //addNodeLL( listCurrentPtr, newNodePtr );
                              
                              //while( listCurrentPtr != NULL )
                              //{
                                 copyString(listCurrentPtr->logLine, " ");
                                 concatenateString(listCurrentPtr->logLine, "KRISSSSSSSSS");
                                 printf("\n\t\t%s\n", listCurrentPtr->logLine );
                                 //listCurrentPtr = listCurrentPtr->next;
                              //}
                               //frees moved to bot of this function
                              
                              ///////////////////////////////////////////////////////////////////
   
   //Start Event Logging
   printf( "==========================\n" );
   printf("Begin Simulation\n\n");
   
   
   //EVENT LOG: System Start
   accessTimer(ZERO_TIMER, timeString);
   eventData = generateEventData(OS, SystemStart, timeString, mdData, process);
   eventLogger(eventData, configDataPtr, listCurrentPtr);
   
   
   //EVENT LOG: Create PCB'S
   accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, CreatePCBs, timeString, mdData, process);
   eventLogger(eventData, configDataPtr, listCurrentPtr);
   
   
   //Create our PCB's, start by counting how many we need(processes in MetaData)
   loopMetaDataPtr = mdData;
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
   
   
   
   //EVENT LOG: All Processes init in NEW
   accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, AllProcNEW, timeString, mdData, process);
   eventLogger(eventData, configDataPtr, listCurrentPtr);
   
   
   
   //Now with that count, create PCB structs for each process
   //And initilize each in NEW state / attach their Program Counter
   PCB pcbArray[processCount];
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
   
   
   
   //EVENT LOG: All Processes init in READY
   accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, AllProcREADY, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
   eventLogger(eventData, configDataPtr, listCurrentPtr);
   
   
   //Initialize in READY state
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      pcbArray[indexI].pState = READY;
   }
   
   
   //Calculate each of the remaining times on each Process
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      currentProgramCounter = pcbArray[indexI].programCounter;
      
      foundProcessFlag = False;
      pcbArray[indexI].remainingTimeMs = 0;
      while( foundProcessFlag == False && currentProgramCounter != NULL )
      {
         if( currentProgramCounter->opLtr == 'A'
            && compareString( currentProgramCounter->opName, "end" ) == STR_EQ )
         {
            foundProcessFlag = True;
         }
         else
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
   
   
   //Main Simulator Loop -- Loop while we have Processes not in EXIT
   while( processingFlag == True )
   {
      //Select process, utilizing cpuScheduler
      oldScheduledProcess = scheduledProcess;
      scheduledProcess = cpuScheduler( pcbArray, processCount );
      
      //check if our selected process is new, otherwise ignore.
      if(oldScheduledProcess != scheduledProcess || isFirstRun == True)
      {
         isFirstRun = False;
         
         //EVENT LOG: ProcessSelected with Remaining time: 
         accessTimer(LAP_TIMER, timeString);
         eventData = generateEventData(OS, ProcSelected, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
         eventLogger(eventData, configDataPtr, listCurrentPtr);
      }
      
      //Set Process in RUNNING // check if already RUNNING, if so ignore.
      if( pcbArray[scheduledProcess].pState != RUNNING )
      {
         pcbArray[scheduledProcess].pState = RUNNING;
         
         //EVENT LOG: select process and set in RUNNING
         accessTimer(LAP_TIMER, timeString);
         eventData = generateEventData(OS, ProcSetIn, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
         eventLogger(eventData, configDataPtr, listCurrentPtr);
      }
      
      //grab our PC, and send to Operation runner to handle run types
         //IE, RUN, I/O, or MEM Operations
      currentProgramCounter = pcbArray[scheduledProcess].programCounter;
      operationRunner( scheduledProcess, currentProgramCounter, configDataPtr,
                                                            pcbArray, 
                                                               listCurrentPtr );
      
      //CHECK FOR FINISH, since we started our program Counter with an offset 
         //from start, this will only ever be A(end)0;
      if( currentProgramCounter->opLtr == 'A' )
      {
         pcbArray[scheduledProcess].pState = EXIT;
            
         //EVENT LOG: end process and set in EXIT
         accessTimer(LAP_TIMER, timeString);
         eventData = generateEventData(OS, ProcEnd, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
         eventLogger(eventData, configDataPtr, listCurrentPtr);
      }
      
      //After each Process is checked, move the program counter, as long as the 
         //process is not in EXIT
      if( pcbArray[scheduledProcess].pState != EXIT )
      {
         pcbArray[scheduledProcess].programCounter = 
            pcbArray[scheduledProcess].programCounter->next;
      }
      
      //Check if all processing are in EXIT, if so we stop simulating
      for( indexI = 0; indexI < processCount; indexI++ )
      {
         processingFlag = False;
         if( pcbArray[indexI].pState != EXIT )
         {
            processingFlag = True;
         }
      }
   }
   
   
   //EVENT LOG: System Stop
   accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, SystemStop, timeString,
      pcbArray[scheduledProcess].programCounter, &pcbArray[scheduledProcess]);
   eventLogger(eventData, configDataPtr, listCurrentPtr);
   
   
   //EXIT with normal operation
   printf("\nEnd Simulation - Complete\n");
   printf( "==========================\n" );
   
   //Check if we need to save of completeLog to a file
   configCodeToString( configDataPtr->logToCode, checkIfFile );
   if( compareString( checkIfFile, "Both" ) == STR_EQ  
            || compareString( checkIfFile, "File" ) == STR_EQ)
   {
      logToFile(listCurrentPtr);
   }
   
   //TESTING PRINT://////////////////////////////////////////
   listCurrentPtr = listHeadPtr;
   while( listCurrentPtr != NULL )
   {
      printf("\n..%s.. INDEX: %d\n", listCurrentPtr->logLine, indexI++);
      listCurrentPtr = listCurrentPtr->next;
   }
   
   //clear out our logLinkedList
   listHeadPtr = clearLinkedList( listHeadPtr );
   free( newNodePtr );
   
   return 0;
}

int cpuScheduler(PCB* pcbArray, int processCount )
{
   int scheduledPid = -1;
   int indexI = 0;
   
   //run until we find a pid that is scheduled
   while( scheduledPid == -1 )
   {  
      //FCFS implementation
      if( pcbArray[indexI].pState == READY 
            || pcbArray[indexI].pState == RUNNING )
      {
         scheduledPid = pcbArray[indexI].pId;
      }
      
      indexI++;
   }
   
   return scheduledPid;
}



void operationRunner(int scheduledProcess,OpCodeType* programCounter,
                                          ConfigDataType* configDataPtr, 
                                                PCB* pcbArray,
                                                  LogLinkedList* listCurrentPtr)
{
   //initializations
   char timeString[MAX_STR_LEN];
   int timeToWaitMs = 0;
   EventData eventData;
   
   //pthread initializations
   pthread_t thread1;
   
   //RUN OPERATIONS
   if( programCounter->opLtr == 'P' )
   { 
      //EVENT LOG: run start
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
      
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer(timeToWaitMs);
   
      //EVENT LOG: run end
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
   }
   
   //MEMORY OPERATIONS
   else if( programCounter->opLtr == 'M' )
   {
      //EVENT LOG: mem start
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
         
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer(timeToWaitMs);
   
      //EVENT LOG: mem end
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
   }
      
   //I/O OPERATIONS
   else if( programCounter->opLtr == 'I'
            || programCounter->opLtr == 'O')
   {
      //EVENT LOG: io start
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
         
      //Wait out our time, utilizing pthreads
      timeToWaitMs = programCounter->opValue * configDataPtr->ioCycleRate;
      
      //ensure we send the memoryLocation of timeToWaitMs as a void* cast
      pthread_create( &thread1, NULL, threadRunTimer, (void*) &timeToWaitMs );
      pthread_join( thread1, NULL);
   
      //EVENT LOG: io end
      accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData, configDataPtr, listCurrentPtr);
   }
}

void eventLogger(EventData eventData, ConfigDataType* configDataPtr, 
                                                  LogLinkedList* listCurrentPtr)
{
   //declare strings holders
   LogLinkedList* newNodePtr = NULL;
   LogLinkedList* oldNodePtr = NULL;
   char finalLogStr[MAX_STR_LEN];
   char eventStr[MAX_STR_LEN];
   char logCodeStr[MAX_STR_LEN];
   char logCodeExtend[MAX_STR_LEN];
   char monitorString[STD_STR_LEN];
   
   //full out any garbage
   copyString( finalLogStr, " " );
   copyString( eventStr, " " );
   copyString( logCodeStr, " " );
   copyString( logCodeExtend, " " );
   
   
   switch( eventData.eventType )
   {
      case OS:
         concatenateString( eventStr, "OS: " );
         break;
         
      case Process:
         concatenateString( eventStr, "Process: " );
         break;
   }
   
   switch( eventData.logCode )
   {
      case BeginSim:
         concatenateString( logCodeStr, 
                           "==========================\nBegin Simulation\n" );
         break;
      
      case SystemStart:
         concatenateString( logCodeStr, "System Start\n" );
         break;
         
      case CreatePCBs:
         concatenateString( logCodeStr, "Create Process Control Blocks\n" );
         break;
         
      case AllProcNEW:
         concatenateString( logCodeStr, 
                              "All processes initialized in NEW state\n" );
         break;
         
      case AllProcREADY:
         concatenateString( logCodeStr, 
                              "All processes initialized in READY state\n" );
         break;
      
      case ProcOpStart:
         sprintf( logCodeStr, "%d %s operation start\n",
                              eventData.pId, eventData.opStartOrEnd );
         break;
      
      case ProcOpEnd:
         sprintf( logCodeStr, "%d %s operation end\n",
                              eventData.pId, eventData.opStartOrEnd );
         break;
      
      case ProcSelected:
         sprintf( logCodeStr, "Process %d selected with %d ms remaining\n",
                                       eventData.pId, eventData.remainingTime);
         break;
         
      case ProcSetIn:
         sprintf( logCodeStr, "Process %d set in RUNNING state\n", 
                                                               eventData.pId );
         break;
         
      case ProcEnd:
         sprintf( logCodeStr, "Process %d ended and set in EXIT state\n", 
                                                               eventData.pId );
         break;
         
      case SystemStop:
         concatenateString( logCodeStr, "System Stop\n" );
         break;
         
      case EndSim:
         concatenateString( logCodeStr, 
                              "==========================\n End Simulation" );
         break;
      
      default:
         break;
      
   }
   
   //concat the strings
   concatenateString( finalLogStr, "\t" );
   concatenateString( finalLogStr, eventData.timeToPrint );
   concatenateString( finalLogStr, "," );
   concatenateString( finalLogStr, eventStr );
   concatenateString( finalLogStr, logCodeStr );
   concatenateString( finalLogStr, logCodeExtend );
   
   //convert our code into a string, then check how we should be eventLogging
   configCodeToString( configDataPtr->logToCode, monitorString );
   if( compareString( monitorString, "Both" ) == STR_EQ  )
   {
      printf( "%s", finalLogStr );
      
      //allocate space, and ensure we intialize it
      newNodePtr = ( LogLinkedList * ) malloc( sizeof( LogLinkedList ) );
      copyString(newNodePtr->logLine, " ");
     
     
      listCurrentPtr = addNodeLL( listCurrentPtr, newNodePtr );
      
      listCurrentPtr = listCurrentPtr->next;
      
      //copyString(listCurrentPtr->logLine, " ");
      //concatenateString(listCurrentPtr->logLine, finalLogStr);
      
      //Get last node position, so we can set it
      while( listCurrentPtr != NULL )
      {
         printf("\n..%s.. ;;;;;;;;\n", listCurrentPtr->logLine);
         oldNodePtr = listCurrentPtr;
         listCurrentPtr = listCurrentPtr->next;
      
      }
      copyString(oldNodePtr->logLine, " ");
      concatenateString(oldNodePtr->logLine, finalLogStr);
                        
      free( newNodePtr );
      
   }
   else if( compareString( monitorString, "File" ) == STR_EQ  )
   {
      //concatenateString( completeLog, finalLogStr );
   }
   else if( compareString( monitorString, "Monitor" ) == STR_EQ  )
   {
      printf("%s", finalLogStr);
   }
   
}

EventData generateEventData( EventType eventType, LogCode logCode, 
                  char* timeString, OpCodeType* programCounter, PCB* process )
{
   EventData eventData;
   
   //load parameters into struct
   eventData.eventType = eventType;
   eventData.logCode = logCode;
   eventData.timeToPrint = timeString;
   eventData.opStartOrEnd = "";
   eventData.pStateStr = "";
   eventData.remainingTime = 0;
   eventData.pId = -1;
   
   
   //return if our pointer isnt valid, the following calls rely on the process
   if( process == NULL  || programCounter == NULL )
   {
      return eventData;
   }
   
   eventData.remainingTime = process->remainingTimeMs;
   
   //check if we have a process id to set
   if( process->pId >= 0 )
   {   
      eventData.pId = process->pId;
   }
   
   return eventData;
}

void* threadRunTimer( void* ptr )
{
   //ensure we grab our sent integer pointer.
   int* timeToWaitMs = (int *) ptr;
   
   //deference the pointer and send off the run timer
   runTimer( *timeToWaitMs );
   
   return NULL;
}

void logToFile( LogLinkedList* listCurrentPtr )
{
   char saveString[MAX_STR_LEN];
   //copyString( saveString, " " );
   //concatenateString( saveString , "======================================\n" );
   //concatenateString( saveString , "Simulator Log File Header\n" );
   
   //printf("\n\nSAVESTR:%s \n\nCOMPLOG:%s\n\n", saveString, completeLog);
   
   //concatenateString( saveString, completeLog );
   
   FILE* filePtr = fopen( "SimulatorLogFile.lgf", "w" );
   //fprintf( filePtr, "%s", completeLog );
   fclose( filePtr );
}

LogLinkedList* addNodeLL( LogLinkedList* localPtr, LogLinkedList* newNode )
{
   // check for local pointer assigned to null
   if( localPtr == NULL )
   {
      // access memory for new link/node
      localPtr = ( LogLinkedList* ) malloc( sizeof( LogLinkedList ) );

      // assign string value to the logLine
      // assign next pointer to null
      copyString( localPtr->logLine, newNode->logLine );
      localPtr->next = NULL;

      // return current local pointer
      return localPtr;
   }

   // assume end of list not found yet
   // assign recursive function to current's next link
   localPtr->next = addNodeLL( localPtr->next, newNode );

   // return current local pointer
   return localPtr;
}

LogLinkedList* clearLinkedList( LogLinkedList* localPtr )
{
   // check for local pointer not set to null (list not empty)
   if( localPtr != NULL )
   {
      // check for local pointer's next node not null
      if( localPtr->next != NULL )
      {
         // call recursive function with next pointer
         clearLinkedList( localPtr->next );
      }

      // after recursive call, release memory to OS
      free( localPtr );
   }
   
   return NULL; 
}



