#ifndef CMPSC311_WORKLOAD
#define CMPSC311_WORKLOAD

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cmpsc311_workload.h
//  Description    : This is the header file for the implementation of the
//                   workload generator for the 311 projects.  It is intended
//                   to be used for various implementations of storage devices
//                   used for the semester projects.
//
//  Author         : Patrick McDaniel
//  Change Log     :
//
//     11/12/19 - Created the intial version of the workload implementation
//

// Includes
#include <stdint.h>

// 
// Definitions
#define CMPSC311_MAX_SPECIFICATION_TOKENS 12
#define CMPSC311_MAX_OPSIZE_DEFAULT 64
#define CMPSC311_MAX_OPSIZE_MAXIMUM 10240
#define CMPSC311_NUMOPS_DEFAULT 256
#define CMPSC311_NOLOCALITY (uint32_t)0xffffffff
#define WL_MAX_OBJS 4096

// Type Definitions

/* Workload object states */
typedef enum workload_state_enum {

    WLO_UNINITIALIZED = 0, // The workload has not been initizlied
    WLO_CREATING      = 1, // workload being created into its initial state
    WLO_PROCESSING    = 2, // workload is open and processing
    WLO_FINISHING     = 3, // workload is in the completion state
    WLO_COMPLETED     = 4, // The workload is completed
    WLO_MAX_STATUS    = 5 

} workload_state_enum;
extern const char * workload_state_strings[WLO_MAX_STATUS];

/* Structure used for maintaining the file state information */
typedef struct object_type_struct {
    
    /* Identifier information */
    char *    object_name;   // This is the object name (if using object names)
    uint32_t  object_id;     // This is the object identifier

    /* State information */
    workload_state_enum state;    // The state of the 
    char *    content_state; // This is the current state of the objectc content
    size_t    content_size;  // This is the current size of the object content
    size_t    content_pos;   // The current position of the "read" head

    /* The final state expected of the object when completed */
    char *    final_state;   // This is the final state of the object
    size_t    final_size;    // This is the final size of the object
    size_t    final_blocksz; // This is the block size for finalization
    char *    object_hash;   // This is a MD5 hash of the object

} object_type;

/* Types of workloads */
typedef enum workload_load_type_enum {

    WLT_UNKNOWN           = 0,  // Unknown workload type
    WLT_LINEAR            = 1,  // Linear (read each object front back)
    WLT_RANDOM            = 2,  // Random (read objects randomly)
    WLT_LOCALITY          = 3,  // Reference locality 
    WLT_MAX_WORKLOAD_TYPE = 4

} workload_load_type;

/* Workload object operations, strings */
typedef enum workload_operations_type_enum {

   WL_OPEN                  = 0, // Open the object for later processing
   WL_WRITE                 = 1, // Write the object data
   WL_READ                  = 2, // Read the object data
   WL_CLOSE                 = 3, // Close the object
   WL_EOF                   = 4, // End of workload
   WLT_MAX_WORKLOAD_OP_TYPE = 5 // Max type

} workload_operations_type;
extern const char * workload_operations_strings[WLT_MAX_WORKLOAD_OP_TYPE];

/* Previous operations structure */
typedef struct workload_last_ops_struct {

    workload_operations_type optype; // The operation performned
    uint32_t                 object; // The object last used
    uint32_t                 offset; // The offset into the object
    uint32_t                 size;   // The size of the operation
    uint32_t                 clock;  // (Virtual) time of the operation

} workload_last_ops;

/* Workload structure*/
typedef struct workload_type_struct {

    /* Workload flags */
    uint8_t            initialized;      // Initialized flag
    uint8_t            overwrite;        // over-write the workload files?

	/* Workload identifier information */
	char *             workload_name;    // The name of the workload
	char *             workload_output;  // The filename for output
	
	/* Workload parameters */
	workload_load_type wltype;           // The type of workload 
	uint32_t           wl_locweight;     // The locality weight (loc only)
	uint32_t           wl_locsize;       // The locality cache size (loc only)
	uint32_t           workload_ops;     // Number of operations
    uint32_t           workload_maxopsz; // maximum operation size (read/writes)
    uint8_t            workload_fixed;   // The workload reads/writes in blocks

	/* Workload objects */
	object_type *      objects[WL_MAX_OBJS]; // The objects to process
	uint32_t           num_objects;      // Number of created objects

	/* Workload state */
    workload_state_enum     state;            // The current state of the workload
    FILE              *fhandle;          // The file handle for the output
	uint32_t           next_oid;         // The next available object ID
    uint32_t           op_counter;       // The operations counter

    /* Saved operations (for reference locality) */
    workload_last_ops *last_ops;         // The last operations 

} workload_type;

/* Workload reading state */
typedef struct {

    char *             filename;         // The filename of the workload
    FILE *             fhandle;          // The file handle for the workload
    uint32_t           lineno;           // The line number of the workload
    uint32_t           errored;          // Flag indicating the workload errored

} workload_state;

/* Workload operation */
typedef struct {

    char     objname[128];            // The object name
    workload_operations_type  op;     // The operation performed
    size_t                    pos;    // Position in the object
    size_t                    size;   // Size of the operation
    char     data[CMPSC311_MAX_OPSIZE_MAXIMUM];   // The data for the operation

} workload_operation;

/*
// Workload Interfaces */

int createCmpsc311Workload( char *specification );
	// Run the workloads on the specification in the file

// Workload processing information
int openCmpsc311Workload( workload_state *state, const char *wlname );
int readCmpsc311Workload( workload_state *wload, workload_operation *opn );
int closeCmpsc311Workload( workload_state *wload );

/*
// Workload internal functions */

int processWorkloadLine( workload_type *wld, char *line, char *tokens[], int numtoks );
int processWorkloadParameter( workload_type *wld, char *param, char *val1, char *val2, char *val3 );
int processWorkloadObjects( workload_type *wld, char *typ, char *val1, char *val2, char *val3, char *val4 );
object_type * createObjectFromFile( char *fname, uint32_t oid );
object_type * createRandomWorkloadObject( workload_type *wld, uint32_t minsz, uint32_t maxsz, char *basenm, uint32_t oid );
int executeWorkload( workload_type *wld );
int selectWorkloadObject( workload_type *wld, uint32_t *objp, uint32_t *locp );
int processObjectWorkload( workload_type *wld, uint32_t obj, uint32_t loc );
int processObjectWorkloadProcessing( workload_type *wld, uint32_t obj, uint32_t loc );
int generateObjectOperation(workload_type *wld, uint32_t obj, workload_operations_type op, size_t off, size_t len, char *dat );
int cleanupWorkload( workload_type *wld );

//
// Unit test

int cmpsc311WorkloadUnitTest(void);
	// Run a unit test checking the workload implementation

#endif
