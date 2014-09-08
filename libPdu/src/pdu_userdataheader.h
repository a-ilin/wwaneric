/*
 *  pdu_userdataheader.h
 *  libPDU
 *
 *  Created by James Pitts on 13/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef _PDU_USER_DATA_HEADER
#define _PDU_USER_DATA_HEADER

#include <sys/types.h>

#include "caststring.h"

#include "common.h"

class Pdu_Packed;

/**
*      Abstraction of a  user data header
 */
class PDU_DECODE_API Pdu_User_Data_Header
{
    unsigned int identifier_;
    unsigned int length_;

protected:
    Pdu_User_Data_Header ( unsigned int identifier, unsigned int length  ) : identifier_ ( identifier ), length_ ( length )  {}
    Pdu_User_Data_Header ( ) : identifier_(0), length_(0)  {}

public:

    enum
    {
      IEI_CONCATENATED = 0,
      IEI_SPECIAL      = 1,
      IEI_RESERVED     = 2,
      IEI_LF           = 3,
      IEI_PORT8BIT     = 4,
      IEI_PORT16BIT    = 5,
      IEI_SMSC         = 6,
      IEI_UDH_SOURCE   = 7,
      IEI_CONCATENATED_16BIT = 8,
      IEI_CONTROL      = 9
    };

    virtual ~Pdu_User_Data_Header ( );

    unsigned int getLength ( void ) const { return length_; }
    unsigned int getIdentifier ( void ) const { return identifier_; }

    static Pdu_User_Data_Header *create ( Pdu_Packed *packed );

    virtual void decode ( Pdu_Packed *packed );
    virtual void dump ( void ) const;
};

/**
 *      Abstraction of a concatenated message user data header
 */
class PDU_DECODE_API Pdu_Concatenated : public Pdu_User_Data_Header
{
    unsigned int ref_;
    unsigned int max_;
    unsigned int seq_;

    bool _16bit_;

    Pdu_Concatenated( ) {}

public:
    Pdu_Concatenated ( unsigned int identifier, unsigned int length, bool _16bit ) :
      Pdu_User_Data_Header  (  identifier, length ),
      _16bit_(_16bit)
    {}

    void decode ( Pdu_Packed *packed );
    void dump ( void ) const;

    unsigned int getReference ( void ) const { return ref_; }
    unsigned int getMax ( void ) const { return max_; }
    unsigned int getSequence ( void ) const { return seq_; }

    const char *getReferenceAsCStr ( void ) const { return cast_string ( ref_ ).c_str(); }
    const char *getMaxAsCStr ( void ) const { return cast_string ( max_ ).c_str(); }
    const char *getSequenceAsCStr ( void ) const { return cast_string ( seq_ ).c_str(); }
};

#endif

