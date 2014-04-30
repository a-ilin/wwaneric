/*
 *  pdu_datacoding.h
 *  libPDU
 *
 *  Created by James Pitts on 11/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef _PDU_DATACODING_H
#define _PDU_DATACODING_H

#include <sys/types.h>
#include "caststring.h"

class Pdu_Data_Coding {
  
    unsigned int dataCoding_;
    
public:
    enum { MC_ALERT = 0, MC_ME_SPECIFIC = 1, MC_SIM_SPECIFIC = 2, MC_TE_SPECIFIC = 3,
           ALPHABET_DEFAULT = 0, ALPHABET_8BIT = 1, ALPHABET_UCS2=2, ALPHABET_RESERVED = 3};

    Pdu_Data_Coding ( unsigned int octet ) : dataCoding_ ( octet ) {} ;
    Pdu_Data_Coding () : dataCoding_ ( 0 ) {};
    
    void setDataCode ( unsigned int octet ) { dataCoding_ = octet; }
    
    bool isGeneralDataEncoding ( void ) const;
    bool isMessageClass ( void ) const;
    bool isCompressed ( void ) const;
    int  getMessageClass ( void ) const;
    int  getAlphabet ( void ) const;
    bool isDefaultAlphabet ( void ) const;
    
    void dump ( void ) const;

    const char *getEncodingAsCStr ( void ) const { return cast_string ( dataCoding_ ).c_str(); }
    
};

#endif



