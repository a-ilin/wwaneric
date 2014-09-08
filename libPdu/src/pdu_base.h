/*
 *  pdu_base.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef __PDU_BASE_H
#define __PDU_BASE_H

#include "pdu_packed.h"
#include "pdu_address.h"

#include "common.h"

class PDU_DECODE_API Pdu_Base : public Pdu_Packed {
    Pdu_Address smsc_;
    unsigned int  header_;
    
public:
    
    enum {  SMS_DELIVER = 0, 
            SMS_DELIVER_REPORT = 0, 
            SMS_STATUS_REPORT = 2, 
            SMS_COMMAND = 2, 
            SMS_SUBMIT = 1, 
            SMS_SUBMIT_REPORT = 1, 
            SMS_RESERVER = 3 };
    
    Pdu_Base();
    Pdu_Base ( std::string pdu );
    Pdu_Base ( const Pdu_Base &that );
   ~Pdu_Base();

    static const std::string formatTimeStamp ( const std::string &time );
    
    int getMTI ( void ) const { return header_ & 0x3; }
    unsigned int getHeader ( void ) const { return header_; }
    virtual void decode ( void );
    virtual void dump ( void ) const;
    const Pdu_Address &getSmsc ( void ) const { return smsc_; }
    const std::string getType ( void ) const { return cast_string ( header_ ); }
    const char *getTypeAsCStr ( void ) const { return getType().c_str(); }
};

#endif

