#ifndef SG_SERVICE_INCLUDED
#define SG_SERVICE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_service.h
//  Description    : This is the declaration of the interface to the 
//                   base functions for the ScatterGather service.
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 03 Sep 2020 06:23:05 PM EDT
//

// Includes
#include <sg_defs.h>

// Defines 

// Type definitions

// Global interface definitions

int sgServicePost( char *packet, size_t *len, char *rpacket, size_t *rlen );
    // Post a packet to the ScatterGather service

#endif