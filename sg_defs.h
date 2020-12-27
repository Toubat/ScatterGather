#ifndef SG_DEFS_INCLUDED
#define SG_DEFS_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_defs.h
//  Description    : This is the declaration of the basic data types and 
//                   global data for the ScatterGather implementation.
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 03 Sep 2020 12:22:02 PM PDT
//

// Includes
#include <cmpsc311_log.h>

// Defines 
#define SG_BLOCK_SIZE 1024
#define SG_MAX_BLOCKS_PER_FILE 264
#define SG_MAGIC_VALUE (uint32_t)0xfefe
#define SG_BLOCK_UNKNOWN ((uint32_t)-1)
#define SG_NODE_UNKNOWN ((uint32_t)-1)
#define SG_SEQNO_UNKNOWN ((uint16_t)-1)
#define SG_INITIAL_SEQNO 10000

// The basic ScatterGather packet size
#define SG_BASE_PACKET_SIZE (\
        sizeof(uint32_t) +        /* magic number */ \
        (sizeof(SG_Node_ID)*2) +  /* loc/rem nid */ \
        sizeof(SG_Block_ID) +     /* block id */ \
        sizeof(SG_System_OP) +    /* operation */ \
        (sizeof(SG_SeqNum)*2) +   /* snd/rec seqno */ \
        sizeof(char) +            /* data len */ \
        sizeof(uint32_t)          /* magic number */ ) 

// The basic ScatterGather packet size WITH block
#define SG_DATA_PACKET_SIZE (SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE)

// Type definitions
typedef int32_t SgFHandle;
typedef uint64_t SG_Node_ID;   // The type for node identifiers
typedef uint64_t SG_Block_ID;  // Unique block identifier
typedef uint16_t SG_SeqNum;    // A sender receiver sequence number
typedef char SGDataBlock[SG_BLOCK_SIZE] ; // A data block

// System operations
typedef enum {
    SG_INIT_ENDPOINT   = 0,   // Intialize the endpoint
    SG_STOP_ENDPOINT   = 1,   // Stop the driver, shutdown
    SG_CREATE_BLOCK    = 2,   // Create a block
    SG_UPDATE_BLOCK    = 3,   // Update a block
    SG_OBTAIN_BLOCK    = 4,   // Get the block (or be redirected)
    SG_DELETE_BLOCK    = 5,   // Delete the block
    SG_MAXVAL_OP       = 6    // Maximum value of the opcode
} SG_System_OP;  

// A packet information structure 
typedef struct {
    SG_Node_ID     locNodeId;  // The local node ID
    SG_Node_ID     remNodeId;  // The remote node ID
    SG_Block_ID    blockID;    // The block ID
    SG_System_OP   operation;  // The operation 
    SG_SeqNum      sendSeqNo;  // The sender sequence number
    SG_SeqNum      recvSeqNo;  // The receiver sequence number
    SGDataBlock   *data;       // The data block (or NULL)
} SG_Packet_Info;

// Status values for the packet processing
typedef enum {
    SG_PACKT_OK        = 0,  // The packet processing is good
    SG_PACKT_LOCID_BAD = 1,  // Local ID is bad
    SG_PACKT_REMID_BAD = 2,  // Remote ID is bad
    SG_PACKT_BLKID_BAD = 3,  // Block ID is bad
    SG_PACKT_OPERN_BAD = 4,  // Operation is bad
    SG_PACKT_SNDSQ_BAD = 5,  // Sender sequence num bad
    SG_PACKT_RCVSQ_BAD = 6,  // Receiver sequence num bad
    SG_PACKT_BLKDT_BAD = 7,  // The block data is bad (NULL)
    SG_PACKT_BLKLN_BAD = 8,  // The block length is bad
    SG_PACKT_PDATA_BAD = 9,  // The packet data is bad
} SG_Packet_Status;

// Global data declarations
extern unsigned long SGServiceLevel; // Service log level
extern unsigned long SGDriverLevel; // Controller log level
extern unsigned long SGSimulatorLevel; // Simulation log level

#endif