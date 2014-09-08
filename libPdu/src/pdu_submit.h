/*
 *  pdu_submit.h
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#ifndef PDU_SUBMIT_H
#define PDU_SUBMIT_H
#include "pdu_base.h"

#include "common.h"

class PDU_DECODE_API Pdu_Submit : public Pdu_Base {
public:
	Pdu_Submit();
    
    void dump () { Pdu_Base::dump(); }
};
#endif



