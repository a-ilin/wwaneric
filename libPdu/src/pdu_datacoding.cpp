/*
 *  pdu_datacoding.cpp
 *  libPDU
 *
 *  Created by James Pitts on 11/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_datacoding.h"
#include "pdu_log.h"

void Pdu_Data_Coding::dump ( void ) const
{
    PDU_LOG->log ( "Pdu_Data_Coding dump\n");
    
    PDU_LOG->log ( "  dataCode : %#x\n", dataCoding_ );
    
    if ( isGeneralDataEncoding() )
        PDU_LOG->log ( "  General Data Encoding : true\n" );
    else
        PDU_LOG->log ( "  General Data Encoding : false\n" );
    
    if ( isCompressed() )
        PDU_LOG->log ( "  Compressed : true\n" );
    else
        PDU_LOG->log ( "  Compressed : false\n" );
    
    if ( isDefaultAlphabet() )
        PDU_LOG->log ( "  Default Alphabet : true\n" );
    else
        PDU_LOG->log ( "  Default Alphabet : false\n" );
    

    switch ( getMessageClass() )
    {
        case MC_ALERT:
            PDU_LOG->log ( "  Message Class : MC_ALERT\n" );
            break;
        case MC_ME_SPECIFIC:
            PDU_LOG->log ( "  Message Class : MC_ME_SPECIFIC\n" );
            break;
        case MC_SIM_SPECIFIC:
            PDU_LOG->log ( "  Message Class : MC_SIM_SPECIFIC\n" );
            break;
        case MC_TE_SPECIFIC:
            PDU_LOG->log ( "  Message Class : MC_TE_SPECIFIC\n" );
            break;
        default:
            PDU_LOG->log ( "  Message Class : UNKNOWN %#x\n", getMessageClass() );
            break;
            
    }
 
    switch ( getAlphabet() )
    {
        case ALPHABET_DEFAULT:
            PDU_LOG->log ( "  Alphabet : ALPHABET_DEFAULT\n" );
            break;
        case ALPHABET_8BIT:
            PDU_LOG->log ( "  Alphabet : ALPHABET_8BIT\n" );
            break;
        case ALPHABET_UCS2:
            PDU_LOG->log ( "  Alphabet : ALPHABET_UCS2\n" );
            break;
        case ALPHABET_RESERVED:
            PDU_LOG->log ( "  Alphabet : ALPHABET_RESERVED\n" );
            break;
        default:
            PDU_LOG->log ( "  Alphabet : UNKNOWN %#x\n", getAlphabet() );
            break;
    }
}

/*
 *      isGeneralDataEncoding
 *
 *  Are bits 7 and 6 of data encoding zero?
 */
bool Pdu_Data_Coding::isGeneralDataEncoding ( void ) const
{
    return (dataCoding_ & 0xC0) == 0;
}

/*
 *
 */
bool Pdu_Data_Coding::isCompressed ( void ) const
{
    if ( isGeneralDataEncoding () == true )
        if ( ( dataCoding_ & 0x10 ) == 0x10 )
            return true;  // compressed
    
    return false;
}

/*
 *
 */
int Pdu_Data_Coding::getMessageClass ( void ) const
{

    if ( isMessageClass () )
    {
        switch ( dataCoding_ & 0x3 )
        {
            case MC_ALERT:
                return MC_ALERT;
                break;
            case MC_ME_SPECIFIC:
                return MC_ME_SPECIFIC;
                break;
            case MC_SIM_SPECIFIC:
                return MC_SIM_SPECIFIC;
                break;
            case MC_TE_SPECIFIC:
                return MC_TE_SPECIFIC;
                break;
        }

      return -1;
    }


    if ( ( dataCoding_ & 0x10 ) == 0)
        return MC_ALERT;
    
    if ( ( dataCoding_ & 0x10 ) == 0x10 ) // bit 4 implies bit 1..0 has meaning
    {
        switch ( dataCoding_ & 0x3 )
        {
            case MC_ALERT:
                return MC_ALERT;
                break;
            case MC_ME_SPECIFIC:
                return MC_ME_SPECIFIC;
                break;
            case MC_SIM_SPECIFIC:
                return MC_SIM_SPECIFIC;
                break;
            case MC_TE_SPECIFIC:
                return MC_TE_SPECIFIC;
                break;
        }
    }
    
    return -1;
}

/**
 *   return the encodings alphabet
 */
int Pdu_Data_Coding::getAlphabet ( void ) const 
{
    if ( isGeneralDataEncoding () )
    {
        switch ( ( dataCoding_ & 0x0C ) >> 2 )
        {
            case ALPHABET_DEFAULT:
                return ALPHABET_DEFAULT;
                break;
            case ALPHABET_8BIT:
                return ALPHABET_8BIT;
                break;
            case ALPHABET_UCS2:
                return ALPHABET_UCS2;
                break;
            case ALPHABET_RESERVED:
                return ALPHABET_RESERVED;
                break;
        }
    } else 
    if ( isMessageClass () )
    {
        switch ( ( dataCoding_ & 0x04 ) >> 2 )
        {
            case ALPHABET_DEFAULT:
                return ALPHABET_DEFAULT;
                break;
            case ALPHABET_8BIT:
                return ALPHABET_8BIT;
                break;
        }
    }
    
    return -1;
}

bool Pdu_Data_Coding::isMessageClass ( void ) const
{
    return (dataCoding_ & 0xF0) == 0xF0;
}

/*
 *
 */
bool Pdu_Data_Coding::isDefaultAlphabet (void) const
{
    if ( isGeneralDataEncoding () == true || isMessageClass () )
    {
        if ( getAlphabet () == ALPHABET_DEFAULT )
        {
            return true;
        }
    }
    
    return false;
}

