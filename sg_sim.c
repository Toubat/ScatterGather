////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_sim.c
//  Description    : This is the main program for the CMPSC311 programming
//                   assignment #2 (beginning of ScatterGather.com Internet
//                   service code)
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 03 Sep 2020 12:22:02 PM PDT
//

// Include Files
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <cmpsc311_assocarr.h>
#include <cmpsc311_workload.h>

// Project Includes 
#include <sg_defs.h>
#include <sg_driver.h>

// Defines
#define SG_ARGUMENTS "hvul:"
#define USAGE \
	"USAGE: sg_sim [-h] [-v] [-l <logfile>] <workload>\n" \
	"\n" \
	"where:\n" \
	"    -h - help mode (display this message)\n" \
	"    -v - verbose output\n" \
	"    -u - perform the unit tests\n" \
	"    -l - write log messages to the filename <logfile>\n" \
	"and\n" \
	"    workload - is the name of the workload file.  Not that this\n" \
	"               file is not needed when running the unit tests.\n" \
	"\n" \

//
// Global Data
int verbose;
unsigned long SGServiceLevel; // Service log level
unsigned long SGDriverLevel; // Controller log level
unsigned long SGSimulatorLevel; // Simulation log level

//
// Functional Prototypes

int simulateScatterGather( char *wload ); // ScatterGather simulation
int sg_unit_test( void ); // The program unit tests
extern int packetUnitTest( void ); // External function (packet processing)

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the ScatterGather simulator
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main( int argc, char *argv[] ) {

	// Local variables
	int ch, verbose = 0, log_initialized = 0, unit_tests = 0;
	
	// Process the command line parameters
	while ((ch = getopt(argc, argv, SG_ARGUMENTS)) != -1) {

		switch (ch) {
		case 'h': // Help, print usage
			fprintf( stderr, USAGE );
			return( -1 );

		case 'v': // Verbose Flag
			verbose = 1;
			break;

		case 'u': // Unit test Flag
			unit_tests = 1;
			break;

		case 'l': // Set the log filename
			initializeLogWithFilename( optarg );
			log_initialized = 1;
			break;

		default:  // Default (unknown)
			fprintf( stderr, "Unknown command line option (%c), aborting.\n", ch );
			return( -1 );
		}
	}

	// Setup the log as needed, log levels
	if ( ! log_initialized ) {
		initializeLogWithFilehandle( CMPSC311_LOG_STDERR );
	}
	SGServiceLevel = registerLogLevel("SG_SERVICE", 0); // Service log level
	SGDriverLevel = registerLogLevel("SG_DRIVER", 0); // Controller log level
	SGSimulatorLevel = registerLogLevel("SG_SIMULATOR", 0); // Simulation log level
	if ( verbose ) {
		enableLogLevels( LOG_INFO_LEVEL );
		enableLogLevels(SGServiceLevel | SGDriverLevel | SGSimulatorLevel);
	}

	// If exgtracting file from data
	if (unit_tests) {

		// Run the unit tests
		enableLogLevels( LOG_INFO_LEVEL );
		logMessage(LOG_INFO_LEVEL, "Running unit tests ....");
		if (sg_unit_test() == 0) {
			logMessage(LOG_INFO_LEVEL, "Unit tests completed successfully.\n\n");
		} else {
			logMessage(LOG_ERROR_LEVEL, "Unit tests failed, aborting.\n\n");
		}

	} else {

		// The filename should be the next option
		if ( argv[optind] == NULL ) {
			fprintf( stderr, "Missing command line parameters, use -h to see usage, aborting.\n" );
			return( -1 );
		}

		// Run the simulation
		if ( simulateScatterGather(argv[optind]) == 0 ) {
			logMessage( LOG_INFO_LEVEL, "ScatterGather.com simulation completed successfully!!!\n\n" );
		} else {
			logMessage( LOG_INFO_LEVEL, "ScatterGather.com simulation failed.\n\n" );
		}
	}

	// Return successfully
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : simulateScatterGather
// Description  : The main control loop for the processing of the SG
//                simulation (which calls the student code).
//
// Inputs       : wload - this is the workload filename
// Outputs      : 0 if successful test, -1 if failure

int simulateScatterGather( char *wload ) {

	/* Local types */
	typedef struct {
		char 	   *filename;
		SgFHandle 	fhandle;
		int         pos;
	} fsysdata;

    /* Local variables */
    workload_state state;
    workload_operation operation;
	SgFHandle fh;
	AssocArray fhTable;
	char buf[10240];
	int opens, reads, writes, seeks, closes;
	fsysdata *fdata;

	/* Initalize the local data and simulation */
	if ( init_assoc(&fhTable, stringCompareCallback, pointerCompareCallback) ) {
		logMessage( LOG_ERROR_LEVEL, "CMPSC311 SG init failed." );
		return( -1 );
	}

	/* Open the workload for processing */
	if ( openCmpsc311Workload(&state, wload) ) {
		logMessage( LOG_ERROR_LEVEL, "CMPSC311 SG workload: failed opening workload [%s]", wload );
		return( -1 );        
	}

	/* Loop until we are done with the workload */
	logMessage( SGSimulatorLevel, "CMPSC311 SG : executing workload [%s]", state.filename );
	do {

		/* Get the next operation to process */
		if ( readCmpsc311Workload(&state, &operation) ) {
			logMessage( LOG_ERROR_LEVEL, "CMPSC311 workload unit test failed at line %d, get op", state.lineno );
			return( -1 );
		}

		/* Verbose log the operation */
		if ( (operation.op == WL_READ) || (operation.op == WL_WRITE) ) {
			logMessage( SGSimulatorLevel, "CMPSCS311 workload op: %s %s off=%d, sz=%d [%.10s <more data follows>]", operation.objname,
				workload_operations_strings[operation.op], operation.pos, operation.size, operation.data );
		} else {
			logMessage( SGSimulatorLevel, "CMPSCS311 workload op: %s %s", operation.objname, 
				workload_operations_strings[operation.op] );
		}

		/* Switch on the operation type */
		switch ( operation.op ) {

			case WL_OPEN: /* Open the file for reading/writing, check error */

				/* Open the file for reading */
				if ( (fh = sgopen(operation.objname)) == -1 ) {
					logMessage( LOG_ERROR_LEVEL, "SG error opening file [%s], aborting", operation.objname );
					return( -1 );
				}

				/* Setup the structure */
				fdata = malloc( sizeof(fsysdata) );
				fdata->filename = strdup( operation.objname );
				fdata->fhandle = fh;
				fdata->pos = 0;

				/* Insert the file into the table */
				insert_assoc( &fhTable, fdata->filename, fdata );
				logMessage( SGSimulatorLevel, "SG Open file [%s]", fdata->filename );
				opens ++;
				break;

			case WL_READ: /* Read a block of data from the file */

				/* Find the file for processing */
				if ( (fdata = find_assoc(&fhTable, operation.objname)) == NULL ) {
					logMessage( LOG_ERROR_LEVEL, "SG error reading unknown file [%s], aborting", 
						operation.objname );
					return( -1 );
				}

				/* If the position within the file is not a read location, seek */
				if ( fdata->pos != operation.pos ) {
					if ( sgseek(fdata->fhandle, operation.pos) != operation.pos ) {
						logMessage( LOG_ERROR_LEVEL, "SG error seek failed [%s, pos=%d], aborting", 
							operation.objname, operation.pos );
						return( -1 );
					}
					fdata->pos = operation.pos;
					seeks ++;
				}

				/* Now do the read from the file */
				if ( sgread(fdata->fhandle, buf, operation.size) != operation.size ) {
					logMessage( LOG_ERROR_LEVEL, "SG error read failed [%s, pos=%d, size=%d], aborting", 
						operation.objname, operation.pos, operation.size );
					return( -1 );
				}

				/* Compare the data read with that in the workload data */
				if ( strncmp(buf, operation.data, operation.size) != 0 ) {
					logMessage( LOG_ERROR_LEVEL, "SG read data compare failed, aborting" );
					logMessage( LOG_ERROR_LEVEL, "Read data     : [%s]", buf );
					logMessage( LOG_ERROR_LEVEL, "Expected data : [%s]", operation.data );
					return( -1 );
				}

				/* Now increment the file position, log the data */
				fdata->pos += operation.size;
				logMessage( SGSimulatorLevel, "Correctly read from [%s], %d bytes at position %d", 
					fdata->filename, operation.size, operation.pos );
				reads ++;
				break;

			case WL_WRITE: /* Write a block of data to the file */

				/* Find the file for processing */
				if ( (fdata = find_assoc(&fhTable, operation.objname)) == NULL ) {
					logMessage( LOG_ERROR_LEVEL, "SG error writing unknown file [%s], aborting", 
						operation.objname );
					return( -1 );
				}

				/* If the position within the file is not a read location, seek */
				if ( fdata->pos != operation.pos ) {
					if ( sgseek(fdata->fhandle, operation.pos) != operation.pos ) {
						logMessage( LOG_ERROR_LEVEL, "SG error seek failed [%s, pos=%d], aborting", 
							operation.objname, operation.pos );
						return( -1 );
					}
					fdata->pos = operation.pos;
					seeks ++;
				}

				/* Now do the write to the file */
				if ( sgwrite(fdata->fhandle, operation.data, operation.size) != operation.size ) {
					logMessage( LOG_ERROR_LEVEL, "SG error write failed [%s, pos=%d, size=%d], aborting", 
						operation.objname, operation.pos, operation.size );
					return( -1 );
				}

				/* Now increment the file position, log the data */
				fdata->pos += operation.size;
				logMessage( SGSimulatorLevel, "Wrote data to file [%s], %d bytes at position %d", 
					fdata->filename, operation.size, operation.pos );
				writes ++;
				break;

			case WL_CLOSE:

				/* Find the file for processing */
				if ( (fdata = find_assoc(&fhTable, operation.objname)) == NULL ) {
					logMessage( LOG_ERROR_LEVEL, "SG error closing unknown file [%s], aborting", 
						operation.objname );
					return( -1 );
				}

				/* Now close the file */
				if ( sgclose(fdata->fhandle) != 0 ) {
					logMessage( LOG_ERROR_LEVEL, "SG error close failed [%s, pos=%d, size=%d], aborting", 
						operation.objname, operation.pos, operation.size );
					return( -1 );
				}

				/* Remove file from file handle table, clean up structures, log */
				logMessage( SGSimulatorLevel, "Closed file [%s].", fdata->filename );
				delete_assoc( &fhTable, fdata->filename );
				free( fdata->filename );
				free( fdata );
				closes ++;
				break;

			case WL_EOF: // End of the workload file
				if ( sgshutdown() ) {
					logMessage( LOG_ERROR_LEVEL, "SG shutdown failed" );
					return( -1 );
				}
				logMessage( SGSimulatorLevel, "End of the workload file (processed)" );
				break;

			default: /* Unknown oepration type, bailout */
				logMessage( LOG_ERROR_LEVEL, "Scatter/gather bad operation type [%d]", operation.op );
				return( -1 );

		}

	} while ( operation.op < WL_EOF );
	
	/* Log, close workload and delete the local file, return successfully  */
	closeCmpsc311Workload( &state );
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sg_unit_test
// Description  : This is the interface for the global unit test function
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int sg_unit_test( void ) {

    // Print a lead in message
    logMessage( LOG_INFO_LEVEL, "ScatterGather: beginning unit tests ..." );

    // Do the UNIT tests
    if ( packetUnitTest() ) {
        logMessage( LOG_ERROR_LEVEL, "ScatterGather: unit tests failed." );
        return( -1 );
    }

    // Return successfully
    logMessage( LOG_INFO_LEVEL, "ScatterGather: exiting unit tests." );
    return( 0 );
}
