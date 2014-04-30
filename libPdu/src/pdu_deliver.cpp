/*
 *  pdu_deliver.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_deliver.h"
#include "pdu_log.h"
#include "pdu_userdata.h"

/**
 *  @param base a reference to the base PDU object
 */
Pdu_Deliver::Pdu_Deliver ( const Pdu_Base & base ) : Pdu ( base ), protocol_( 0 ),
    dataCoding_ ( 0 ), timeStamp_ ( "" ), userDataLength_ ( 0 ), userData_ ( 0 )
{
}

/*
Pdu_Deliver &Pdu_Deliver ( const Pdu_Deliver &that )
{
}
*/

Pdu_Deliver::~Pdu_Deliver ( )
{
    if ( userData_ )
        delete userData_;
}

/**
 *  dump.
 *
 *  Dumps to the logger all information about this SMS_DELIVER PDU
 */
void Pdu_Deliver::dump ( void ) const
{
    Pdu::dump();

    PDU_LOG->log ("Pdu_Deliver\n");

    if ( moreMessagesToSend () == true )
        PDU_LOG->log ( "  MMS : true\n" );
    else
        PDU_LOG->log ( "  MMS : false\n" );

    if ( statusReportIndicator () == true )
        PDU_LOG->log ( "  SRI : true\n" );
    else
        PDU_LOG->log ( "  SRI : false\n" );

    if ( userDataHeaderIndicator () == true )
        PDU_LOG->log ( "  UDHI : true\n" );
    else
        PDU_LOG->log ( "  UDHI : false\n" );

    if ( replyPath () == true )
        PDU_LOG->log ( "  RP : true\n" );
    else
        PDU_LOG->log ( "  RP : false\n" );



    //PDU_LOG->log ( "  Origin: %s\n", senderNumber_.getValue().c_str() );

    senderNumber_.dump();

    PDU_LOG->log ( "  Protocol : %#x\n", protocol_ );
    dataCoding_.dump();

    PDU_LOG->log ( "  Timestamp: %s\n", timeStamp_.c_str() );
    PDU_LOG->log ( "  Timestamp: %s\n", formatTimeStamp (timeStamp_).c_str() );
    PDU_LOG->log ( "  User Data Length: %d\n", userDataLength_ );

    if ( userData_ )
        userData_->dump();
}


/*
 *      decode
 *
 *  Decodes the SMS_DELIVER portion of the pdu
 */
void Pdu_Deliver::decode ( void )
{


    getAddressField ( senderNumber_ );

    protocol_ = getOctetAsInt();
    dataCoding_.setDataCode (  getOctetAsInt() );
    getSemiOctetAsString ( timeStamp_, 14 ); // timestamp is 14 semi octets, TP_SCTS
    userDataLength_ = getOctetAsInt();

    userData_ = new Pdu_User_Data ( this, userDataLength_ );

    if ( userDataHeaderIndicator () == true )
        userData_->decodeUserDataHeader( );

    userData_->decodeUserData( dataCoding_ );

}

/**
 *
 *  moreMessagesToSend
 *
 *  The logic on this appears to be opposite to what you'd expect.
 *
 *  0 in bit 2, more messages to send.
 *  1 in bit 2, no more messages to send.
 *
 *  @author James Pitts
 *  @return true if more messages to send otherwise false.
 */
bool Pdu_Deliver::moreMessagesToSend ( void ) const
{
    if ( (( getHeader() >> 2 ) & 1) == 1)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*
 *  replyPath
 *
 *  TP-RP
 */
bool Pdu_Deliver::replyPath ( void ) const
{
    if ( (( getHeader() >> 7 ) & 1) == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *  userDataHeaderIndicator
 *
 *      TP_UDHI
 *  @return true if a user data head is present.
 *
 */
bool Pdu_Deliver::userDataHeaderIndicator (void) const
{
    if ( (( getHeader() >> 6 ) & 1) == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *
 *  TP-SRI
 *
 * @return true if a status report will be returned to the SME
 */
bool Pdu_Deliver::statusReportIndicator (void) const
{
    if ( (( getHeader() >> 6 ) & 1) == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *
 */
const Pdu_Concatenated *Pdu_Deliver::getConcatenated ( void ) const
{
    Pdu_User_Data::iterator iter = userData_->getIterator ();

    while ( iter != userData_->getEndIterator () )
    {
        if ( (*iter)->getIdentifier () == Pdu_User_Data_Header::IEI_CONCATENATED )
        {
            const Pdu_Concatenated *concat = dynamic_cast <Pdu_Concatenated*>(*iter);

            return concat;
        }
    }

    return NULL;
}


