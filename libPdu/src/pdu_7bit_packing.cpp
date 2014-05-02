/*
 *  pdu_7bit_packing.cpp
 *  libPDU
 *
 *  Created by James Pitts on 12/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_7bit_packing.h"
#include "pdu_packed.h"

#include <string>
#include <assert.h>

/**
 *
 */
Pdu_7Bit_Packing::Pdu_7Bit_Packing ( Pdu_Packed *packed) :
    packed_ ( packed )
{
}

Pdu_7Bit_Packing::Pdu_7Bit_Packing ( ) : packed_ ( 0 )
{
    assert ( 0 );
}



/*
 *  packed7
 *
 *  buffer = output
 *  packed = input
 *  dataLen = output buffer size, set to size written to buffer on return,
 *              includes terminating null
 */

void Pdu_7Bit_Packing::unpackSeptetLength ( QString &output, int septets, int startSeptet)
{
    int mask = 0x7f;
    int shift = 0;
    int packing = 0;
    int byte = 0;
    int wrote = 0;

    output = "";

    if ( startSeptet )
    {
        shift = startSeptet % 7;
        packing =  packed_->getOctetAsInt ( );
        packing >>= (7 - shift);
        shift++;
    }

    while ( wrote < septets )
    {
        int decimal = packed_->getOctetAsInt ( );

        decimal <<= shift;
        decimal |= packing;

        packing = ( decimal & ~mask ) >> 7;

        byte = decimal & 0x7f;

        output += (char)byte;
        wrote ++;

        if (shift == 7)
        {
            // imaginary 0 shift here.
            output += (char)(packing & 0x7f);
            wrote ++;

            shift = 1;
            packing = (packing & ~mask) >> 7;

        }
        else
        {
            shift++;
        }

    }

    return;

}

/**
 *  Unpacks semiOctets into output.
 *
 *
 *  @param output Unpacked destination string .
 *  @param semiOctets Count of semiOctets in source to unpack.
 */
void Pdu_7Bit_Packing::unpack ( std::string &output, unsigned int semiOctets )
{
    int mask = 0x7f;
    int shift = 0;
    int packing = 0;
    int byte = 0;
    int wrote = 0;
    /*int start = 0;*/

    output = "";

    for (unsigned int i = 0; i <  semiOctets; i += 2)
    {

        int decimal = packed_->getOctetAsInt ( );

        decimal <<= shift;
        decimal |= packing;

        packing = ( decimal & ~mask ) >> 7;

        byte = decimal & 0x7f;

        output += (char)byte;
        wrote ++;

        if (shift == 7)
        {
            // imaginary 0 shift here.
            output += (char)(packing & 0x7f);
            wrote ++;

            shift = 1;
            packing = (packing & ~mask) >> 7;

        }
        else
        {
            shift++;
        }

    }

    return;
}


