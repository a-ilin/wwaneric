/*
 *  pdu_packed.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include "pdu_packed.h"
#include "pdu_address.h"
#include "pdu_log.h"

#include <climits>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string>
#include <exception>

/**
 *
 * @param packed the raw pdu as from the ME.
 */
Pdu_Packed::Pdu_Packed ( std::string packed ) :
    Pdu_7Bit_Packing ( this ), Pdu_8Bit_Packing ( this ),
    packed_ ( packed ), current_( 0 )
{
}

/**
 *  copy constructor ensuring the 7 and 8 bit packing maintain the correct location of this object.
 */
Pdu_Packed::Pdu_Packed ( const Pdu_Packed &that ) : Pdu_7Bit_Packing ( this ), Pdu_8Bit_Packing ( this ),
  packed_ ( that.packed_ ), current_ ( that.current_ )
{
}

/**
 *	getOctetAsInt will get an octet from the pdu, decode it and return it's value.
 *
 * @return value of octet
 */
unsigned int Pdu_Packed::getOctetAsInt ( void )
{

    unsigned int val = 0;

    try {

      val =  ::strtol  ( packed_.substr(current_,2).c_str(), NULL, 16);

      if (val == LONG_MAX || val == LONG_MIN )
        throw "strtol failed : "__FILE__;

      current_ += 2;
     } catch ( std::exception e ) {

      throw "getOctectAsInt failed : "__FILE__;
     }

    return val;
}

/**
 *
 */
unsigned int Pdu_Packed::getSemiOctetAsInt ( void )
{

    if (current_ + 2 > packed_.length() )
    {
      throw "getOctectAsInt failed : "__FILE__;
    }

    unsigned int val =  ::strtol  ( packed_.substr(current_,2).c_str(), NULL, 16);

    if (val == LONG_MAX || val == LONG_MIN )
      throw "strtol failed : "__FILE__;

    current_ += 2;

    val = ( (val & 0xF0) >> 4) | ( ( val & 0x0F ) << 4 );

    return val;
}


/**
 *
 */
Pdu_Packed::~Pdu_Packed ()
{
}

/**
 *	getSemiOctetAsString
 *
 *
 *  length is the count of _octets_
 *  @param dest destination for the string
 *  @param length length in semioctets of string
 */
void Pdu_Packed::getSemiOctetAsString  ( std::string &dest, unsigned int length )
{

    if ( length <= 0 )
      throw "Length <= 0 : "__FILE__;

    if ( current_ + length > packed_.length() )
      throw "Request for  > Pdu length  : "__FILE__;

    unsigned int i = 0;
    dest = "";



    for (i = 0; i < length; i+=2)
    {

        dest += packed_[current_+i+1];

        if (packed_[current_+i] != 'F')
            dest += packed_[current_+i];

    }

    current_ += length;
}

/**
 *  gets a raw string from the pdu of length octets
 *
 *  @param dest destination of the string
 *  @param lenmgth length in octets of the string
 *
 */
void Pdu_Packed::getOctetAsString ( std::string &dest, int length )
{
    if ( length <= 0 )
      throw "Length <= 0 : "__FILE__;

    if ( current_ + length > packed_.length() )
      throw "Request for  > Pdu length  : "__FILE__;

    length *= 2;  // convert to semi octets
    dest  = packed_.substr ( current_, length );
    current_ += length;
}

/**
 *      Gets an address field from the current location in the pdu.
 *
 * @param addr destination for the address
 * @param lengthAsOctets true if the address length field in the pdu is encoded as octets
 *                       against the etsi spec 03.40, often  used for the smsc address.
 */
void Pdu_Packed::getAddressField ( Pdu_Address &addr, bool lengthAsOctets )
{

    if (lengthAsOctets == true)
    {
        addr.setLengthAsOctets ( getOctetAsInt() );
    }
    else
    {
        addr.setLengthAsSemiOctets ( getOctetAsInt() );
    }

    addr.setType ( getOctetAsInt () );

    std::string number("");

    unsigned int numLen = ( addr.getLength() );

    if ( ( addr.getTypeOfNumber () == Pdu_Address::TN_INTERNATIONAL ) ||
         ( addr.getTypeOfNumber () == Pdu_Address::TN_NATIONAL ) ||
         ( addr.getTypeOfNumber () == Pdu_Address::TN_UNKNOWN )
         )
    {

        getSemiOctetAsString (number, numLen);

        addr.setValue ( QString::fromStdString(number) );

    }
    else if ( addr.getTypeOfNumber () == Pdu_Address::TN_ALPHANUMERIC )
    {
        unpack ( number, numLen );
        addr.setValue( QString::fromStdString(number) );
    }
    else
    {
        skipSemiOctets ( numLen );
    }
}

/**
 *  Converts an octet count to a septet count
 */
unsigned int Pdu_Packed::octetsToSeptets ( unsigned int octets )
{

    return (unsigned int) ceil (octets * 8 / 7.0f);
}

