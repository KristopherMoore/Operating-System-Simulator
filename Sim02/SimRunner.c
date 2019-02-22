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
#include "ConfigAccess.h"
#include "MetaDataAccess.h"
#include "pthread.h"
#include "SimRunner.h"
#include "simtimer.h"
#include "StringUtils.h"
 
int simulationRunner(ConfigDataType* configDataPtr, OpCodeType* mdData)
{
   //initializations
   int timeToWaitMs = 1000;
   double currentTime = 0.0;
   char timeString[MAX_STR_LEN];
   OpCodeType* loopMetaDataPtr;
   OpCodeType* currentProgramCounter;
   int processCount = 0;
   int indexI = 0;
   int scheduledProcess = 0;
   Boolean processingFlag = True;
   Boolean foundProcessFlag = False;
   EventData eventData;
   
   //Start Event Logging
   printf( "==========================\n" );
   printf("Begin Simulation\n");
   
   //EVENT LOG: System Start
   currentTime = accessTimer(ZERO_TIMER, timeString);
   eventData = generateEventData(OS, SystemStart, timeString);
   eventLogger(eventData);
   
   //START TESTING CODE: ///////////////////////////////////////////////////////
   currentTime = accessTimer(ZERO_TIMER, timeString);
   printf("\nCur Time: %lf", currentTime);
   printf("\nCur TimeSTR: %s", timeString);
   
   runTimer(timeToWaitMs);
   
   currentTime = accessTimer(LAP_TIMER, timeString);
   printf("\nCur Time: %lf", currentTime);
   printf("\nCur TimeSTR: %s", timeString);
   
   timeToWaitMs = 0.01;
   runTimer(timeToWaitMs);
   
   currentTime = accessTimer(LAP_TIMER, timeString);
   printf("\nCur Time: %lf", currentTime);
   printf("\nCur TimeSTR: %s", timeString);
   
   //END TESTING CODE: /////////////////////////////////////////////////////////
   
   //EVENT LOG: Create PCB'S
   currentTime = accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, CreatePCBs, timeString);
   eventLogger(eventData);
   
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
      //point to next node
      loopMetaDataPtr = loopMetaDataPtr->next;
   }
   
   //reset our loopMetaDataPtr to start
   loopMetaDataPtr = mdData;
   
   
   //EVENT LOG: All Processes init in NEW
   currentTime = accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, AllProcNEW, timeString);
   eventLogger(eventData);
   
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
      printf("\npcbArray[%d]: pid: %d , state: %d, programCounter: %c   %d\n", indexI, pcbArray[indexI].pId, pcbArray[indexI].pState, pcbArray[indexI].programCounter->opLtr, pcbArray[indexI].programCounter->opValue);
   }
   
   //Initialize in READY state
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      pcbArray[indexI].pState = READY;
      printf("\npcbArray[%d]: pid: %d , state: %d\n", indexI, pcbArray[indexI].pId, pcbArray[indexI].pState);
   }
   
   //Main Simulator Loop -- Loop while we have Processes not in EXIT
   printf("\n\n TEST BEFORE SEG FAULT\n\n");
   while( processingFlag == True )
   {
      //Select process, utilizing cpuScheduler
      scheduledProcess = cpuScheduler( pcbArray, processCount );
      
      //Set Process in RUNNING // check if already RUNNING, if so ignore.
      if( pcbArray[scheduledProcess].pState != RUNNING )
      {
         pcbArray[scheduledProcess].pState = RUNNING;
      }
      
      //CHECK OPERATION TYPES and RUN ACCORDINGLY
      //RUN OPERATIONS
      currentProgramCounter = pcbArray[scheduledProcess].programCounter;
      if( currentProgramCounter->opLtr == 'P' )
      {
         timeToWaitMs = currentProgramCounter->opValue * configDataPtr->procCycleRate;
         runTimer(timeToWaitMs);
   
         currentTime = accessTimer(LAP_TIMER, timeString);
         printf("\nCur Time: %lf", currentTime);
         printf("\nCur TimeSTR: %s", timeString);
      }
      
      //MEMORY OPERATIONS
      if( currentProgramCounter->opLtr == 'M' )
      {
         timeToWaitMs = 
            currentProgramCounter->opValue * configDataPtr->procCycleRate;
            
         runTimer(timeToWaitMs);
   
         currentTime = accessTimer(LAP_TIMER, timeString);
         printf("\nCur Time: %lf", currentTime);
         printf("\nCur TimeSTR: %s", timeString);
      }
      
      //I/O OPERATIONS
      else if( currentProgramCounter->opLtr == 'I'
               || currentProgramCounter->opLtr == 'O')
      {
         timeToWaitMs = 
            currentProgramCounter->opValue * configDataPtr->ioCycleRate;
                  
         runTimer(timeToWaitMs);
   
         currentTime = accessTimer(LAP_TIMER, timeString);
         printf("\nCur Time: %lf", currentTime);
         printf("\nCur TimeSTR: %s", timeString);
      }
      
      //APPLICATION OPS, since we started our program Counter with an offset 
         //from start, this will only ever be A(end)0;
      else if( currentProgramCounter->opLtr == 'A' )
      {
         pcbArray[scheduledProcess].pState = EXIT;
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
   
   
   
   //System Stop
   
   //EXIT with normal operation
   printf("\nEnd Simulation - Complete\n");
   printf( "==========================\n" );
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
      if( pcbArray[indexI].pState == READY )
      {
         scheduledPid = pcbArray[indexI].pId;
      }
      
      indexI++;
   }
   
   return scheduledPid;
}

void eventLogger(EventData eventData)
{
   //declare strings holders
   char finalLogStr[MAX_STR_LEN];
   char eventStr[MAX_STR_LEN];
   char logCodeStr[MAX_STR_LEN];
   char logCodeExtend[MAX_STR_LEN];
   
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
         
      case ProcSelected:
         sprintf( logCodeStr, "Process: %d selected with %d  remaining\n",
                              eventData.pId, eventData.remainingTime);
         break;
         
      case ProcSetIn:
         sprintf( logCodeStr, "Process: %d set in %s  state\n",
                              eventData.pId, eventData.pStateStr);
         break;
         
      case ProcEnd:
         sprintf( logCodeStr, "Process: %d ended and set in %s  state\n",
                              eventData.pId, eventData.pStateStr);
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
   concatenateString( finalLogStr, eventData.timeToPrint );
   concatenateString( finalLogStr, "," );
   concatenateString( finalLogStr, eventStr );
   concatenateString( finalLogStr, logCodeStr );
   concatenateString( finalLogStr, logCodeExtend );
   
   printf("%s", finalLogStr);
   
}

EventData generateEventData(EventType eventType, LogCode logCode, 
                                                            char* timeString)
{
   EventData eventData;
   
   //load parameters into struct
   eventData.eventType = eventType;
   eventData.logCode = logCode;
   eventData.timeToPrint = timeString;
   
   return eventData;
}



