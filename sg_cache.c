////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : YOUR NAME
//   Last Modified : 
//

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include <sg_cache.h>

// Defines
typedef struct {
    size_t accessTime;
    SG_Block_ID blockID;
    SG_Node_ID nodeID;
    SGDataBlock block;
} SG_Cache_Data;

// LRU Cache defined
SG_Cache_Data * cache[SG_MAX_CACHE_ELEMENTS];
int cache_size;
int next_location = 0;
size_t queries = 0;
size_t hit = 0;
// Functional Prototypes

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : initSGCache
// Description  : Initialize the cache of block elements
//
// Inputs       : maxElements - maximum number of elements allowed
// Outputs      : 0 if successful, -1 if failure

int initSGCache( uint16_t maxElements ) {
    // dynamically allocate memory for cache lines
    for (int i = 0; i < maxElements; i++) {
        cache[i] = (SG_Cache_Data *) malloc(sizeof(SG_Cache_Data));
    }
    cache_size = maxElements;

    // Return successfully
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : closeSGCache
// Description  : Close the cache of block elements, clean up remaining data
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int closeSGCache( void ) {
    // free memory
    for (int i = 0; i < cache_size; i++) {
        free(cache[i]);
    }
    // Return successfully
    logMessage(SGDriverLevel, "Closing cache: %d queries, %d hits (%.2f%% hit rate).", queries, hit, (float) hit * 100 / queries);
    logMessage(LOG_INFO_LEVEL, "Closed cmpsc311 cache, deleting %d items", next_location);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
// Outputs      : pointer to block or NULL if not found

char * getSGDataBlock( SG_Node_ID nde, SG_Block_ID blk ) {
    queries += 1;
    int i = 0;
    for (i = 0; i < next_location; i++) {
        // if match, increase # hits by 1 and assign block
        if (nde == cache[i]->nodeID && blk == cache[i]->blockID) {
            hit += 1;
            cache[i]->accessTime = queries;
            return cache[i]->block;
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : putSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
//                block - block to insert into cache
// Outputs      : 0 if successful, -1 if failure

int putSGDataBlock( SG_Node_ID nde, SG_Block_ID blk, char *block ) {
    for (int i = 0; i < next_location; i++) {
        // update block information
        if (nde == cache[i]->nodeID && blk == cache[i]->blockID) {
            queries += 1;
            memcpy(cache[i]->block, block, SG_BLOCK_SIZE);
            cache[i]->accessTime = queries;
            hit += 1;
            return 0;
        }
    }
    // capacity reached
    if (next_location >= cache_size) {
        size_t timeLRU = queries;
        int idx = 0;
        // evict element which is LRU
        for (int i = 0; i < cache_size; i++) {
            if (cache[i]->accessTime < timeLRU) {
                idx = i;
                timeLRU = cache[i]->accessTime;
            }
        }
        cache[idx]->accessTime = queries;
        cache[idx]->blockID = blk;
        cache[idx]->nodeID = nde;
        memcpy(cache[idx]->block, block, SG_BLOCK_SIZE); 
    } else {
        // map cache element into new location
        cache[next_location]->accessTime = queries;
        cache[next_location]->blockID = blk;
        cache[next_location]->nodeID = nde;
        memcpy(cache[next_location]->block, block, SG_BLOCK_SIZE); 
        next_location += 1;
    }

    // Return successfully
    return 0;
}
