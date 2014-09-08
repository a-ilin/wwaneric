/*
 *  pdu_packed.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef __PDU_PACKED_H
#define __PDU_PACKED_H

#include <string>
#include "pdu_7bit_packing.h"
#include "pdu_8bit_packing.h"

#include "common.h"

class Pdu_Address;

class PDU_DECODE_API Pdu_Packed : public Pdu_7Bit_Packing, public Pdu_8Bit_Packing {

    std::string packed_;
    unsigned int current_;

public:
    Pdu_Packed ( std::string packed );
    Pdu_Packed ( const Pdu_Packed &that);
    ~Pdu_Packed ();

    const std::string *getPdu ( void ) const { return &packed_; }
    const char *getPduAsCStr ( void ) const { return packed_.c_str(); }
    unsigned int getOctetAsInt( void );
    unsigned int getSemiOctetAsInt ( void );
    void getSemiOctetAsString  (std::string &dest, unsigned int length );
    void getOctetAsString ( std::string &dest, int length );
    void getAddressField ( Pdu_Address &addr, bool lengthAsOctets = false );
    void skipSemiOctets ( unsigned int skip) { current_ += skip; }
    void skipOctets ( int skip) { skipSemiOctets ( skip * 2); }
    unsigned int octetsToSeptets ( unsigned int octets );
};

#endif

