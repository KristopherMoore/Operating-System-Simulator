//Program Information///////////////////////////////////////////////////////////
/*
 * @file Sim02.c
 *
 * @version 1.00
 *          Kristopher Moore (22 January 2019)
 *          Initial Program Build.
 */

#include <stdio.h>
#include "ConfigAccess.h"
#include "MetaDataAccess.h"
#include "SimRunner.h"

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
   
   //display Simulator initialization header
   printf( "\nSimulator Program\n" );
   printf( "==========================\n\n" );

   //display start of config upload
   printf( "\nUploading Configuration Files\n" );

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

   // check for unsuccessful upload
   if( configAccessResult != NO_ERR )
   {
      // display configuration upload error and exit
      displayConfigError( configAccessResult );
      return 0;
   }

   //display start of meta data upload
   printf( "\nUploading Meta Data Files\n\n" );

   // get data from meta data file
   copyString( mdFileName, configDataPtr->metaDataFileName );
   mdAccessResult = getOpCodes( mdFileName, &mdData );

   // check for unsuccessful upload
   if( mdAccessResult != NO_ERR )
   {
      // display meta data error message and exit
      displayMetaDataError( mdAccessResult );
      return 0;
   }
   
   //If we reach this point, we have our correct files, so we can begin sim
   simulationRunner( configDataPtr, mdData);
   
   
   // shut down, clean up program
   // clear configuration data
   clearConfigData( &configDataPtr );
   printf( "\n" );

   // clear meta data
   mdData = clearMetaDataList( mdData );
   printf( "\n" );

   // return successful exit of our program
   return 0;
}
