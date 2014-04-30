/*
 *  pdu_base.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_base.h"
#include "pdu_packed.h"
#include "pdu_address.h"
#include "pdu_log.h"

#include <cstdlib>
#include <string>
#include <stdio.h>

/*
 *  Pdu_Base
 */
Pdu_Base::Pdu_Base () : Pdu_Packed (""), header_ ( 0 )
{
}

Pdu_Base::Pdu_Base ( std::string pdu ) : Pdu_Packed ( pdu ), header_ ( 0 )
{
}

Pdu_Base::Pdu_Base ( const Pdu_Base &that ) : Pdu_Packed ( that ), 
    smsc_ ( that.smsc_ ), 
    header_ ( that.header_ )
{
}

/*
 *  ~Pdu_Base
 */
Pdu_Base::~Pdu_Base ()
{
}

/**
 *      Formats a PDU timestamp into something more human readable
 *  @param time A Pdu 14 Semi Octet timestamp
 *  @return A decent timestamp
 */
const std::string Pdu_Base::formatTimeStamp ( const std::string &time ) 
{
    std::string formatted ("") ;

    if (time.length() < 14) 
      return formatted;
    
    formatted  = time.substr (0, 2);
    formatted += '-';
    formatted += time.substr (2, 2);
    formatted += "-";
    formatted += time.substr (4, 2);
    formatted += " ";
    formatted += time.substr (6, 2); // hour
    formatted += ":";
    formatted += time.substr (8, 2); // minute
    formatted += ":";
    formatted += time.substr (10, 2); // seconds
    formatted += "+";
    
    
    std::string quarter =  time.substr ( 13,1 );
    quarter += time.substr ( 12, 1) ;

    int hours = ::atoi (quarter.c_str());
    hours /= 4;
    formatted += hours + '0';  // FIXME:: wont work for anything > + 9
    
    return formatted;
    
}
 
/**
 *  decode
 */
void Pdu_Base::decode(void)
{
    
    getAddressField ( smsc_, true );
    
    header_ = getOctetAsInt ();
    
}

/**
 *      dump
 */
void Pdu_Base::dump ( void  ) const
{
    PDU_LOG->log ("Pdu_Base Dump\n");
    PDU_LOG->log ("  PDU : %s\n", getPdu()->c_str() );
    switch ( getMTI() )
    {
        case SMS_DELIVER:
            PDU_LOG->log ( "MTI : SMS_DELIVER\n" );
            break;
        case SMS_STATUS_REPORT:
            PDU_LOG->log ( "MTI : SMS_STATUS_REPORT\n" );
            break;
        case SMS_SUBMIT_REPORT:
            PDU_LOG->log ( "MTI : SMS_SUBMIT_REPORT\n" );
            break;
        case SMS_RESERVER:
            PDU_LOG->log ( "MTI : SMS_RESERVER\n" );
            break;
            
    }
    smsc_.dump();
 
}


