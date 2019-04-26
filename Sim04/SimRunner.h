//Header File Information//////////////////////////////////////////////////////
/*
 * @file SimRunner.h
 *
 * @version 2.00
 *          Kristopher Moore (19 February 2019)
 *          Initial Sim02 Build.
 */
 
 //NOTE: This header is the base for both SimRunner.c and Logger.c, as they
 //      both require the same data structures, this avoids recursive definition
 
#ifndef SIM_RUNNER_H
#define SIM_RUNNER_H

#include <stdio.h>
#include <pthread.h>
#include "ConfigAccess.h"
#include "MetaDataAccess.h"
#include "MemoryManagementUnit.h"
#include "simtimer.h"
#include "StringUtils.h"


//Process States
typedef enum
{
   NEW,
   READY,
   RUNNING,
   BLOCKED,
   EXIT
   
} ProcessState;

//PCB data structure
typedef struct 
{
   ProcessState pState;
   int pId;
   int remainingTimeMs;
   OpCodeType* programCounter;
   MMU mmuInfo;
   
} PCB;

//Event Logger eventTypes
typedef enum
{
   OS,
   Process
   
} EventType;

//Event Logger logCodes
typedef enum
{
   BeginSim,
   SystemStart,
   CreatePCBs,
   AllProcNEW,
   AllProcREADY,
   Interrupt,
   ProcOpStart,
   ProcOpEnd,
   ProcSelected,
   ProcSetIn,
   ProcBlocked,
   ProcEnd,
   MMUAllocAttempt,
   MMUAllocSuccess,
   MMUAllocFailed,
   MMUAccessAttempt,
   MMUAccessSuccess,
   MMUAccessFailed,
   SegFault,
   SystemIdle,
   SystemStop,
   EndSim
   
} LogCode;

//Event Logger Event Data
typedef struct
{
   char* timeToPrint;
   EventType eventType;
   LogCode logCode;
   int pId;
   int remainingTime;
   char* pStateStr;
   char* opStartOrEnd;
   char opType[80];
   MMU mmuData;
} EventData;

//linked list for log structures
typedef struct LogLinkedList
{
   char logLine[80];
   struct LogLinkedList* next;
   
} LogLinkedList;

//Interrupt Queue Action Codes
typedef enum
{
   INIT,
   ENQUEUE,
   DEQUEUE,
   DESTRUCT,
   ISEMPTY,
   ISFULL,
   HEAD,
   REAR
   
} InterruptAction;

//ThreadInput data structure
typedef struct 
{
   int pId;
   int timeToWait;
   
} ThreadInput;

//Thread Action Codes
typedef enum
{
   tINIT,
   tISEMPTY,
   tPUSH,
   tPOP,
   tDESTRUCT,
   tFIRST
   
} ThreadAction;


//function prototypes for SimRunner.c
int simulationRunner( ConfigDataType* configDataPtr, OpCodeType* mdData );
int cpuScheduler( PCB* pcbArray, int processCount, 
                                                ConfigDataType* configDataPtr );
int operationRunner( int scheduledProcess, OpCodeType* programCounter, 
                                 ConfigDataType* configDataPtr, PCB* pcbArray,
                                    LogLinkedList* listCurrentPtr,
                                       MMU* mmuCurrentPtr, MMU* mmuHeadPtr );
void *threadRunTimer( void* threadInput );
pthread_t threadManager(ThreadAction action, ThreadInput* threadInput);
int findProcessCount( OpCodeType* loopMetaDataPtr, OpCodeType* mdData );
void createPCBs( PCB* pcbArray, OpCodeType* loopMetaDataPtr, int processCount );
void initInReady( PCB* pcbArray, int processCount );
void calcRemainingTimes( PCB* pcbArray, ConfigDataType* configDataPtr, 
                                                            int processCount );
int interruptQueue(InterruptAction action, int pId, int processCount);

//function prototypes for Logger.c
void eventLogger(EventData eventData, ConfigDataType* configDataPtr,
                                                LogLinkedList* listCurrentPtr);
EventData generateEventData(EventType eventType, LogCode logCode, 
                  char* timeString, OpCodeType* programCounter, PCB* process);
void logToFile(LogLinkedList* listCurrentPtr, ConfigDataType* configDataPtr );
LogLinkedList* addNodeLL( LogLinkedList* localPtr, LogLinkedList* newNode );
LogLinkedList* clearLinkedList( LogLinkedList* localPtr );


#endif // SIM_RUNNER_H



