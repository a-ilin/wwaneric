/*
 *  pdu_userdataheader.cpp
 *  libPDU
 *
 *  Created by James Pitts on 13/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */


#include "pdu_userdataheader.h"
#include "pdu_packed.h"
#include "pdu_log.h"

/**     Destructor.
 *
 */
Pdu_User_Data_Header::~Pdu_User_Data_Header ( )
{

}

/**  A default decoder for unknown or unhandled blocks.
 *
 */
void Pdu_User_Data_Header::decode ( Pdu_Packed *packed ) 
{ 
    packed->skipOctets ( length_ ); 
}

void Pdu_User_Data_Header::dump ( void ) const
{
    PDU_LOG->log ( "  Length : %d\n", length_);
    PDU_LOG->log ( "  Identifier : %d\n", identifier_);
}

/**  Create a user data header object
 *
 */
Pdu_User_Data_Header *Pdu_User_Data_Header::create ( Pdu_Packed *packed )
{
    
    unsigned int identifier = packed->getOctetAsInt ();
    unsigned int length = packed->getOctetAsInt ();

    
    Pdu_User_Data_Header *header = NULL;
    
    if ( identifier == IEI_CONCATENATED )
    {
        header = new Pdu_Concatenated ( identifier, length );
    }
    else
    {
        header = new Pdu_User_Data_Header ( identifier, length );
    }
    
    header->decode( packed ); // grab the body

    return header;
}


/**  Decode a concatenated user data message
 *
 */
void Pdu_Concatenated::decode ( Pdu_Packed *packed )
{
    ref_ = packed->getOctetAsInt ();
    max_ = packed->getOctetAsInt ();
    seq_ = packed->getOctetAsInt ();
}


void Pdu_Concatenated::dump ( void ) const
{
    Pdu_User_Data_Header::dump();
    PDU_LOG->log ( "  Reference : %d\n", ref_);
    PDU_LOG->log ( "  Max : %d\n", max_);
    PDU_LOG->log ( "  Sequence : %d\n", seq_);
}



