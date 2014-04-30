/*
 *  pdu_log.cpp
 *  libPDU
 *
 *  Created by James Pitts on 08/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_log.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

Pdu_Log *Pdu_Log::logger_ = 0;

Pdu_Log::Pdu_Log ()
{
}

Pdu_Log *Pdu_Log::instance ()
{
    if ( logger_ == 0 )
    {
        logger_ = new Pdu_Log();
        logger_->setCallback ( 0 );
    }
    
    return logger_;
}

void Pdu_Log::log (const char *str, ... )
{
    va_list ap;
    va_start(ap, str);
    
    char *buf = new char [1024];
    
    vsnprintf( buf, 1023, str, ap);
    
    if (callback_ != 0)
        callback_ ( buf );
    else
        printf ( buf );
    
    delete buf;
}

void Pdu_Log::setCallback ( callback *cb)
{
    callback_ = cb;
}


/**
 *      Hex Dump orginally by Steffen Dettmer
 *      http://sws.dett.de/layout=simple/mini/hexdump-c/
 *      @author Steffen Dettmer
 */
void Pdu_Log::hexDump(const void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
    * [0000] 75 6E 6B 6E 6F 77 6E 20
    *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
    * (in a single line of course)
    */
    
    if (size > 256) size = 256;
    
    const unsigned char *p = (const unsigned char*) data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
                     ((unsigned int)p-(unsigned int)data) );
        }
        
        c = *p;
        if (isprint(c) == 0) {
            c = '.';
        }
        
        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);
        
        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);
        
        if(n%16 == 0) { 
            /* line completed */
            log("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }
    
    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        log("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

