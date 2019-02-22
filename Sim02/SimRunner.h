//Header File Information//////////////////////////////////////////////////////////////////////////////
/*
 * @file SimRunner.h
 *
 * @version 2.00
 *          Kristopher Moore (19 February 2019)
 *          Initial Sim02 Build.
 */
 
#ifndef SIM_RUNNER_H
#define SIM_RUNNER_H

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
   ProcOpStart,
   ProcOpEnd,
   ProcSelected,
   ProcSetIn,
   ProcEnd,
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
} EventData;

//Process States
typedef enum
{
   NEW,
   READY,
   RUNNING,
   EXIT
   
} ProcessState;

//PCB data structure
typedef struct 
{
   ProcessState pState;
   int pId;
   int remainingTimeMs;
   OpCodeType* programCounter;
   
} PCB;

//function prototypes
int simulationRunner( ConfigDataType* configDataPtr, OpCodeType* mdData );
int cpuScheduler( PCB* pcbArray, int processCount );
void operationRunner(int scheduledProcess, OpCodeType* programCounter, 
                                 ConfigDataType* configDataPtr, 
                                                      PCB* pcbArray,
                                                            char* completeLog);
void eventLogger(EventData eventData, ConfigDataType* configDataPtr,
                                                            char* completeLog);
EventData generateEventData(EventType eventType, LogCode logCode, 
                  char* timeString, OpCodeType* programCounter, PCB* process);
void *threadRunTimer(void* ptr);
void logToFile(char* completeLog);

#endif // SIM_RUNNER_H



