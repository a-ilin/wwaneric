/*
 *  pdu_userdata.cpp
 *  libPDU
 *
 *  Created by James Pitts on 09/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_userdata.h"
#include "pdu_datacoding.h"
#include "pdu_log.h"

/**
 *      Constructor.
 */
Pdu_User_Data::Pdu_User_Data ( Pdu_Packed *packed, int length ) :
    packed_ ( packed ),
    userDataLength_ ( length ),
    userData8Bit_ ( 0 ),
    userHeaderLength_ ( 0 )
  //, userDataHeader_ ( 0 )
{
}


/**
 *  The ~Pdu_User_Data destructor frees the 8bit user data header
 *  or an unpacked data area if present.
 */
Pdu_User_Data::~Pdu_User_Data ()
{

    if (userData8Bit_)
        delete [] userData8Bit_;

    unsigned int indx;
    for (indx = 0; indx < userDataHeader_.size(); indx++)
    {
        delete userDataHeader_[indx];
    }

    userDataHeader_.clear();

}

/**
 *      Decodes the 8bit users data header.
 *
 */
void Pdu_User_Data::decodeUserDataHeader ( void )
{
    userHeaderLength_ = packed_->getOctetAsInt();

    unsigned int read = 0;
    while ( read < userHeaderLength_ )
    {
        Pdu_User_Data_Header *header = Pdu_User_Data_Header::create  ( packed_ );
        userDataHeader_.push_back ( header);
        read += header->getLength() + 2; // + 2 octets for id and length fields
    }
}


/**
 *  Decodes the user data.
 *
 *  @param dataCoding   Format of the data user area
 */
void Pdu_User_Data::decodeUserData ( const Pdu_Data_Coding &dataCoding )
{
    if ( dataCoding.isDefaultAlphabet() )
    {
        unsigned int startSeptet = 0;
        if ( userHeaderLength_ )
        {
          startSeptet = packed_->octetsToSeptets ( userHeaderLength_  );
          userDataLength_ = userDataLength_ - packed_->octetsToSeptets ( userHeaderLength_ + 1);   // + 1 for UDHL
        }
        packed_->unpackSeptetLength( userdata_, userDataLength_, startSeptet );
    }
    else if ( dataCoding.getAlphabet () == Pdu_Data_Coding::ALPHABET_8BIT )
    {
        userData8Bit_ = new unsigned char [  userDataLength_ - userHeaderLength_ ];
        packed_->getBytes ( userData8Bit_,  userDataLength_ - userHeaderLength_);
    }
}

/**
 *      dump displays all the useful information it can about this object.
 */
void Pdu_User_Data::dump ( void ) const
{
    PDU_LOG->log ( "Pdu_User_Data Dump\n" );
    PDU_LOG->log ( " User Data Length : %d\n", userDataLength_ );
    PDU_LOG->log ( " User Data Header Length : %d\n", userHeaderLength_ );

    unsigned int indx;
    for (indx = 0; indx < userDataHeader_.size(); indx++)
    {
        PDU_LOG->log ("User Data Header %d\n", indx + 1);
        userDataHeader_[indx]->dump();
    }


    if ( userdata_.length() )
    {
        PDU_LOG->log ("User Data : %s\n", userdata_.c_str() );
        PDU_LOG->hexDump ( userdata_.c_str(), userDataLength_ );
    }
    else if  ( userData8Bit_ )
    {
        PDU_LOG->log ( "8 Bit data dump\n" );
        PDU_LOG->hexDump ( userData8Bit_, userDataLength_ - userHeaderLength_ );
    }

}

