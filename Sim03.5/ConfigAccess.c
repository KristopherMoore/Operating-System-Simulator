//Program Information//////////////////////////////////////////////////////////////////////////////
/*
 * @file ConfigAccess.c
 *
 * @version 1.10
 *          Kristopher Moore (30 January 2019)
 *          Debugging and Error Fixing.
 *
 * @version 1.00
 *          Kristopher Moore (22 January 2019)
 *          Initial Program Build.
 */

#include "ConfigAccess.h"

/*
Function name: getConfigData
Algorithm: opens file, acquires configuration data,
           returns pointer to data structure
Precondition: for correct operation, file is available, is formatted correctly,
              and has all configuration lines and data although
              the configuration lines are not required to be
              in a specific order
Postcondition: in correct operation,
               returns pointer to correct configuration data structure
Exceptions: correct and appropriately (without program failure)
            responds to and reports file access failure,
            incorrectly formatted lead or end descriptors,
            incorrectly formatted prompt, data out of range,
            and incomplete file conditons
Notes: none
*/

int getConfigData( char *fileName, ConfigDataType **configData )
{

   // set constant number of data lines to 9
   const int NUM_DATA_LINES = 9;

   // set line counter to zero
   int lineCtr = 0;

   // set read only constant to "r"
   const char READ_ONLY_FLAG[] = "r";

   // create pointer for data input
   ConfigDataType *tempData;

   // declare other variables
   FILE *fileAccessPtr;
   char dataBuffer[ MAX_STR_LEN ];
   int intData, dataLineCode;
   double doubleData;

   // initialize config data pointer in case of return error
   *configData = NULL;

   // open file
   fileAccessPtr = fopen( fileName, READ_ONLY_FLAG );

   // check for file open failure
   if( fileAccessPtr == NULL )
   {
      return CFG_FILE_ACCESS_ERR;
   }

   // get first line, check for failure
   if( getLineTo( fileAccessPtr, MAX_STR_LEN, COLON,
                      dataBuffer, IGNORE_LEADING_WS ) != NO_ERR 
        || compareString( dataBuffer, "Start Simulator Configuration File" )
                                                                   != STR_EQ )
   {
      // close file access
      fclose( fileAccessPtr );

      return CFG_CORRUPT_DESCRIPTOR_ERR;
   }

   // create temporary pointer to configuration data structure
   tempData = (ConfigDataType *) malloc( sizeof( ConfigDataType ) );

   // loop to end of config data items
   while( lineCtr < NUM_DATA_LINES )
   {
      // get line leader, check for failure
      if( getLineTo( fileAccessPtr, MAX_STR_LEN, COLON,
                           dataBuffer, IGNORE_LEADING_WS ) != NO_ERR )
      {
         // free temp struct memory
         free( tempData );

         // close file access
         fclose( fileAccessPtr );
         
         return INCOMPLETE_FILE_ERR;
      }

      // find correct data line by number
      dataLineCode = getDataLineCode( dataBuffer );

      // check for data line found
      if( dataLineCode != CFG_CORRUPT_PROMPT_ERR )
      {
         // check for version number (double value)
         if( dataLineCode == CFG_VERSION_CODE )
         {
            // get version number
            fscanf( fileAccessPtr, "%lf", &doubleData );
         }

         // otherwise, check for metaData or logTo file names
         // or CPU Scheduling names
         else if( dataLineCode == CFG_MD_FILE_NAME_CODE
                     || dataLineCode == CFG_LOG_FILE_NAME_CODE
                           || dataLineCode == CFG_CPU_SCHED_CODE
                              || dataLineCode == CFG_LOG_TO_CODE )
         {
            // get string input
            fscanf( fileAccessPtr, "%s", dataBuffer );
         }

         // otherwise, assume integer data
         else
         {
            fscanf( fileAccessPtr, "%d", &intData );
         }

         // check for data value in range
         if( valueInRange( dataLineCode, intData, doubleData, dataBuffer )
                                                                       == True )
         {
            // assign to data pointer depending on config item
            switch( dataLineCode )
            {
               // case version code
               case CFG_VERSION_CODE:
                  // assign version to double data
                  tempData->version = doubleData;
                  break;

               // case meta data file name code
               case CFG_MD_FILE_NAME_CODE:
                  // assign version to string data
                  copyString( tempData->metaDataFileName, dataBuffer );
                  break;

               // case CPU schedule code
               case CFG_CPU_SCHED_CODE:
                  // assign CPU schedule code using string data
                  tempData->cpuSchedCode = getCpuSchedCode( dataBuffer );
                  break;

               // case quantum cycles code
               case CFG_QUANT_CYCLES_CODE:
                  // assign quantum cycles to int data
                  tempData->quantumCycles = intData;
                  break;

               // case memory available code
               case CFG_MEM_AVAILABLE_CODE:
                  // assign memory available to int data
                        tempData->memAvailable = intData;
                        break;
                        
               // case processor cycles code
               case CFG_PROC_CYCLES_CODE:
                  // assign processor cycle rate to int data
                  tempData->procCycleRate = intData;
                  break;

               // case I/O cycles code
               case CFG_IO_CYCLES_CODE:
                  // assign I/O cycle rate to int data
                  tempData->ioCycleRate = intData;
                  break;

               // case log to code
               case CFG_LOG_TO_CODE:
                  // assign code using string data converted to log to code
                  tempData->logToCode = getLogToCode( dataBuffer );
                  break;

               // case log to file name code
               case CFG_LOG_FILE_NAME_CODE:
                  // assign log to file name using string data
                  copyString( tempData->logToFileName, dataBuffer );
                  break;
            }
         }

         // otherwise, assume data value not in range
         else
         {
            // free temp struct memroy
            free( tempData );

            // close file access
            fclose( fileAccessPtr );

            return CFG_DATA_OUT_OF_RANGE_ERR;
         }
      }

      // otherwise, assume data line not found
      else
      {
         // free temp struct memory
         free( tempData );

         // close file access
         fclose( fileAccessPtr );
         
         return CFG_CORRUPT_PROMPT_ERR;
      }

      lineCtr++;
   }
   
   // acquire end of sim config string
   if( getLineTo( fileAccessPtr, MAX_STR_LEN, PERIOD, 
                      dataBuffer, IGNORE_LEADING_WS ) != NO_ERR 
         || compareString( dataBuffer, "End Simulator Configuration File" )
                                                                   != STR_EQ )
   {
      // free temp struct memory
      free ( tempData );

      // close file access
      fclose( fileAccessPtr );

      return CFG_CORRUPT_DESCRIPTOR_ERR;
   }

   // assign temporary pointer to parameter return pointer
   *configData = tempData;

   // close file access
   fclose( fileAccessPtr );

   // return no error code
	return NO_ERR;
}

/*
Function name: getDataLineCode
Algorithm: tests string for one of known leader strings, returns line number
           if string is correct, returns CFG_CORRUPT_DATA_ERR
           if string is not found
Precondtion: dataBuffer is valid C-style string
Postcondtion: returns line number of data item in terms of a constant
              (e.g., CFG_VERSION_CODE, CFG_CPU_SCHED_CODE, etc.)
Exceptions: returns CFG_CORRUPT_FILE_ERR if string is not identified
Notes: none
*/
int getDataLineCode( char *dataBuffer )
{
   // return appropriate code depending on prompt string provided

   // check for version/phase string
   if( compareString( dataBuffer, "Version/Phase" ) == STR_EQ )
   {
      return CFG_VERSION_CODE;
   }

   // check for file path string
   if( compareString( dataBuffer, "File Path" ) == STR_EQ )
   {
      return CFG_MD_FILE_NAME_CODE;
   }

   // check for cpu scheduling code string
   if( compareString( dataBuffer, "CPU Scheduling Code" ) == STR_EQ )
   { 
      return CFG_CPU_SCHED_CODE;
   }

   // check for quantum time string
   if( compareString( dataBuffer, "Quantum Time (cycles)" ) == STR_EQ )
   {
      return CFG_QUANT_CYCLES_CODE;
   }

   // check for memory available string
   if( compareString( dataBuffer, "Memory Available (KB)" ) == STR_EQ 
         || compareString(dataBuffer, "Memory Available (MB)") == STR_EQ)
   {
      return CFG_MEM_AVAILABLE_CODE;
   }

   // check for processor cycle time string
   if( compareString( dataBuffer, "Processor Cycle Time (msec)" ) 
                                                               == STR_EQ )
   {
      return CFG_PROC_CYCLES_CODE;
   }

   // check for I/O cycle time string
   if( compareString( dataBuffer, "I/O Cycle Time (msec)" ) == STR_EQ )
   {
      return CFG_IO_CYCLES_CODE;
   }

   // check for log to string
   if( compareString( dataBuffer, "Log To" ) == STR_EQ )
   {
      return CFG_LOG_TO_CODE;
   }

   // check for log file path string
   if( compareString( dataBuffer, "Log File Path" ) == STR_EQ )
   {
      return CFG_LOG_FILE_NAME_CODE;
   }

    // at this point, assume failed string access, return correct prompt error
    return CFG_CORRUPT_PROMPT_ERR;
}

/*
Function name: getCpuSchedCode
Algorithm: converts string data (e.g., "SJF-N", "SRTF-P")
           to constant code number to be stored as integer
Precondtion: codeStr is a valid C-style string with one
             of the specified cpu scheduling operations
Postcondition: returns error code representing scheduling actions
Exceptions: defaults to fcfs code
Notes: none
*/
ConfigDataCodes getCpuSchedCode( char *codeStr )
{
   // set up temporary string for testing
   int strLen = getStringLength( codeStr );
   char *tempStr = (char *) malloc( strLen + 1 );

   // set default return to FSFC-N
   int returnVal = CPU_SCHED_FCFS_N_CODE;

   // set string to lower case for testing
   setStrToLowerCase( tempStr, codeStr );

   // check for SJF-N
   if( compareString( tempStr, "sjf-n" ) == STR_EQ )
   {
      returnVal = CPU_SCHED_SJF_N_CODE;
   }

   // check for SRTF-P
   else if( compareString( tempStr, "srtf-p" ) == STR_EQ )
   {
      returnVal = CPU_SCHED_SRTF_P_CODE;
   }

   // check for FCFS-P
   else if( compareString( tempStr, "fcfs-p" ) == STR_EQ )
   {
      returnVal = CPU_SCHED_FCFS_P_CODE;
   }

   // check for RR-P
   else if( compareString( tempStr, "rr-p" ) == STR_EQ )
   {
      returnVal = CPU_SCHED_RR_P_CODE;
   }

   // free temp string memory
   free( tempStr );

   // return code found
   return returnVal;
}

/*
Function name: valueInRange
Algorithm: tests one of three values (int, double, string) for being
           in specified range, depending on data code
           (i.e, specified config value)
Precondtion: one of the three data values is valid
Postcondition: returns True if data is within specified parameters,
               False otherwise
Exceptions: metadata or logfile names are ignored and return True
Notes: none
*/
Boolean valueInRange( int lineCode, int intVal, 
                                           double doubleVal, char *stringVal )
{
   Boolean result = True;

   // declare other variables
   char *tempStr;
   int strLen;

   // use line code to indentify prompt line            
   switch( lineCode )
   {
      // for version code
      case CFG_VERSION_CODE:
         // check if limits of version code are executed
         if( doubleVal < 0.00 || doubleVal > 10.00 )
         {
            // set Boolean result to false
            result = False;
         }
         break;

      // for cpu scheduling code
      case CFG_CPU_SCHED_CODE:
         // create temporary string and set to lower case
         strLen = getStringLength( stringVal );
         tempStr = (char *) malloc( strLen + 1 );
         setStrToLowerCase( tempStr, stringVal );

         // check for not finding one of the scheduling strings 
         if( compareString( tempStr, "none" ) != STR_EQ
               && compareString( tempStr, "fcfs-n" ) != STR_EQ
               && compareString( tempStr, "sjf-n" ) != STR_EQ
               && compareString( tempStr, "srtf-p" ) != STR_EQ
               && compareString( tempStr, "fcfs-p" ) != STR_EQ
               && compareString( tempStr, "rr-p" ) != STR_EQ )
         {
            result = False;
         }
         
         free( tempStr );
         break;

      // for quantum cycles
      case CFG_QUANT_CYCLES_CODE:
         // check for quantum cycles limits exceeded
         if( intVal < 0 || intVal > 100 )
         {
            result = False;
         }
         break;

      // for memory available
      case CFG_MEM_AVAILABLE_CODE:
         // check for available memory limits exceeded
         if( intVal < 0 || intVal > 102400 )
         {
            result = False;
         }
         break;

      // for process cycles
      case CFG_PROC_CYCLES_CODE:
         // check for process cycles limits exceeded
         if( intVal < 0 || intVal > 1000 )
         {
            result = False;
         }
         break;

      // for I/O cycles
      case CFG_IO_CYCLES_CODE:
         // check for I/O cycles limits exceeded
         if( intVal < 1 || intVal > 10000 )
         {
            result = False;
         }
         break;

      // for log to operation
      case CFG_LOG_TO_CODE:
         // create temporary string and set to lower case
         strLen = getStringLength( stringVal );
         tempStr = (char *) malloc( strLen + 1 );
         setStrToLowerCase( tempStr, stringVal );

         // check for not finding one of the log to strings
         if( compareString( tempStr, "both" ) != STR_EQ
             && compareString( tempStr, "monitor" ) != STR_EQ
             && compareString( tempStr, "file" ) != STR_EQ )
         {
            result = False;
         }

         // free temp string memory
         free( tempStr );

         break;
   }

   // return result of limits analysis
   return result;
}

/*
Function name: getLogToCode
Algorithm: converts string data (e.g., "File", "Monitor")
           to constant code number to be stored as integer
Precondtion: codeStr is a C-style string with one 
             of the specified log to operations
Postcondtion: returns code representing log to actions
Exceptions: defaults to monitor code
Notes: none
*/

ConfigDataCodes getLogToCode( char *logToStr )
{
   // create temporary string
   int strLen = getStringLength( logToStr );
   char *tempStr = (char *) malloc( strLen + 1 );

   // set default return value to log to monitor
   int returnVal = LOGTO_MONITOR_CODE;

   // set temp string to lower case
   setStrToLowerCase( tempStr, logToStr );

   // check for BOTH
   if( compareString( tempStr, "both" ) == STR_EQ )
   {
      returnVal = LOGTO_BOTH_CODE;
   }

   // check for FILE
   if( compareString( tempStr, "file" ) == STR_EQ )
   {
      returnVal = LOGTO_FILE_CODE;
   }

   // free temp string memory
   free( tempStr );

   // return code found
   return returnVal;
}

/*
Function name: displayConfigData
Algorithm: diagnostic function to show config data output
Precondition: parameter has pointer to allocated data set
Postcondition: configuration data is displayed to the screen
Exceptions: none
Notes: none
*/
void displayConfigData( ConfigDataType *configData )
{
   char displayString[ STD_STR_LEN ];

   // print lines of display
   printf( "\nConfig File Display\n" );
   printf( "===================\n" );
   printf( "Version                : %3.2f\n", configData->version );
   printf( "Program File Name      : %s\n", configData->metaDataFileName );
   configCodeToString( configData->cpuSchedCode, displayString );
   printf( "CPU schedule selection : %s\n", displayString );
   printf( "Quantum time           : %d\n", configData->quantumCycles );
   printf( "Memory Available       : %d\n", configData->memAvailable );
   printf( "Process cycle rate     : %d\n", configData-> procCycleRate );
   printf( "I/O cycle rate         : %d\n", configData->ioCycleRate );
   configCodeToString( configData->logToCode, displayString );
   printf( "Log to selection       : %s\n", displayString );
   printf( "Log file name          : %s\n", configData->logToFileName );
}

/*
Function name: configCodeToString
Algorithm: utility function to support display
           of CPU scheduling or Log To code strings
Precondition: code variable holds constant value from ConfigDataCodes
              for item (e.g., CPU_SCHED_SRTF_P, LOGTO_FILE_CODE, etc.)
Postcondition: string parameter holds correct string
               associated with the given constant
Exceptions: none
Notes: none
*/
void configCodeToString( int code, char *outString )
{
   // Define array with eight items, and short (10) lengths
   char displayStrings[ 8 ][ 10 ] = { "SJF-P", "SRTF-P", "FCFS-P",
                                       "RR-P", "FCFS-N", "Monitor",
                                       "File", "Both" };

   // copy string to return parameter
   copyString( outString, displayStrings[ code ] );
}

/*
Function name: displayConfigError
Algorithm: utility function to support display
	       or error code strings
Precondtion: parameter holds constant value from error code list
             for item (e.g., CFG_FILE_ACCESS_ERR, etc.)
Postcondition: correct error string (related to integer error code)
               is displayed
Exceptions: none
Notes: none
*/
void displayConfigError( int errCode )
{
   // Define array with seven items, and short (40) lengths
   // Includes three potential errors from StringManipErrors
   char displayStrings[ 7 ][ 40 ] = 
                                 { "No Error",
                                   "Incomplete File Error",
                                   "Input Buffer Overrun",
                                   "Configuration File Access Error",
                                   "Corrupt Configuration Descriptor Error",
                                   "Data Out Of Range Configuration Error",
                                   "Corrupt Configuration Prompt Error" };

   printf( "\nFATAL ERROR: %s, Program aborted\n", displayStrings[ errCode ] );
}

/*
Function name: clearConfigData
Algorithm: frees all allocated memory for config data
Precondition: pointer to config data linked list passed
              in as a parameter
Postcondition: config data memory is freed,
               pointer is set to null
Exceptions: none
Notes: none
*/
void clearConfigData( ConfigDataType **configDataPtr )
{
   // check that config data pointer is not null
   if( *configDataPtr != NULL )
   {
      // free data structure memory
      free( *configDataPtr );
   }

   // set config data pointer to null (returned as paramter)
   *configDataPtr = NULL;
}








