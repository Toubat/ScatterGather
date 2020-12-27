////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : Boquan Yin
//   Last Modified : 1
//

// Include Files
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Project Includes
#include <sg_driver.h>
#include <sg_service.h>
#include <sg_cache.h>

// define

// Type definitions
typedef struct{
    SgFHandle fHandle;
    int fPointer;
    int fSize;
    int numBlocks;
    int open;  
    SG_Block_ID * blockID;
    SG_Node_ID * remNodeID;
} SG_File;

typedef struct{
    char ** fPaths;          // store file names
    SG_File ** files;        // store files
} SG_File_Map;

// Global Data
SG_File_Map sgFileMap;
SgFHandle nextFHandle = 0;   // index of next file handle to assign
int fileSize;                // total allocated number of files

SG_Node_ID * remNodeIDs;
SG_SeqNum * remSeqNums;
int length;                  // total allocated number of node-sequence pairs
int next = 0;                // index of next node-sequence pair to assign
// Driver file entry

// Global data
int sgDriverInitialized = 0; // The flag indicating the driver initialized
SG_Block_ID sgLocalNodeId;   // The local node identifier
SG_SeqNum sgLocalSeqno;      // The local sequence number

// Driver support functions
int sgInitEndpoint( void ); // Initialize the endpoint

// Functions
SG_SeqNum find(SG_Node_ID remNodeID);

int put(SG_Node_ID remNodeID, SG_SeqNum remSeqNum);

int update(SG_Node_ID remNodeID, SG_SeqNum remSeqNum);

int mallocBlockPerFile(SgFHandle fh);

// File system interface implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

SgFHandle sgopen(const char *path) {

    // First check to see if we have been initialized
    if (!sgDriverInitialized) {

        // Call the endpoint initialization 
        if (sgInitEndpoint()) {
            logMessage( LOG_ERROR_LEVEL, "sgopen: Scatter/Gather endpoint initialization failed." );
            return( -1 );
        }

        // Set to initialized
        sgDriverInitialized = 1;
    }
    SgFHandle fHandle = -1;
    // check if file exist
    int i = 0;
    for (i = 0; i < nextFHandle; i++) {
        if (strcmp(sgFileMap.fPaths[i], path) == 0) {
            fHandle = i;
            break;
        }
    }
    if (fHandle == -1) {
        if (nextFHandle >= fileSize) {
            fileSize = fileSize * 2;
            // malloc total files 
            sgFileMap.files = (SG_File **) realloc(sgFileMap.files, fileSize * sizeof(SG_File *));
            sgFileMap.fPaths = (char **) realloc(sgFileMap.fPaths, fileSize * sizeof(char *));
            logMessage(LOG_INFO_LEVEL, "resize file map: %d to %d", fileSize / 2, fileSize);
        }
        // create a new file
        fHandle = nextFHandle;
        sgFileMap.fPaths[fHandle] = (char *) malloc(strlen(path) + 1);
        strcpy(sgFileMap.fPaths[fHandle], path);
        sgFileMap.files[fHandle] = (SG_File *) malloc(sizeof(SG_File));
        sgFileMap.files[fHandle]->fHandle = fHandle;
        sgFileMap.files[fHandle]->fSize = 0;
        sgFileMap.files[fHandle]->numBlocks = 0;
        nextFHandle++;
    }

    sgFileMap.files[fHandle]->fPointer = 0;
    sgFileMap.files[fHandle]->open = 1;
 
    // Return the file handle 
    return fHandle;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgread
// Description  : Read data from the file
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure

int sgread(SgFHandle fh, char *buf, size_t len) {
    if (fh < 0 || fh >= nextFHandle) {
        return -1;
    } else if (sgFileMap.files[fh]->open == 0) {
        return -1;
    }
    int position = sgFileMap.files[fh]->fPointer;
    if (position >= sgFileMap.files[fh]->fSize) {
        logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: pointer is set to the end of the file.");
        return -1;
    }
    // Local variables
    SGDataBlock block = {[0 ... 1023] = 0};
    char * indicator;
    SG_Block_ID blockID = sgFileMap.files[fh]->blockID[position / SG_BLOCK_SIZE];
    SG_Node_ID sgRemoteNodeId = sgFileMap.files[fh]->remNodeID[position / SG_BLOCK_SIZE];
    SG_SeqNum sgRemoteSeqNum = find(sgRemoteNodeId);    

    // check if block is in cache
    if ((indicator = getSGDataBlock(sgRemoteNodeId, blockID)) != NULL) {
        memcpy(block, indicator, SG_BLOCK_SIZE);
    } else {
        // block is not in cache
        char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
        size_t pktlen, rpktlen;
        SG_Node_ID loc, rem;
        SG_Block_ID blkid;
        SG_SeqNum sloc, srem;
        SG_System_OP op;
        SG_Packet_Status status;

        pktlen = SG_BASE_PACKET_SIZE;
        // Setup the packet
        if ((status = serialize_sg_packet(sgLocalNodeId,  // Local ID
                                        sgRemoteNodeId,   // Remote ID
                                        blockID,          // Block ID
                                        SG_OBTAIN_BLOCK,  // Operation
                                        sgLocalSeqno,     // Sender sequence number
                                        ++sgRemoteSeqNum, // Receiver sequence number
                                        NULL, initPacket, &pktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: failed serialization of packet [%d].", status);
            return( -1 );
        }
        sgLocalSeqno++;
        // Send the packet
        rpktlen = SG_DATA_PACKET_SIZE;
        if (sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen)) {
            logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: failed packet post" );
            return( -1 );
        }
        // Unpack the recieived data
        if ((status = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, &srem, block, recvPacket, rpktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: failed deserialization of packet [%d]", status);
            return( -1 );
        }
        update(sgRemoteNodeId, srem);

        // put block data into cache
        putSGDataBlock(sgRemoteNodeId, blockID, block);
    }
    // copy len size data into buffer based on file pointer
    if (len == SG_BLOCK_SIZE) {
        memcpy(buf, block, len);
    } else {
        switch (position % SG_BLOCK_SIZE) {
            case (0):
                memcpy(buf, block, len);
                break;
            case (256):
                memcpy(buf, block + 256, len);
                break;
            case (512):
                memcpy(buf, block + 512, len);
                break;
            case (768):
                memcpy(buf, block + 768, len);
                break;
            default:
                logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: pointer is not set correctly");
                break;
        }
    }
    // update file position
    sgFileMap.files[fh]->fPointer += len;
    // Return the bytes processed
    return len;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int sgwrite(SgFHandle fh, char *buf, size_t len) {
    if (fh < 0 || fh >= nextFHandle) {
        return -1;
    } else if (sgFileMap.files[fh]->open == 0) {
        return -1;
    }
    // local variables
    int position = sgFileMap.files[fh]->fPointer;
    char initPacket[SG_DATA_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    SGDataBlock block = {[0 ... 1023] = 0};
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status status;
    if (position > sgFileMap.files[fh]->fSize) {
        logMessage( LOG_ERROR_LEVEL, "sgCreateBlock: pointer is not at the end of the file.");
        return -1;
    }
    // check if file pointer is at the end
    if (position == sgFileMap.files[fh]->fSize) {
        // write a new block
        pktlen = SG_DATA_PACKET_SIZE;       
        memcpy(block, buf, len);
        // set up the packet
        if((status = serialize_sg_packet(sgLocalNodeId, 
                                        SG_NODE_UNKNOWN, 
                                        SG_BLOCK_UNKNOWN,
                                        SG_CREATE_BLOCK,
                                        sgLocalSeqno,
                                        SG_SEQNO_UNKNOWN,
                                        block, initPacket, &pktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgCreateBlock: failed serialization of packet [%d].", status);
            return -1;
        }
        sgLocalSeqno++;
        // send the packet
        rpktlen = SG_BASE_PACKET_SIZE;
        if (sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen)) {
            logMessage(LOG_ERROR_LEVEL, "sgCreateBlock: failed packet post" );
            return -1;
        }
        // unpack the received data
        if ((status = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgCreateBlock: failed deserialization of packet [%d]", status);
            return -1;
        }
        // malloc total blocks per file
        if (position / SG_BLOCK_SIZE >= sgFileMap.files[fh]->numBlocks) {
            mallocBlockPerFile(fh);
        }
        // save node/block IDs as the current block in the file
        sgFileMap.files[fh]->blockID[position / SG_BLOCK_SIZE] = blkid;
        sgFileMap.files[fh]->remNodeID[position / SG_BLOCK_SIZE] = rem;
        SG_SeqNum sgRemoteSeqNum = find(rem);
        if (sgRemoteSeqNum == 0) {
            put(rem, srem);
        } else {
            update(rem, srem);
        }
        sgFileMap.files[fh]->fSize += SG_BLOCK_SIZE;
        putSGDataBlock(rem, blkid, block);
    } else {
        // update file block
        SG_Block_ID blockID = sgFileMap.files[fh]->blockID[position / SG_BLOCK_SIZE];
        SG_Node_ID sgRemoteNodeId = sgFileMap.files[fh]->remNodeID[position / SG_BLOCK_SIZE];

        pktlen = SG_DATA_PACKET_SIZE;
        // obtain old block data
        switch (position % SG_BLOCK_SIZE) {
            case (0):
                sgread(fh, block, SG_BLOCK_SIZE);
                memcpy(block, buf, len);
                sgFileMap.files[fh]->fPointer -= SG_BLOCK_SIZE;
                break;
            case (256):
                sgFileMap.files[fh]->fPointer -= 256;
                sgread(fh, block, SG_BLOCK_SIZE);
                memcpy(block + 256, buf, len);
                sgFileMap.files[fh]->fPointer -= 768;
                break;
            case (512):
                sgFileMap.files[fh]->fPointer -= 512;
                sgread(fh, block, SG_BLOCK_SIZE);
                memcpy(block + 512, buf, len);
                sgFileMap.files[fh]->fPointer -= 512;
                break;
            case (768):
                sgFileMap.files[fh]->fPointer -= 768;
                sgread(fh, block, SG_BLOCK_SIZE);
                memcpy(block + 768, buf, len);
                sgFileMap.files[fh]->fPointer -= 256;
                break;
            default:
                logMessage(LOG_ERROR_LEVEL, "sgCreateBlock: pointer is not set correctly");
                break;
        }
        SG_SeqNum sgRemoteSeqNum = find(sgRemoteNodeId);
        // Setup the packet
        if ((status = serialize_sg_packet(sgLocalNodeId,    // Local ID
                                          sgRemoteNodeId,   // Remote ID
                                          blockID,          // Block ID
                                          SG_UPDATE_BLOCK,  // Operation
                                          sgLocalSeqno,     // Sender sequence number
                                          ++sgRemoteSeqNum, // Receiver sequence number
                                          block, initPacket, &pktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgUpdateBlock: failed serialization of packet [%d].", status);
            return( -1 );
        }
        sgLocalSeqno++;
        // Send the packet
        rpktlen = SG_BASE_PACKET_SIZE;
        if (sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen)) {
            logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: failed packet post" );
            return( -1 );
        }
        // Unpack the recieived data
        if ((status = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK) {
            logMessage(LOG_ERROR_LEVEL, "sgObtainBlock: failed deserialization of packet [%d]", status);
            return( -1 );
        }
        update(sgRemoteNodeId, srem);

        // put block data into cache
        putSGDataBlock(rem, blkid, block);
    }
    // increase the file pointer
    sgFileMap.files[fh]->fPointer += len;

    // Log the write, return bytes written
    return len;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : new position if successful, -1 if failure

int sgseek(SgFHandle fh, size_t off) {
    // error checking
    if (fh < 0 || fh >= nextFHandle) {
        return -1;
    } else if (sgFileMap.files[fh]->open == 0) {
        return -1;
    } else if (off >= sgFileMap.files[fh]->fSize) {
        logMessage(LOG_ERROR_LEVEL, "sgseek: offset exceed the file size");      
        return -1;
    }
    sgFileMap.files[fh]->fPointer = off;

    // Return new position
    return off;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int sgclose(SgFHandle fh) {
    if (fh < 0 || fh >= nextFHandle) {
        return -1;
    } else if (sgFileMap.files[fh]->open == 0) {
        return -1;
    } 
    sgFileMap.files[fh]->open = 0;

    // Return successfully
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int sgshutdown(void) {
    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status status;

    // free file data and data paths
    for (int fh = 0; fh < nextFHandle; fh++) {
        if (sgFileMap.files[fh]) {
            free(sgFileMap.files[fh]->blockID);
            free(sgFileMap.files[fh]->remNodeID);
            // logMessage(LOG_INFO_LEVEL, "Closed file %d, deleting %d items out of %d items", fh, sgFileMap.files[fh]->fSize / SG_BLOCK_SIZE, sgFileMap.files[fh]->numBlocks);
            free(sgFileMap.files[fh]);
            free(sgFileMap.fPaths[fh]);
            sgFileMap.files[fh] = NULL;
            sgFileMap.fPaths[fh] = NULL;
        }
    }

    // free node-sequence map
    free(remNodeIDs);
    free(remSeqNums); 
    logMessage(LOG_INFO_LEVEL, "Closed Node-Sequence Map, deleting %d items out of %d items", next - 1, length);

    // free file map
    free(sgFileMap.files);
    free(sgFileMap.fPaths);
    logMessage(LOG_INFO_LEVEL, "Closed File Map, deleting %d items out of %d items", nextFHandle, fileSize);

    pktlen = SG_BASE_PACKET_SIZE;
    // Setup the packet
    if ((status = serialize_sg_packet(sgLocalNodeId,     // Local ID
                                      SG_NODE_UNKNOWN,   // Remote ID
                                      SG_BLOCK_UNKNOWN,  // Block ID
                                      SG_STOP_ENDPOINT,  // Operation
                                      sgLocalSeqno,      // Sender sequence number
                                      SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                      NULL, initPacket, &pktlen)) != SG_PACKT_OK) {
        logMessage(LOG_ERROR_LEVEL, "sgStopEndPoint: failed serialization of packet [%d].", status);
        return( -1 );
    }
    sgLocalSeqno++;
    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if (sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen)) {
        logMessage(LOG_ERROR_LEVEL, "sgStopEndPoint: failed packet post");
        return( -1 );
    }
    // Unpack the recieived data
    if ((status = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK) {
        logMessage(LOG_ERROR_LEVEL, "sgStopEndPoint: failed deserialization of packet [%d]", status);
        return( -1 );
    }
    // close cache
    closeSGCache();

    // Log, return successfully
    logMessage(LOG_INFO_LEVEL, "Shut down Scatter/Gather driver.");
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : serialize_sg_packet
// Description  : Serialize a ScatterGather packet (create packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status serialize_sg_packet(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk, 
        SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data, 
        char *packet, size_t *plen) {
    // define local variable
    SG_Magic magic_num;
    SG_Packet_Status status;
    SG_DataIndicator indicator;
    magic_num = SG_MAGIC_VALUE; 
    // initial data validity checking and error returning
    if (packet) {
        status = check_serialize_sg_Data(loc, rem, blk, op, sseq, rseq);
    } else {
        status = SG_PACKT_PDATA_BAD;
    }
    if (status != SG_PACKT_OK) {
        print_sg_packet_log_message(SG_SERIALIZE, status);
        return status;
    }
    // check if data is Null and assign data indicator
    if (data) {
        indicator = 1;
        *plen = SG_DATA_PACKET_SIZE;
    } else {
        indicator = 0;
        *plen = SG_BASE_PACKET_SIZE;
    }
    SG_Packet_Buffer buffer;
    // pass value into packet buffer, and serialize buffer with data block (if any) into packet
    construct_sg_packet_buffer(&buffer, magic_num, loc, rem, blk, op, sseq, rseq, indicator);
    memcpy(packet, &buffer, sizeof(SG_Packet_Buffer));
    if (indicator == 1) {
        memcpy(packet + sizeof(SG_Packet_Buffer), data, SG_BLOCK_SIZE);
        memcpy(packet + sizeof(SG_Packet_Buffer) + SG_BLOCK_SIZE, &magic_num, sizeof(SG_Magic));
    } else {
        memcpy(packet + sizeof(SG_Packet_Buffer), &magic_num, sizeof(SG_Magic));
    }
    // print log message indicating a success processing
    // logMessage( LOG_INFO_LEVEL, "sg_packet proccessing worked correctly (single packet)." );
    
    return status; 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : deserialize_sg_packet
// Description  : De-serialize a ScatterGather packet (unpack packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status deserialize_sg_packet( SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk, 
        SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data, 
        char *packet, size_t plen ) {
    // define local variables
    SG_Packet_Buffer buffer;
    SG_Packet_Status status;
    SGDataBlock block;
    // copy packet information to packet buffer (except *data and the last magic number)
    if (packet) {
        memcpy(&buffer, packet, sizeof(SG_Packet_Buffer));
        // check data validity in packet and check if input variable pointer is Null
        status = check_deserialize_sg_Data(buffer, loc, rem, blk, op, sseq, rseq);
    } else {
        status = SG_PACKT_PDATA_BAD;
    }
    if (status == SG_PACKT_OK) {
        SG_Magic magic;
        // checking plen, data block, and the last magic value (the last part of validity checking)
        if (buffer.indicator == 1) {
            memcpy(block, packet + sizeof(buffer), SG_BLOCK_SIZE);
            if (plen != SG_DATA_PACKET_SIZE) {
                status = SG_PACKT_BLKLN_BAD;
            } else if (data) {
                memcpy(&magic, packet + sizeof(buffer) + SG_BLOCK_SIZE, sizeof(SG_Magic));
                if (magic != SG_MAGIC_VALUE) {
                    status = SG_PACKT_PDATA_BAD;
                }
            } else {
                status = SG_PACKT_BLKDT_BAD;
            }
        } else if (plen != SG_BASE_PACKET_SIZE) {
            status = SG_PACKT_BLKLN_BAD;
        } else {
            memcpy(&magic, packet + sizeof(buffer), sizeof(SG_Magic));
            if (magic != SG_MAGIC_VALUE) {
                status = SG_PACKT_PDATA_BAD;
            }
        }
    }
    // reporting error message if status indicates bad data
    if (status != SG_PACKT_OK) {
        print_sg_packet_log_message(SG_DESERIALIZE, status);
        return status;
    }
    // deserialize packet into input pointers
    memcpy(loc, &(buffer.sendNodeId), sizeof(SG_Node_ID));
    memcpy(rem, &(buffer.recvNodeId), sizeof(SG_Node_ID));
    memcpy(blk, &(buffer.blockID), sizeof(SG_Block_ID));
    memcpy(op, &(buffer.operation), sizeof(SG_System_OP));
    memcpy(sseq, &(buffer.sendSeqNo), sizeof(SG_SeqNum));
    memcpy(rseq, &(buffer.recvSeqNo), sizeof(SG_SeqNum));
    if (buffer.indicator == 1) {
        memcpy(data, block, SG_BLOCK_SIZE);  
    }
  
    return status;
}
           
////////////////////////////////////////////////////////////////////////////////
//
// Function     : check_serialize_sg_Data
// Description  : check if input data from serialized function is valid
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
// Outputs      : status - packet status that identify bad data or OK

SG_Packet_Status check_serialize_sg_Data(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk, 
        SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq) {
    SG_Packet_Status status;
    // identify, if any, incorrectly formulated input data, and set corresponding status 
    if (loc <= 0) {
        status = SG_PACKT_LOCID_BAD;
    } else if (rem <= 0) {
        status = SG_PACKT_REMID_BAD;
    } else if (blk <= 0) {
        status = SG_PACKT_BLKID_BAD;
    } else if (op >= SG_MAXVAL_OP || op < SG_INIT_ENDPOINT) {
        status = SG_PACKT_OPERN_BAD;
    } else if (sseq <= 0) {
        status = SG_PACKT_SNDSQ_BAD;
    } else if (rseq <= 0) {
        status = SG_PACKT_RCVSQ_BAD;
    } else {
        status = SG_PACKT_OK;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : check_deserialize_sg_Data
// Description  : check if input data from deserialized function and packet is valid
//
// Inputs       : buffer - data extracted from serialized packet
//                loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
// Outputs      : status - packet status that identify bad data or OK

SG_Packet_Status check_deserialize_sg_Data(SG_Packet_Buffer buffer, SG_Node_ID *loc, SG_Node_ID *rem, 
        SG_Block_ID *blk, SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq) {
    SG_Packet_Status status;
    // check packet data and if input parameters is Null
    if (buffer.magic != SG_MAGIC_VALUE) {
        status = SG_PACKT_PDATA_BAD;
    } else {
        status = check_serialize_sg_Data(buffer.sendNodeId, buffer.recvNodeId, buffer.blockID, 
        buffer.operation, buffer.sendSeqNo, buffer.recvSeqNo);
    }
    // if ((status = check_input_sg_Data(loc, rem, blk, op, sseq, rseq)) == SG_PACKT_OK) 
    return status;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : construct_sg_packet_buffer
// Description  : pass serialize input data to packet buffer, which is partailly filled with data to be serialized
//
// Inputs       : buffer - data extracted from serialized packet
//                magic_num - magic value
//                loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                indicator - the data indicator
// Outputs      : return 0 always

int construct_sg_packet_buffer(SG_Packet_Buffer *buffer, SG_Magic magic_num, SG_Node_ID loc, SG_Node_ID rem, 
    SG_Block_ID blk, SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, SG_DataIndicator indicator) {
    // assigning input data into packet buffer used for serialization process
    buffer->magic = magic_num;
    buffer->sendNodeId = loc;
    buffer->recvNodeId = rem;
    buffer->blockID = blk;
    buffer->operation = op;
    buffer->sendSeqNo = sseq;
    buffer->recvSeqNo = rseq;
    buffer->indicator = indicator;

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : print_sg_packet_log_message
// Description  : The function used to print error message
//
// Inputs       : process - either serialize or deserialize
//                status - packet status indicating type of bad data
// Outputs      : return 0 always

int print_sg_packet_log_message(SG_Process process, SG_Packet_Status status) {
    // define local variable
    const char * message;
    const char * error;
    // synthesize error message based on type of process and type of error
    if (process == SG_SERIALIZE) {
        message = "serialize_sg_packet";
    } else {
        message = "deserialize_sg_packet";
    }
    switch(status) {
        case (SG_PACKT_LOCID_BAD):
            error = "bad local ID [0]";
            break;
        case (SG_PACKT_REMID_BAD):
            error = "bad remote ID [0]";
            break;
        case (SG_PACKT_BLKID_BAD):
            error = "bad block ID [0]";
            break;
        case (SG_PACKT_OPERN_BAD):
            error = "bad op code (212)";
            break;
        case (SG_PACKT_SNDSQ_BAD):
            error = "bad sender sequence number [0]";
            break;
        case (SG_PACKT_RCVSQ_BAD):
            error = "bad receiver sequence num [0]";
            break;
        case (SG_PACKT_BLKDT_BAD):
            error = "bad block data [0]";
            break;
        case (SG_PACKT_BLKLN_BAD):
            error = "bad block length [0]";
            break;
        case (SG_PACKT_PDATA_BAD):
            error = "bad packet data [0]";
            break;
        default:
            error = "bad local ID [0]";
    }
    // report type of bad data
    logMessage( LOG_ERROR_LEVEL, "%s: %s", message, error);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : find
// Description  : The function used to find remote sequence number corresponding to a given remote node ID 
//
// Inputs       : remNodeID - remote node ID
// Outputs      : return remSeqNum if found, otherwise return 0

SG_SeqNum find(SG_Node_ID remNodeID) {
    for (int i = 0; i < next; i++) {
        if (remNodeIDs[i] == remNodeID) {
            return remSeqNums[i];
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put
// Description  : The function used to insert a new remote node together with its remote sequence number. 
//                Re-allocate node-sequence map if needed.
//
// Inputs       : remNodeID - remote node ID
//                remSeqNum - remote sequence number
// Outputs      : return 0 always

int put(SG_Node_ID remNodeID, SG_SeqNum remSeqNum) {
    remNodeIDs[next] = remNodeID;
    remSeqNums[next] = remSeqNum;
    next++;
    if (next >= length) {
        length = length * 2;
        remNodeIDs = (SG_Node_ID *) realloc(remNodeIDs, length * sizeof(SG_Node_ID));
        remSeqNums = (SG_SeqNum *) realloc(remSeqNums, length * sizeof(SG_SeqNum));
    }
    logMessage(LOG_INFO_LEVEL, "resize node-sequence map: %d to %d", length / 2, length);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : update
// Description  : The function used to update remote sequence number. 
//
// Inputs       : remNodeID - remote node ID
//                remSeqNum - remote sequence number
// Outputs      : return 0 always

int update(SG_Node_ID remNodeID, SG_SeqNum remSeqNum) {
    for (int i = 0; i < next; i++) {
        if (remNodeIDs[i] == remNodeID) {
            remSeqNums[i] = remSeqNum;
            break;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : mallocBlockPerFile
// Description  : The function used to dynamically allocate blocks per file. 
//
// Inputs       : fh - file handle
// Outputs      : return 0 always

int mallocBlockPerFile(SgFHandle fh) {
    if (sgFileMap.files[fh]->numBlocks == 0) {
        sgFileMap.files[fh]->blockID = (SG_Block_ID *) malloc(sizeof(SG_Block_ID));
        sgFileMap.files[fh]->remNodeID = (SG_Node_ID *) malloc(sizeof(SG_Node_ID));
        sgFileMap.files[fh]->numBlocks = 1;
        logMessage(LOG_INFO_LEVEL, "resize number of blocks per file on file handle %d: %d to %d", fh, 0, 1);
    } else {
        sgFileMap.files[fh]->numBlocks *= 2;
        sgFileMap.files[fh]->blockID = (SG_Block_ID *) realloc(sgFileMap.files[fh]->blockID, sgFileMap.files[fh]->numBlocks * sizeof(SG_Block_ID));
        sgFileMap.files[fh]->remNodeID = (SG_Node_ID *) realloc(sgFileMap.files[fh]->remNodeID, sgFileMap.files[fh]->numBlocks * sizeof(SG_Node_ID));
        logMessage(LOG_INFO_LEVEL, "resize number of blocks per file on file handle %d: %d to %d", fh, sgFileMap.files[fh]->numBlocks / 2, sgFileMap.files[fh]->numBlocks);
    }

    return 0;
}


//
// Driver support functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgInitEndpoint
// Description  : Initialize the endpoint
//
// Inputs       : none
// Outputs      : 0 if successfull, -1 if failure

int sgInitEndpoint( void ) {

    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // initialize cache
    initSGCache(SG_MAX_CACHE_ELEMENTS);

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet(SG_NODE_UNKNOWN,   // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_INIT_ENDPOINT,  // Operation
                                    sgLocalSeqno,      // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
        return( -1 );
    }
    sgLocalSeqno++;
    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // intialize node-sequence map
    remNodeIDs = (SG_Node_ID *) malloc(sizeof(SG_Node_ID));
    remSeqNums = (SG_SeqNum *) malloc(sizeof(SG_SeqNum));
    length = 1;

    // initialize file map
    sgFileMap.files = (SG_File **) malloc(sizeof(SG_File *));
    sgFileMap.fPaths = (char **) malloc(sizeof(char *));
    fileSize = 1;

    // Set the local node ID, log and return successfully
    sgLocalNodeId = loc;
    logMessage( LOG_INFO_LEVEL, "Completed initialization of node (local node ID %lu", sgLocalNodeId );
    return( 0 );
}
