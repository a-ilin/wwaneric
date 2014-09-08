/*
 *  pdu_address.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef __PDU_ADDRESS_H
#define __PDU_ADDRESS_H

#include <QString>
#include "caststring.h"

#include "common.h"

class PDU_DECODE_API Pdu_Address {
    unsigned int        length_;  // Address-Length
    unsigned int        type_;    // Type-of-Address
    QString  value_;   // Address-Value
    
    
public:
        
    enum {
        // type-of-number
        TN_UNKNOWN        = 0,
        TN_INTERNATIONAL  = 1,
        TN_NATIONAL       = 2,
        TN_NETWORK        = 3,
        TN_SUBSCRIBER     = 4,
        TN_ALPHANUMERIC   = 5,
        TN_ABBREVIATED    = 6,
        TN_RESERVED       = 7,

        // numbering-plan-identification
        NP_UNKNOWN        = 0,
        NP_TELEPHONE      = 1,
        NP_DATA           = 3,
        NP_TELEX          = 4,
        NP_NATIONAL       = 8,
        NP_PRIVATE        = 9,
        NP_ERMES          = 10,
        NP_RESERVED       = 15
    };
    
    Pdu_Address ();
    
    void dump ( void ) const;
    
    void setLengthAsSemiOctets ( unsigned int length );
    void setLengthAsOctets ( unsigned int length );
    void setType ( unsigned int type ) { type_ = type; }
    void setValue ( const QString &value ) { value_ = value; }
    
    QString getValue (void) const { return value_; }
    unsigned int getLength (void) const { return length_; }
    unsigned int getType (void) const { return type_; }
    unsigned int getTypeOfNumber ( void ) const { return ( type_ & 0x70 ) >> 4; }
    const std::string getTypeAsString ( void ) const { return cast_string(getType()); }
    const char *getTypeAsCStr ( void ) const { return getTypeAsString().c_str(); }
};

#endif


