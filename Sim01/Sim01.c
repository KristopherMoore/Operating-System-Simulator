//Program Information//////////////////////////////////////////////////////////////////////////////
/*
 * @file Sim01.c
 *
 * @version 1.00
 *          Kristopher Moore (22 January 2019)
 *          Initial Program Build.
 */

#include <stdio.h>
#include "ConfigAccess.h"
#include "MetaDataAccess.h"

/*
Function name: main
Algorithm: driver function to test config and metadata file
           upload operation together
Precondition: none
Postcondition: returns zero (0) on success
Exceptions: none
Notes: demonstrates use of combined files
*/
int main( int argc, char **argv )
{

   // initialization
   int configAccessResult, mdAccessResult;
   char configFileName[ MAX_STR_LEN ];
   char mdFileName[ MAX_STR_LEN ];
   ConfigDataType *configDataPtr;
   OpCodeType *mdData;

   // display to user, the Config Upload is to be performed.
   printf( "\nConfig File Upload Program\n" );
   printf( "==========================\n\n" );

   // check for not correct number of command line arguments (two)
   if (argc < 2 )
   {
       // print missing command line argument error
       printf( "ERROR: Program requires file name for config file " );
       printf( "as command line argument\n" );
       printf( "Program Terminated\n" );

       // return 1, to signify our program exited abnormally
       return 1;
   }

   // grab our data from the supplied configuration file
   copyString( configFileName, argv[ 1 ] );
   configAccessResult = getConfigData( configFileName, &configDataPtr );

   // check for successful upload
   if( configAccessResult == NO_ERR )
   {
      // display data returned from config upload.
      displayConfigData( configDataPtr );
   }

   // otherwise, assume failed upload
   else
   {
       // display configuration upload error and exit
      displayConfigError( configAccessResult );
      return 0;
   }

   // display component title
   printf( "\nMeta Data File Upload Component\n" );
   printf( "======================\n" );

   // get data from meta data file
   copyString( mdFileName, configDataPtr->metaDataFileName );
   mdAccessResult = getOpCodes( mdFileName, &mdData );

   // check for successful upload
   if( mdAccessResult == NO_ERR )
   {
      // display meta data file
      displayMetaData( mdData );
   }

   // otherwise, assume unsuccessful upload
   else
   {
      // display meta data error message and exit
      displayMetaDataError( mdAccessResult );
      return 0;
   }

   // shut down, clean up program

   // clear configuration data
   clearConfigData( &configDataPtr );

   // add endline for vertical spacing
   printf( "\n" );

   // clear meta data
   mdData = clearMetaDataList( mdData );

   // add endline for vertical spacing
   printf( "\n" );

   // return successful exit of our program
   return 0;
}
