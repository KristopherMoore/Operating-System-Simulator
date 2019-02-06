//Program Information//////////////////////////////////////////////////////////////////////////////
/*
 * @file StringUtils.c
 *
 * @version 1.00
 *          Kristopher Moore (22 January 2019)
 *          Initial Program Build.
 */

#include "StringUtils.h"

// global constants
const int MAX_STR_LEN = 200;
const int STD_STR_LEN = 80;
const int SUBSTRING_NOT_FOUND = -101;
const int STR_EQ = 0;
const char NULL_CHAR = '\0';
const char SPACE = ' ';
const char COLON = ':';
const char SEMICOLON = ';';
const char PERIOD = '.';
const char LEFT_PAREN = '(';
const char RIGHT_PAREN = ')';

const Boolean IGNORE_LEADING_WS = True;
const Boolean ACCEPT_LEADING_WS = False;

/*
Function name: getStringLength
Algorithm: find the length of the string, up to the null character
Precondition: given C-style string with no character at end
Postcondtion: return the number of characters from the beginning
              up to the null character
Exceptions: none
Note: limit test loop to maximum characters for safety
*/
int getStringLength( char *testStr )
{
   int index = 0;

   // loop until null character OR limit
   while( index < MAX_STR_LEN && testStr[ index ] != NULL_CHAR )
   {
      // increment the counter index
      index++;
   }
   return index;
}

/*
Function Name: copyString
Algorithm: copies content of one string into another
Precondition: given C-style source string, having a null character (\'0')
              at end of string; destination string is passed in
              as a parameter with enoguh memory to accept the source string
Postcondtion: destination string contains an exact copy of the source string
Exceptions: none
Note: limit test loop to maximum characters for safety
*/
void copyString( char *destination, char *source )
{
   int index = 0;

    // loop until null character is found in source string
   while( index < MAX_STR_LEN && source[ index ] != NULL_CHAR )
   {
      // assign source character to destination element
      destination[ index ] = source[ index ];

      // increment index
      index++;

      // assign null character to next destination element
      destination[ index ] = NULL_CHAR;
   }
}

/*
Function name: concatenateString
Algorithm: concatenates or appends contents of one string
           onto the end of another
Precondition: given C-style source string, having a null character ('\0')
              at the end of string; destination string is passed in
              as a parameter with enough memory to accept the source string
Postcondition: destination string contains its original string with 
               the source string appended or concatenated to the end of it
Exceptions: none
Note: limit test loop to maximum characters for safety
*/
void concatenateString( char *destination, char *source )
{
   // initialize function/variables
    
   // set destination index to length of destination string
   int destIndex = getStringLength( destination );

   // set source index to zero
   int sourceIndex = 0;

    // loop until end of source index (null character)
   while( sourceIndex < MAX_STR_LEN && source[ sourceIndex ] != NULL_CHAR )
   {
      // assign source character to destination at destination index
      destination[ destIndex ] = source[ sourceIndex ];

      // increment source and destination indices
      sourceIndex++; destIndex++;

      // assign null character to next destination element
      destination[ destIndex ] = NULL_CHAR;
   }    
}

/*
Function name: compareString
Algorithm: compares two strings alphabetically such that:
           if oneStr < otherStr, the function returns a value < 0
           if oneStr > otherStr, the function returns a value > 0
           if oneStr == otherStr, the function returns a 0
           if two strings are identical up to the point that one is longer,
           the difference in lengths will be returned
Precondition: given two C-style strings, having a null character (\'0')
              at end of each string
Postcondition: integer value returned as specified
Exceptions: none
Note: limit test loop to maximum character for safety
*/
int compareString( char *oneStr, char *otherStr )
{
   int index = 0;
   int difference;

   // loop to end of the two strings
   while( index < MAX_STR_LEN 
           && oneStr[ index ] != NULL_CHAR 
           && otherStr[ index ] != NULL_CHAR )
   {
      // find the difference between the currently aligned characters
      difference = oneStr[ index ] - otherStr[ index ];

      // check for non-zero difference
      if( difference != 0 )
      {
            return difference;
      }
      index++;
       
   }
   
   // assume strings are equal at this point, return string length difference
   return getStringLength( oneStr ) - getStringLength( otherStr );
}

/*
Function name: getSubString
Algorithm: captures sub string within larger string
           between two inclusive indices
Precondition: given a C-style source string, having a null character (\'0')
              at end of string, and another string parameter
              with enough memory to hold the resulting substring
Postcondtion: substring is returned as a parameter
Exeptions: empty string returned if any of the index parameters
           are out of range
Note: copy of source string is made internally to protect from aliasing
*/
void getSubString( char *destStr, char *sourceStr, 
                                      int startIndex, int endIndex )
{
   // set length of source string
   int sourceStrLen = getStringLength( sourceStr );

   // initialize destination index to zero
   int destIndex = 0;

   // initialize source index to start index parameter
   int sourceIndex = startIndex;

   // create pointer to temp string
   char *tempSourceStr;

   // check for indices within limits
   if( startIndex >= 0 && startIndex <= endIndex && endIndex < sourceStrLen )
   {
      // create temporary string, copy source string to it
      tempSourceStr = (char *) malloc( sourceStrLen + 1 );
      copyString( tempSourceStr, sourceStr );

      // loop across requested substring (indices)
      while( sourceIndex <= endIndex )
      {  
         // assign source character to destination element
         destStr[ destIndex ] = tempSourceStr[ sourceIndex ];

         // increment indices
         destIndex++; sourceIndex++;

         // add null character to next destination string element
         destStr[ destIndex ] = NULL_CHAR;
      }
      // release memory used for temp source string
      free( tempSourceStr );
   }

   // otherwise, assume indices not in limits
   else
   {
      // create empty string with null character
      destStr[ 0 ] = NULL_CHAR;
   }
}

/*
Function name: findSubString
Algorithm: linear search for given substring within a given text string
Precondition: given a C-style source string, having a null character (\'0')
              at end of string, and given search string with
              a null character ('\n') at the end of that string
Postcondtion: index of substring location returned,
              or SUBSTRING_NOT_FOUND constant is returned
Exeptions: none
Note: none
*/
int findSubString( char *testStr, char *searchSubStr )
{
   // initialize test string length
   int testStrLen = getStringLength( testStr );

   // initialize master index - location of sub string start point
   int masterIndex = 0;

   int searchIndex, internalIndex;

    // loop across test string
   while( masterIndex < testStrLen )
   {
      // set internal loop index to current test string index
      internalIndex = masterIndex;

      // set internal search index to zero
      searchIndex = 0;

      // loop to end of test string
      // while test string/ substring characters are the same
      while( internalIndex <= testStrLen 
               && testStr[ internalIndex ] == searchSubStr[ searchIndex ] )
      {
         // increment test string, substring indices
         internalIndex++; searchIndex++;

         // check for end of substring (search completed)
         if( searchSubStr[ searchIndex ] == NULL_CHAR )
         {
            // return current test string index
            return masterIndex;
         }
      }
      // increment current test index
      masterIndex++;
   }
   
   // assume tests have failed at this point, return SUBSTRING_NOT_FOUND
   return SUBSTRING_NOT_FOUND;
}

/*
Function name: setStrToLowerCase
Algorithm: iterates through string, set all upper case letters
           to lower case without changing any other characters
Precondition: given a C-style source string, having a null character (\'0')
              at end of string, and destination string parameter
              is passed with enough memory to hold the lower case string
Postcondtion: all upper case letters in given string are set
              to lower case; no change to any other characters
Exeptions: limit on string loop in case incorrect string format
Note: copy of source string is made internally to protect from aliasing
*/
void setStrToLowerCase( char *destStr, char *sourceStr )
{
   // create temporary source string
   int strLen = getStringLength( sourceStr );
   char *tempStr = (char *) malloc( strLen + 1 );

   // initialize source string index to zero
   int index = 0;

    // copy source string to temporary string
    copyString( tempStr, sourceStr );

    // loop to end of temp/source string
   while( index < MAX_STR_LEN && tempStr[ index ] != NULL_CHAR )
   {
      // change letter to lower case as needed and assign to destination string
      destStr[ index ] = setCharToLowerCase( tempStr[ index ] );

      // increment index
      index++;

      // add null character to next destination string element
      destStr[ index ] = NULL_CHAR;
   }

    // release temporary string memory
    free( tempStr );
}

/*
Function name: setCharToLowerCase
Algorithm: tests character parameter for upper case, changes it to lower case, 
           makes no changes if not upper case
Precondition: given character value
Postcondition: upper case letter is set to lower case,
               otherwise, character is unchanged
Exceptions: none
Note: none
*/
char setCharToLowerCase( char testChar )
{
   // check for character between 'A'and 'Z' inclusive
   if( testChar >= 'A' && testChar <= 'Z' )
   {
      // convert upper case letter to lower case
      testChar = (char) (testChar - 'A' + 'a' );
   }
   
    return testChar;
}

/*
Function name: getLineTo
Algorithm: finds given text in file, skipping white space if specified,
           stops searching at given character or length
Precondition: file is open with valid file pointer;
              char buffer has adequete memory for data
              stop character and length are valid
Postcondition: ignores leading white space if specified;
               captures all printable characters and stores in string buffer;
Exceptions: returns INCOMPLETE_FILE_ERR if no valid data found;
            return NO_ERR if successful operation
Note: none
*/
int getLineTo( FILE *filePtr, int bufferSize, char stopChar,
                          char *buffer, Boolean omitLeadingWhiteSpace )
{
   // initialize character index
   int charIndex = 0;

   // initialize status return to NO_ERR
   int statusReturn = NO_ERR;

   // initialize buffer size available flag to true
   Boolean bufferSizeAvailable = True;

   // initialize other variables
   int charAsInt;

   // get the first character
   charAsInt = fgetc( filePtr );

   // use a loop to consume leading white space, if flagged
   while( omitLeadingWhiteSpace == True
           && charAsInt != (int) stopChar
           && charIndex < bufferSize
           && charAsInt <= (int) SPACE )
   {
      // get next character (as integer)
      charAsInt = fgetc( filePtr );
   }
   
   // capture string
   // loop while character is not stop character and buffer size is available
   while( charAsInt != (int) stopChar && bufferSizeAvailable == True )
   {
      // check for input failure
      if( isEndOfFile( filePtr ) == True )
      {
         // return incomplete file error
         return INCOMPLETE_FILE_ERR;
      }

      // check for usable (printable) character
      if( charAsInt >= (int) SPACE )
      {
         // assign input character to buffer string
         buffer[ charIndex ] = (char) charAsInt;
         
         charIndex++;
      }

      // set next buffer element to null character
      buffer[ charIndex ] = NULL_CHAR;

      // check for not at end of buffer size
      if( charIndex < bufferSize - 1 )
      {
         // get a new character
         charAsInt = fgetc( filePtr );
      }

      // otherwise, assume end of buffer size
      else
      {
         // set buffer size Boolean to false end loop
         bufferSizeAvailable = False;

         // set status return to buffer overrun error
         statusReturn = INPUT_BUFFER_OVERRUN_ERR;
      }
   }
   
    return statusReturn;
}

/*
Function name: isEndOfFile
Algorithm: reports end of file, using eof
Precondition: file is open with valid file pointer
Postcondition: reports end of file
Exceptions: none
Note: none
*/
Boolean isEndOfFile( FILE *filePtr )
{
   // check for feof end of file response
   if( feof( filePtr ) != 0 )
   {
      return True;
   }

   // assume test failed at this point, return false
   return False;
}











