//Program Information///////////////////////////////////////////////////////////
/*
 * @file Logger.c
 *
 *
 * @version 3.5
 *          Kristopher Moore (19 Feburary 2019)
 *          Condense of version 3 Build.
 */

//needs simRunner for prototypes and data definititons
#include "SimRunner.h"

/*
Function name: eventLogger
Algorithm: using eventData, and configurationPtr. This function creates the
            necessary strings for monitor and file output. Then stores each
            event in a linkedList for final file process by the logToFile method
Precondition: eventData, configData, and a LogLinkedList to append to
Postcondition: returns when its logging has completed
Exceptions: none
Notes: none
*/
void eventLogger(EventData eventData, ConfigDataType* configDataPtr, 
                                                  LogLinkedList* listCurrentPtr)
{
   //inits / declare strings holders
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
   
   //parse our eventType and add to string
   switch( eventData.eventType )
   {
      case OS:
         concatenateString( eventStr, "OS: " );
         break;
         
      case Process:
         sprintf( eventStr, " Process: %d,", eventData.pId );
         break;
   }
   
   //parse our logCode and add relevant text to string
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
         sprintf( logCodeStr, "%s start\n", eventData.opType );
         break;
      
      case ProcOpEnd:
         sprintf( logCodeStr, "%s end\n", eventData.opType );
         break;
      
      case ProcSelected:
         sprintf( logCodeStr, "Process %d selected with %d ms remaining\n",
                                       eventData.pId ,eventData.remainingTime);
         break;
         
      case ProcSetIn:
         sprintf( logCodeStr, "Process %d set in RUNNING state\n",
                                                               eventData.pId );
         break;
         
      case ProcEnd:
         sprintf( logCodeStr, 
                    "Process %d ended and set in EXIT state\n\n", eventData.pId );
         break;
      
      case MMUAllocAttempt:
         sprintf( logCodeStr, " MMU attempt to allocate %d/%d/%d\n", 
                                             eventData.mmuData.segNumber,
                                             eventData.mmuData.memBase,
                                             eventData.mmuData.memOffset );
         break;
      
      case MMUAllocSuccess:
         concatenateString( logCodeStr, "MMU sucessful allocate\n");
         break;
         
      case MMUAllocFailed:
         concatenateString( logCodeStr, "MMU failed to allocate\n");
         break;
         
      case MMUAccessAttempt:
         sprintf( logCodeStr, " MMU attempt to access %d/%d/%d\n", 
                                             eventData.mmuData.segNumber,
                                             eventData.mmuData.memBase,
                                             eventData.mmuData.memOffset );
         break;
      
      case MMUAccessSuccess:
         concatenateString( logCodeStr, "MMU sucessful access\n");
         break;
         
      case MMUAccessFailed:
         concatenateString( logCodeStr, "MMU failed to access\n");
         break;
      
      case SegFault:
         sprintf( logCodeStr, 
                 "Process %d experiences segmentation fault\n", eventData.pId );
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
   
   //NOW, we need to add the string to our linkedList.
   //allocate space, and ensure we intialize it
   newNodePtr = ( LogLinkedList * ) malloc( sizeof( LogLinkedList ) );
   copyString( newNodePtr->logLine, " " );
     
   //add node and adjust ptr
   listCurrentPtr = addNodeLL( listCurrentPtr, newNodePtr );
   listCurrentPtr = listCurrentPtr->next;
      
   //Get last node position, so we can set it
   while( listCurrentPtr != NULL )
   {
      oldNodePtr = listCurrentPtr;
      listCurrentPtr = listCurrentPtr->next;
   }
   //set our log string to our linked list
   copyString( oldNodePtr->logLine, " " );
   concatenateString( oldNodePtr->logLine, finalLogStr );
                        
   free( newNodePtr );
   
   
   //convert our code into a string, then check how we should be eventLogging
   configCodeToString( configDataPtr->logToCode, monitorString );
   
   //check if we need to print to screen, and do it.
   if( compareString( monitorString, "Both" ) == STR_EQ
            ||compareString( monitorString, "Monitor" ) == STR_EQ )
   {
      printf( "%s", finalLogStr ); 
   }
}


/*
Function name: generateEventData
Algorithm: using event information, generates the necessary struct info of 
            the eventData type. checks for vaild pointers before initialization
Precondition: eventInformation from calling method
Postcondition: returns a eventData struct for parsing in the EventLogger
Exceptions: none
Notes: none
*/
EventData generateEventData( EventType eventType, LogCode logCode, 
                  char* timeString, OpCodeType* programCounter, PCB* process )
{
   EventData eventData;
   char opTypeStr[MAX_STR_LEN];
   copyString( opTypeStr, " " );
   
   //load parameters into struct
   eventData.eventType = eventType;
   eventData.logCode = logCode;
   eventData.timeToPrint = timeString;
   eventData.opStartOrEnd = "";
   copyString( eventData.opType, " " );
   eventData.pStateStr = "";
   eventData.remainingTime = 0;
   eventData.pId = -1;
   
   
   //return if our pointer isnt valid, the following calls rely on the process
   if( process == NULL  || programCounter == NULL )
   {
      return eventData;
   }
   
   //store remaining time
   eventData.remainingTime = process->remainingTimeMs;
   
   //check if we have a process id to set
   if( process->pId >= 0 )
   {   
      eventData.pId = process->pId;
   }
   
   //check what our operation name is, if any
   if( compareString( programCounter->opName, "hard drive" ) == STR_EQ )
   {
      concatenateString( opTypeStr, "hard drive" );
   }
   else if( compareString( programCounter->opName, "keyboard" ) == STR_EQ )
   {
      concatenateString( opTypeStr, "keyboard" );
   }
   else if( compareString( programCounter->opName, "printer" ) == STR_EQ )
   {
      concatenateString( opTypeStr, "printer" );
   }
   else if( compareString( programCounter->opName, "monitor" ) == STR_EQ )
   {
      concatenateString( opTypeStr, "monitor" );
   }
   
   //check what our operation type is, if any of these apply
   if( programCounter->opLtr == 'P' )
   {
      concatenateString( opTypeStr, "run operation" );
   }
   else if( programCounter->opLtr == 'I')
   {
      concatenateString( opTypeStr, " input" );
   }
   else if( programCounter->opLtr == 'O')
   {
      concatenateString( opTypeStr, " output" );
   }
   
   //copy the concated string into our opType event Data.
   copyString( eventData.opType, opTypeStr );
   
   //store a copy of our mmuInformation (SS BBB AAA values)
   eventData.mmuData = process->mmuInfo;
   
   return eventData;
}


/*
Function name: logToFile
Algorithm: given the LogLinkedList, and the configuration data, logs simulator
            events into a .lgf file
Precondition: filled LogLinkedList and configuration data
Postcondition: writes to a SimulatorLogFile.lgf
Exceptions: none
Notes: none
*/
void logToFile( LogLinkedList* listHeadPtr , ConfigDataType* configDataPtr )
{
   FILE* filePtr = fopen( "SimulatorLogFile.lgf", "w" );
   
   //write header
   fprintf( filePtr, "======================================\n" );
   fprintf( filePtr, "Simulator Log File Header\n\n" );
   
   //write config data
   fprintf( filePtr, "File Name\t\t\t: %s\n", 
                                       configDataPtr->metaDataFileName );
   fprintf( filePtr, "CPU Scheduling\t\t\t: %d\n", 
                                       configDataPtr->cpuSchedCode );
   fprintf( filePtr, "Quantum Cycles\t\t\t: %d\n", 
                                       configDataPtr->quantumCycles );
   fprintf( filePtr, "Memory Available (KB)\t\t: %d\n", 
                                       configDataPtr->memAvailable );
   fprintf( filePtr, "Processor Cycle Rate (ms/cycle)\t: %d\n", 
                                       configDataPtr->procCycleRate );
   fprintf( filePtr, "I/O Cycle Rate (ms/cycle)\t: %d\n\n", 
                                       configDataPtr->ioCycleRate );
   
   //write from each of the nodes of the logging linkedlist
   while( listHeadPtr != NULL )
   {
      fprintf( filePtr, "%s", listHeadPtr->logLine );
      listHeadPtr = listHeadPtr->next;
   }
   fclose( filePtr );
}

/*
Function name: addNodeLL
Algorithm: responsible for allocation of space for new node in LogLinkedList
Precondition: ptr to list, and a new node
Postcondition: last node in list
Exceptions: none
Notes: none
*/
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

/*
Function name: clearLinkedList
Algorithm: responsible freeing all memory allocated by log linked list
Precondition: ptr to the head of list
Postcondition: returns null after all memory has been freed
Exceptions: none
Notes: none
*/
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

