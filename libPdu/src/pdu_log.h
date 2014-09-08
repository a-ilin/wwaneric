/*
 *  pdu_log.h
 *  libPDU
 *
 *  Created by James Pitts on 08/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef _PDU_LOG_H
#define _PDU_LOG_H

#include <stdarg.h>

#include "common.h"

#define PDU_LOG Pdu_Log::instance()

class PDU_DECODE_API Pdu_Log {
    
    typedef void (callback) (const char *); 

    callback *callback_;
    static Pdu_Log *logger_;
    
    Pdu_Log ();
public:
    
    static Pdu_Log *instance();
    
    void log (const char *str, ... );
    void hexDump(const void *data, int size);

    void setCallback ( callback *cb) ;
};



#endif

