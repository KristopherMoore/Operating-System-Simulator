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
   int timeToWaitMs = 0;
   double currentTime = 0.0;
   char timeString[MAX_STR_LEN];
   OpCodeType* loopMetaDataPtr;
   OpCodeType* currentProgramCounter;
   PCB* process = NULL;
   int processCount = 0;
   int indexI = 0;
   int scheduledProcess = 0;
   int oldScheduledProcess = 0;
   Boolean processingFlag = True;
   Boolean foundProcessFlag = False;
   EventData eventData;
   
   //Start Event Logging
   printf( "==========================\n" );
   printf("Begin Simulation\n\n");
   
   //EVENT LOG: System Start
   currentTime = accessTimer(ZERO_TIMER, timeString);
   eventData = generateEventData(OS, SystemStart, timeString, mdData, process);
   eventLogger(eventData);
   
   //EVENT LOG: Create PCB'S
   currentTime = accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, CreatePCBs, timeString, mdData, process);
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
   eventData = generateEventData(OS, AllProcNEW, timeString, mdData, process);
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
   }
   
   //EVENT LOG: All Processes init in READY
   currentTime = accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, AllProcREADY, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
   eventLogger(eventData);
   
   //Initialize in READY state
   for( indexI = 0; indexI < processCount; indexI++ )
   {
      pcbArray[indexI].pState = READY;
   }
   
   //Main Simulator Loop -- Loop while we have Processes not in EXIT
   while( processingFlag == True )
   {
      //Select process, utilizing cpuScheduler
      oldScheduledProcess = scheduledProcess;
      scheduledProcess = cpuScheduler( pcbArray, processCount );
      
      if(oldScheduledProcess != scheduledProcess)
      {
         //EVENT LOG: run start
         currentTime = accessTimer(LAP_TIMER, timeString);
         eventData = generateEventData(OS, ProcSelected, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
         eventLogger(eventData);
      }
      
      //Set Process in RUNNING // check if already RUNNING, if so ignore.
      if( pcbArray[scheduledProcess].pState != RUNNING )
      {
         pcbArray[scheduledProcess].pState = RUNNING;
      }
      
      //grab our PC, and send to Operation runner to handle run types
         //IE, RUN, I/O, or MEM Operations
      currentProgramCounter = pcbArray[scheduledProcess].programCounter;
      operationRunner( scheduledProcess, currentProgramCounter, configDataPtr,
                                                                  pcbArray );
      
      //CHECK FOR FINISH, since we started our program Counter with an offset 
         //from start, this will only ever be A(end)0;
      if( currentProgramCounter->opLtr == 'A' )
      {
         pcbArray[scheduledProcess].pState = EXIT;
            
         //EVENT LOG: end process and set in EXIT
         currentTime = accessTimer(LAP_TIMER, timeString);
         eventData = generateEventData(Process, ProcEnd, timeString,
                                    pcbArray[scheduledProcess].programCounter, 
                                                &pcbArray[scheduledProcess]);
         eventLogger(eventData);
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
   currentTime = accessTimer(LAP_TIMER, timeString);
   eventData = generateEventData(OS, SystemStop, timeString,
      pcbArray[scheduledProcess].programCounter, &pcbArray[scheduledProcess]);
   eventLogger(eventData);
   
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
                                 ConfigDataType* configDataPtr, PCB* pcbArray)
{
   char timeString[MAX_STR_LEN];
   int timeToWaitMs = 0;
   double currentTime = 0.0;
   EventData eventData;
   
   //RUN OPERATIONS
   if( programCounter->opLtr == 'P' )
   { 
      //EVENT LOG: run start
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
      
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer(timeToWaitMs);
   
      //EVENT LOG: run end
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
   }
   
   //MEMORY OPERATIONS
   else if( programCounter->opLtr == 'M' )
   {
      //EVENT LOG: run start
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
         
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer(timeToWaitMs);
   
      //EVENT LOG: run end
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
   }
      
   //I/O OPERATIONS
   else if( programCounter->opLtr == 'I'
            || programCounter->opLtr == 'O')
   {
      //EVENT LOG: run start
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpStart, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
         
      //Wait out our time
      timeToWaitMs = programCounter->opValue * configDataPtr->procCycleRate;
      runTimer(timeToWaitMs);
   
      //EVENT LOG: run end
      currentTime = accessTimer(LAP_TIMER, timeString);
      eventData = generateEventData(Process, ProcOpEnd, timeString,
                                 programCounter, &pcbArray[scheduledProcess]);
      eventLogger(eventData);
   }
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
      
      case ProcOpStart:
         sprintf( logCodeStr, "%d %s operation start\n",
                              eventData.pId, eventData.opStartOrEnd);
         break;
      
      case ProcOpEnd:
         sprintf( logCodeStr, "%d %s operation end\n",
                              eventData.pId, eventData.opStartOrEnd);
         break;
      
      case ProcSelected:
         sprintf( logCodeStr, "%d selected with %d  remaining\n",
                              eventData.pId, -100);
         break;
         
      case ProcSetIn:
         sprintf( logCodeStr, "%d set in %s  state\n",
                              eventData.pId, "TESTFIX");
         break;
         
      case ProcEnd:
         sprintf( logCodeStr, "%d ended and set in %s  state\n",
                              eventData.pId, "TESTFIX");
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
   
   printf("%s", finalLogStr);
   
}

EventData generateEventData(EventType eventType, LogCode logCode, 
                     char* timeString, OpCodeType* programCounter, PCB* process)
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
   
   //eventData.pStateStr = process->pState;
   eventData.remainingTime = process->remainingTimeMs;
   
   //check if we have a process id to set
   if(process->pId >= 0)
   {   
      eventData.pId = process->pId;
   }
   
   return eventData;
}



