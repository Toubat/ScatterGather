#ifndef SG_CACHE_INCLUDED
#define SG_CACHE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_cache.h
//  Description    : This is the declaration of the block cache for the scatter
//                   gather system.
//
//   Author        : Patrick McDaniel
//   Last Modified : Sun 18 Oct 2020 03:53:24 PM EDT
//

// Includes
#include <sg_defs.h>

//
// Defines
#define SG_MAX_CACHE_ELEMENTS 128

// 
// Cache functions

int initSGCache( uint16_t maxElements );
    // Initialize the cache of block elements

int closeSGCache( void );
    // Close the cache of block elements, clean up remaining data

char *getSGDataBlock( SG_Node_ID nde, SG_Block_ID blk );
    // Get the data block from the block cache

int putSGDataBlock( SG_Node_ID nde, SG_Block_ID blk, char *block );
    // Get the data block from the block cache

#endif