/*
 *  pdu_7bit_packing.h
 *  libPDU
 *
 *  Created by James Pitts on 12/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef _PDU_7BIT_PACKING
#define _PDU_7BIT_PACKING

#include <string>

#include <QString>

class Pdu_Packed;

class Pdu_7Bit_Packing
{
    Pdu_Packed *packed_;

    Pdu_7Bit_Packing ( Pdu_7Bit_Packing & /*that*/) {}
    Pdu_7Bit_Packing ( ) ;
public:
  //  Pdu_7Bit_Packing ( ) ;
    Pdu_7Bit_Packing ( Pdu_Packed *packed);
    void unpackSeptetLength ( QString &output, int septets, int startSeptet = 0 );
    void unpack (std::string &output, unsigned int semiOctets );
};


#endif

