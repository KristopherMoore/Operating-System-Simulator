//Program Information//////////////////////////////////////////////////////////////////////////////
/*
 * @file MetaDataAccess.c
 *
 * @version 1.10
 *          Kristopher Moore (30 January 2019)
 *          Debugging and Error Fixing.
 *
 * @version 1.00
 *          Kristopher Moore (22 January 2019)
 *          Initial Program Build.
 */

#include "MetaDataAccess.h"

/*
Function name: getOpCodes
Algorithm: opens file, acquires op code data,
           returns pointer to head of linked list
Precondtion: for correct operation, file is available, is formated correrctly,
             and has all correctly formed op codes
Postcondition: in correct operation,
               returns pointer to head of op code linked list
Exceptions: correctly and appropriately (without program failure)
            responds to and reports file access failure,
            incorrectly formatted lead or end descriptors,
            incorrectly formatted prompt, incorrect op code letter,
            icorrect op code name, op code value out of range,
            and incomplete file conditions
Notes: none
*/

int getOpCodes( char *fileName, OpCodeType **opCodeDataHead )
{
   // initialize read only constant
   const char READ_ONLY_FLAG[] = "r";

   // initialize start and end counts for balanced app operations
   int startCount = 0, endCount = 0;

   // initialize local head pointer to null
   OpCodeType *localHeadPtr = NULL;

   // initialize variables
   int accessResult;
   char dataBuffer[ MAX_STR_LEN ];
   OpCodeType *newNodePtr;
   FILE *fileAccessPtr;

   // initialize op code data pointer in case of return error
   *opCodeDataHead = NULL;

   // open file for reading
   fileAccessPtr = fopen( fileName, READ_ONLY_FLAG );

   // check file for open failure
   if( fileAccessPtr == NULL)
   {
      return MD_FILE_ACCESS_ERR;
   }

   // check first line for correct leader
   if( getLineTo( fileAccessPtr, MAX_STR_LEN, COLON,
                             dataBuffer, IGNORE_LEADING_WS ) != NO_ERR
      || compareString( dataBuffer, "Start Program Meta-Data Code" ) != STR_EQ )
   {
      // close file
      fclose( fileAccessPtr );
      
      return MD_CORRUPT_DESCRIPTOR_ERR;
   }

   // allocate memory for the temporary data structure
   newNodePtr = ( OpCodeType * ) malloc( sizeof( OpCodeType ) );

   // get the first op command
   accessResult = getOpCommand( fileAccessPtr, newNodePtr );

   // get start and end counts for later comparison
   startCount = updateStartCount( startCount, newNodePtr->opName );
   endCount = updateEndCount( endCount, newNodePtr->opName );

   // check for failure of first complete op command
   if( accessResult != COMPLETE_OPCMD_FOUND_MSG )
   {
      // close file
      fclose( fileAccessPtr );

      // clear data from the structure list
      *opCodeDataHead = clearMetaDataList( localHeadPtr );

      // free temporary structure memory
      free( newNodePtr );

      // return result of operation
      return accessResult;
   }

   // loop across all remaining op commands
   // (while complete op commands are found)
   while( accessResult == COMPLETE_OPCMD_FOUND_MSG )
   {
      // add the new op command to the linked list
      localHeadPtr = addNode( localHeadPtr, newNodePtr );

      // get a new op command
      accessResult = getOpCommand( fileAccessPtr, newNodePtr );

      // update start and end counts for later comparison
      startCount = updateStartCount( startCount, newNodePtr->opName );
      endCount = updateEndCount( endCount, newNodePtr->opName );
   }

   // after loop completion, check for last op command found
   if( accessResult == LAST_OPCMD_FOUND_MSG )
   {
      // check for start and end op code counts equal
      if( startCount == endCount )
      {
         // add the last node to the linked list
         localHeadPtr = addNode( localHeadPtr, newNodePtr );

         // set access result to no error for later operation
         accessResult = NO_ERR;

         // check last line for incorrect end descriptor
         if( getLineTo( fileAccessPtr, MAX_STR_LEN, PERIOD,
                             dataBuffer, IGNORE_LEADING_WS ) != NO_ERR
            || compareString( dataBuffer, "End Program Meta-Data Code" ) != STR_EQ )
         {
            // set access result to corrupted descriptor error
            accessResult = MD_CORRUPT_DESCRIPTOR_ERR;
         }
      }

      // otherwise, assume start count not equal to end count
      else
      {
         // set access result to unbalance start/end error
         accessResult = UNBALANCED_START_END_ERR;
      }
   }

   // check for any errors found (not no error)
   if( accessResult != NO_ERR )
   {
      // clear the op command list
      localHeadPtr = clearMetaDataList( localHeadPtr );
   }

   // close access file
   fclose( fileAccessPtr );

   // release temporary structure memory
   free( newNodePtr );

   // assign temporary local head pointer to parameter return pointer
   *opCodeDataHead = localHeadPtr;

   // return access result
   return 0;
}

/*
Function name: getOpCommand
Algorithm: acquires one op command, verifies all parts of it,
           returns as parameter
Precondition: file is open and file cursor is at beginning
              of an op code
Postcondition: in correct operation,
               finds, tests, and returns op command as parameter,
               and returns status as integer
               - either complete op command found,
               or last op command found
Exceptions: correctly and appropriately (without program failure)
            responds to and reports file access failure,
            incorrectly formatted op command letter,
            incorrectly formatted op command name,
            incorrect or out of range op command value
Notes: none
*/
int getOpCommand( FILE *filePtr, OpCodeType *inData )
{
   // initialize local constants - max op name and max op value lengths (10 & 9)
   const int MAX_OP_NAME_LENGTH = 10; // actual max name length
   const int MAX_OP_VALUE_LENGTH = 9; // actual max value length

   // initialize integer buffer value to zero
   int intBuffer = 0;

   // initialize source and destination indices to zero
   int sourceIndex = 0, destIndex = 0;

   // initialize other variables
   int accessResult;
   char strBuffer[ STD_STR_LEN ];

   // get whole op command as string
   accessResult = getLineTo( filePtr, STD_STR_LEN, SEMICOLON,
                                          strBuffer, IGNORE_LEADING_WS );

   // check for successful access
   if( accessResult == NO_ERR )
   {
      // assign op command letter to struct component
      inData->opLtr = strBuffer[ sourceIndex ];
   }

   // otherwise, assume unsuccessful access
   else
   {
      // set pointer to data structure to null
      inData = NULL;
      
      return OPCMD_ACCESS_ERR;
   }

   // verify op command letter
   switch( inData->opLtr )
   {
      // check for all correct cases
      case 'S':
      case 'A':
      case 'P':
      case 'M':
      case 'I':
      case 'O':
      
      break;

      // otherwise, assume not a correct case
      default:
         // set op command pointer to null
         inData = NULL;

         return CORRUPT_OPCMD_LETTER_ERR;
   }

   // loop until left paren found
   while( sourceIndex < STD_STR_LEN && strBuffer[ sourceIndex ] != LEFT_PAREN )
   {
      sourceIndex++;
   }

   // skip left paren element, increment source index
   sourceIndex++;

   // set op command text
   // loop until right paren found
   while( sourceIndex < STD_STR_LEN 
           && destIndex < MAX_OP_NAME_LENGTH
              && strBuffer [ sourceIndex ] != RIGHT_PAREN )
   {
      // acquire letter
      inData->opName[ destIndex ] = strBuffer[ sourceIndex ];
      
      destIndex++; sourceIndex++;

      // set end/null character to current end of string
      inData->opName[ destIndex ] = NULL_CHAR;
   }

   // check for incorrect op string
   if( checkOpString( inData->opName ) == False )
   {
      // set struct to null
      inData = NULL;
        
      return CORRUPT_OPCMD_NAME_ERR;
   }

   // skip right paren element - increment source index, reset dest index
   sourceIndex++;
   destIndex = 0;

   // get integer value, loop while digits are found
   while( sourceIndex < STD_STR_LEN
           && destIndex < MAX_OP_VALUE_LENGTH 
              && isDigit( strBuffer[ sourceIndex ] ) == True )
   {
      // multiply current buffer by ten
      intBuffer *= 10;

      // add next integer value, converted from character to integer
      intBuffer += (int) ( strBuffer[ sourceIndex ] - '0' );
        
      destIndex++; sourceIndex++;
   }

   // check for loop overrun failure, check specified lengths
   if( sourceIndex == STD_STR_LEN || destIndex == MAX_OP_VALUE_LENGTH )
   {
      // set struct to null
      inData = NULL;
      
      return CORRUPT_OPCMD_VALUE_ERR;
   }

   // set value to data structure component
   inData->opValue = intBuffer;

   // check for last op command "S(end)0"
   if( inData->opLtr == 'S'
        && compareString( inData->opName, "end" ) == STR_EQ )
   {
      return LAST_OPCMD_FOUND_MSG;
   }

   return COMPLETE_OPCMD_FOUND_MSG;
}

/*
Function name: updateStartCount
Algorithm: updates number of "start" op commands found in file
Precondition: count >= 0, op string has "start" or other op name
Postcondition: if op string has "start", input count +1 is returned;
               otherwise, input count is returned unchanged
Exceptions: none
Notes: none
*/
int updateStartCount( int count, char *opString )
{
   // check for "start" in op string, increment our count if so
   if( compareString( opString, "start" ) == STR_EQ )
   {
      return count + 1;
   }
 
   // return unchanged start count
   return count;
}

/*
Function name: updateEndCount
Algorithm: updates number of "end" op commands found in file
Precondition: count >= 0, op string has "end" or other op name
Postcondition: if op string has "end", input count +1 is returned;
               otherwise, input count is returned unchanged
Exceptions: none
Notes: none
*/
int updateEndCount( int count, char *opString )
{
   // check for "end" in op string, increment our count if so
   if( compareString( opString, "end" ) == STR_EQ )
   {
      return count + 1;
   }
 
   // return unchanged end count
   return count;
}

/* 
Function name: addNode
Algorithm: adds op command structure with data to a linked list
Precondition: linked list pointer assigned to null or to one op command link,
              struct pointer assigned to op command structure data
Postcondition: assigns new structure node to beginning of linked list,
               or end of established linked list
Exceptions: none
Notes: assumes memory access/availability
*/
OpCodeType *addNode( OpCodeType *localPtr, OpCodeType *newNode )
{
   // check for local pointer assigned to null
   if( localPtr == NULL )
   {
      // access memory for new link/node
      localPtr = ( OpCodeType *) malloc( sizeof( OpCodeType ) );

      // assign all three values to newly created node,
      // assign next pointer to null
      localPtr->opLtr = newNode->opLtr;
      copyString( localPtr->opName, newNode->opName );
      localPtr->opValue = newNode->opValue;
      localPtr->next = NULL;

      // return current local pointer
      return localPtr;
   }

   // assume end of list not found yet
   // assign recursive function to current's next link
   localPtr->next = addNode( localPtr->next, newNode );

   // return current local pointer
   return localPtr;
}

/*
Function name: checkOpString
Algorithm: check tested op string against list of possibles
Precondition: test op string is C-style string
              with potential op command name in it
Postcondition: in correct operation,
               verifies the test string with one
               of the potential op strings and returns true;
               otherwise, returns false
Exceptions: none
Notes: none
*/
Boolean checkOpString( char *testStr ) 
{
   // check all possible op names
   if( compareString( testStr, "access" )     == STR_EQ
     || compareString( testStr, "allocate" )   == STR_EQ
     || compareString( testStr, "end" )        == STR_EQ
     || compareString( testStr, "hard drive" ) == STR_EQ
     || compareString( testStr, "keyboard" )   == STR_EQ
     || compareString( testStr, "printer" )    == STR_EQ
     || compareString( testStr, "monitor" )    == STR_EQ
     || compareString( testStr, "run" )        == STR_EQ
     || compareString( testStr, "start" )      == STR_EQ )
   {
      return True;
   }
   
    return False; 
}

/*
Function name: isDigit
Algorithm: checks for character digit, returns result
Precondition: test value is character
Postcondition: if test value is a value '0' < value < '9',
               returns true, otherwise returns false
Exceptions: none
Notes: none
*/
Boolean isDigit( char testChar ) 
{
   // check for test character between characters '0' - '9' inclusive
   if( testChar >= '0' && testChar <= '9' )
   {
      return True;
   }

   // otherwise, assume character is not digit, return false
   return False; 
}

/*
Function name: displayMetaData
Algorithm: iterates through op code linked list,
           displays op code data individually
Precondition: linked list, with or without data
             (should not be called if no data)
Postcondition: displays all op codes in list
Exceptions: none
Notes: none
*/
void displayMetaData( OpCodeType *localPtr )
{
   // display title, with underline
   printf( "\nMeta-Data File Display\n" );
   printf( "======================\n" );

   // loop to end of linked list
   while( localPtr != NULL )
   {
      printf( "Op code letter: %c\n", localPtr->opLtr );
      printf( "Op code name: %s\n", localPtr->opName );
      printf( "Op code value: %d\n\n", localPtr->opValue );

      // assign local pointer to next node
      localPtr = localPtr->next;
   }
}

/*
Function name: displayMetaDataError
Algorithm: uses enum/constant values as indices to select display string,
           then displays error message with selected string
Precondition: integer input code has one of the enumerated code values
Postcondition: error message is displayed with the correct
               local error item
Exceptions: none
Notes: none
*/
void displayMetaDataError( int code ) 
{
   // create string error list, 10 items, max 35 letters
   // include 3 errors from StringManipError
   char errList[ 10 ][ 35 ] =
                   { "No Error",
                     "Incomplete File Error",
                     "Input Buffer Overrun",
                     "MD File Access Error",
                     "MD Corrupt Descriptor Error",
                     "Op Command Access Error",
                     "Corrupt Op Command Letter Error",
                     "Corrupt Op Command Name Error",
                     "Corrupt Op Command Value Error",
                     "Unbalanced Start End Code Error" };

   // display error to monitor with selected error string
   printf( "\nFATAL ERROR: %s, Program aborted\n", errList[ code ] );
}

/*
Function name: clearMetaDataList
Algorithm: recursively iterates through op code linked list,
           returns memory to OS from the bottom of the list upward
Precondition: linked list, with or without data
Postcondition: all node memory, if any, is returned to OS,
               return pointer (head) is set to null
Exceptions: none
Notes: none
*/
OpCodeType *clearMetaDataList( OpCodeType *localPtr )
{
    // check for local pointer not set to null (list not empty)
    if( localPtr != NULL )
   {
        // check for local pointer's next node not null
      if( localPtr->next != NULL )
      {
         // call recursive function with next pointer
         clearMetaDataList( localPtr->next );
      }

      // after recursive call, release memory to OS
      free( localPtr );
   }
   
   return NULL; 
}












