#ifndef SG_DRIVER_INCLUDED
#define SG_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.h
//  Description    : This is the declaration of the interface to the 
//                   ScatterGather driver (student code).
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 03 Sep 2020 01:26:06 PM PDT
//

// Includes
#include <sg_defs.h>

// Defines 

// Type definitions
typedef uint32_t SG_Magic;      // Magic value type

typedef char SG_DataIndicator;  // Data indicator type

struct SG_Packet_Buffer_t{
    SG_Magic magic;             // Magic number 1
    SG_Node_ID sendNodeId;      // The local node ID
    SG_Node_ID recvNodeId;      // The remote node ID
    SG_Block_ID blockID;        // The block ID
    SG_System_OP operation;     // The operation 
    SG_SeqNum sendSeqNo;        // The sender sequence number
    SG_SeqNum recvSeqNo;        // The receiver sequence number
    SG_DataIndicator indicator; // Data indicator
} __attribute__((packed));      // Avoid c automatic padding to retain data size
typedef struct SG_Packet_Buffer_t SG_Packet_Buffer;

typedef enum {
    SG_SERIALIZE    = 0,        // Processing serialized function
    SG_DESERIALIZE  = 1,        // Processing deserialized function
} SG_Process;

// Global interface definitions

// File system interface definitions

SgFHandle sgopen( const char *path );
    // Open the file for for reading and writing

int sgread( SgFHandle fh, char *buf, size_t len );
    // Read data from the file hande

int sgwrite( SgFHandle fh, char *buf, size_t len );
    // Write data to the file

int sgseek( SgFHandle fh, size_t off );
    // Seek to a specific place in the file

int sgclose( SgFHandle fh );
    // Close the file

int sgshutdown( void );
    // Shut down the filesystem

//
// Helper Functions
SG_Packet_Status check_serialize_sg_Data(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk, 
        SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq);

SG_Packet_Status check_deserialize_sg_Data(SG_Packet_Buffer buffer, SG_Node_ID *loc, SG_Node_ID *rem, 
        SG_Block_ID *blk, SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq);

int construct_sg_packet_buffer(SG_Packet_Buffer *buffer, SG_Magic magic_num, SG_Node_ID loc, SG_Node_ID rem, 
        SG_Block_ID blk, SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, SG_DataIndicator indicator);

int print_sg_packet_log_message(SG_Process process, SG_Packet_Status status);

SG_Packet_Status serialize_sg_packet( SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk, 
        SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data, char *packet, size_t *plen );
    // Serialize a ScatterGather packet (create packet)

SG_Packet_Status deserialize_sg_packet( SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk, 
        SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data, char *packet, size_t plen );
    // De-serialize a ScatterGather packet (unpack packet)

#endif