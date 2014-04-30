/*
 *  pdu_deliver.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef PDU_DELIVER_H
#define PDU_DELIVER_H

#include "Pdu.h"
#include "pdu_address.h"
#include "pdu_datacoding.h"
#include "pdu_userdataheader.h"

class Pdu_User_Data;

class Pdu_Deliver : public Pdu
{
    Pdu_Address senderNumber_;
    unsigned int protocol_;
    Pdu_Data_Coding dataCoding_;
    std::string timeStamp_;
    unsigned int userDataLength_;
    Pdu_User_Data *userData_;

    Pdu_Deliver ( const Pdu_Deliver &that );

    
    
public:

    Pdu_Deliver ( const Pdu_Base &base ); // friend

    ~Pdu_Deliver ( );
    virtual void dump( void ) const;
    
    bool moreMessagesToSend ( void ) const;
    bool userDataHeaderIndicator (void) const;
    bool replyPath ( void ) const;
    bool statusReportIndicator (void) const;
    
    const Pdu_User_Data *getUserData ( void ) const { return userData_; }

    const Pdu_Address &getSenderNumber ( void ) const { return senderNumber_; }
    const std::string &getSenderNumberAsString ( void ) const { return senderNumber_.getValue(); }
    const char *getSenderNumberAsCStr ( void ) const { return senderNumber_.getValueAsCStr (); }

    const std::string getProtocolAsString ( void ) const { return cast_string  ( protocol_ ); }
    const char *getProtocolAsCStr( void ) const { return getProtocolAsString().c_str(); }
    const Pdu_Data_Coding &getDataCoding ( void ) const { return dataCoding_; }
    const char *getTimeStamp ( void ) const { return timeStamp_.c_str(); }
    const std::string &getTimeStampAsString ( void ) const { return timeStamp_; }

    const Pdu_Concatenated *getConcatenated ( void ) const;

    virtual void decode ( void );
        
};

#endif


