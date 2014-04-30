/*
 *  pdu_8bit_packing.cpp
 *  libPDU
 *
 *  Created by James Pitts on 12/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_8bit_packing.h"
#include "pdu_packed.h"

Pdu_8Bit_Packing::Pdu_8Bit_Packing ( Pdu_Packed *packed) : packed_ ( packed )
{
}

/**
 *
 */
void Pdu_8Bit_Packing::getBytes ( unsigned char *buf, int length )
{
    for (int i = 0; i < length; ++i)
    {
        buf[i] = packed_->getOctetAsInt ();
    }
}
