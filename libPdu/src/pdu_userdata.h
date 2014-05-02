/*
 *  pdu_userdata.h
 *  libPDU
 *
 *  Created by James Pitts on 09/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef _PDU_USERDATA_H
#define _PDU_USERDATA_H

#include "pdu_packed.h"
#include "pdu_userdataheader.h"

#include <vector>
#include <iterator>

#include <QString>

class Pdu_Data_Coding;

class Pdu_User_Data
{
    Pdu_Packed *packed_;
    QString userdata_;
    unsigned int userDataLength_;
    unsigned int userHeaderLength_;
    std::vector<Pdu_User_Data_Header *> userDataHeader_;

public:
    typedef std::vector<Pdu_User_Data_Header *>::const_iterator iterator;
    Pdu_User_Data ( Pdu_Packed *packed, int length );

    ~Pdu_User_Data();
    void decodeUserDataHeader ( void );
    void decodeUserData ( const Pdu_Data_Coding &dataCoding );

    void dump ( void ) const;

    iterator getIterator ( void ) const { return userDataHeader_.begin() ; }
    iterator getEndIterator ( void ) const { return userDataHeader_.end() ; }

    const QString &getUserData ( void ) const { return userdata_; }
    unsigned int getUserDataLength ( void ) const { return userDataLength_; }

};

#endif

