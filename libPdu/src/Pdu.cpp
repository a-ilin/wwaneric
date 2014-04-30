/*
 *  Pdu.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "Pdu.h"
#include "pdu_deliver.h"

/*
 *      Pdu
 */
Pdu::Pdu( )
{


}

Pdu::~Pdu ()
{

}

/**  create
 *
 */
Pdu *Pdu::create ( const std::string &pdu )
{
  Pdu base ( pdu );


  try {

    base.decode();

    if (base.getMTI() == Pdu_Base::SMS_DELIVER)
    {
      Pdu_Deliver *deliver = new Pdu_Deliver ( base );
      if (deliver)
        deliver->decode();
      return deliver;
    }

  } catch ( const char *e ) {
    PDU_LOG->log ( "Caught an exception %s(%d): %s\n", __FILE__, __LINE__, e );
  } catch ( ... ) {
    PDU_LOG->log ( "Caught an unknown exception %s(%d)\n", __FILE__, __LINE__ );
  }

  base.dump();

  return 0;
}


/**  A support method for a common operation.
 *
 */
const std::string Pdu::keyword ( const std::string &str )
{

  std::string keyword ( "" );
  unsigned int pos = str.find ( ' ' );
  if (pos == std::string::npos)
  {
    keyword = str;
  }
  else
  {
    keyword = str.substr (0,pos);
  }

  return keyword;
}

/**  A support method for a common operation.
 *
 */
const std::string Pdu::text ( const std::string &str )
{
  unsigned int pos = str.find (' ');

  if (pos == std::string::npos)
  {
    // if no keyword lets return just the whole message
    return str;
  }

  return str.substr (pos + 1);
}
