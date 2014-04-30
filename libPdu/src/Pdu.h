/*
 *  Pdu.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef PDU__H_
#define PDU__H_

#include "pdu_base.h"
#include "pdu_userdata.h"
#include "pdu_7bit_packing.h"
#include "pdu_8bit_packing.h"
#include "pdu_address.h"
#include "pdu_datacoding.h"
#include "pdu_log.h"
#include "pdu_packed.h"
#include "pdu_userdataheader.h"


#include <string>


class Pdu  : public Pdu_Base
{
    
    Pdu ( const std::string &pdu ) : Pdu_Base ( pdu ) {}
    Pdu ();

public:
    Pdu ( const Pdu_Base &pdu ) : Pdu_Base ( pdu ) {} // friend
    ~Pdu ();    
    

    static Pdu *create ( const std::string &pdu );

    virtual void dump ( void ) const { PDU_LOG->log (("Pdu::dump\n")); }

    static const std::string keyword ( const std::string &str );
    static const std::string text ( const std::string &str );
    
};



#endif


