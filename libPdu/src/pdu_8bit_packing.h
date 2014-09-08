/*
 *  pdu_8bit_packing.h
 *  libPDU
 *
 *  Created by James Pitts on 12/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */


#ifndef _PDU_8BIT_PACKING
#define _PDU_8BIT_PACKING

#include "common.h"

class Pdu_Packed;

class PDU_DECODE_API Pdu_8Bit_Packing
{
    Pdu_Packed *packed_;
      
public:
    Pdu_8Bit_Packing ( Pdu_Packed *packed);
    
    void getBytes ( unsigned char *buf, int length );
};

#endif

