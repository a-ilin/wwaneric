/*
 *  pdu_address.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_address.h"
#include "pdu_log.h"

Pdu_Address::Pdu_Address () : length_ ( 0 ), type_ ( 0 ), value_ ( "" )
{
}


void Pdu_Address::dump ( void ) const
{
    PDU_LOG->log ( "Pdu_Address Dump\n");
    PDU_LOG->log ( "  length : %d\n", length_ );
    PDU_LOG->log ( "  type : %#x\n", type_ );
    PDU_LOG->log ( "  value : %s\n", value_.toLocal8Bit().constData() );
    
}

/*
 *  setLengthAsOctets
 *
 *  The length of an address is specified in semi-octets
 *    and always even.
 *  We subtract one to account for the type field
 *    and mult by 2 to get semioctets.
 */
void Pdu_Address::setLengthAsOctets ( unsigned int length )
{
    length_ = ( length - 1 ) * 2; 
}

/*
 *  setLengthAsSemiOctets
 *
 *  The length of an address is specified in semi-octets
 *    and always even.
 *  Semi octet counts of addresses are of the usful numbers
 *    not the actual field length.  So if it's odd we add
 *    an extra one to make it end on an octet boundary
 */
void Pdu_Address::setLengthAsSemiOctets ( unsigned int length )
{
    
    // round up to an even semi-octect count if we have to..
    length % 2 == 1 ? length ++ : length ;
    
    length_ = length; 
}

