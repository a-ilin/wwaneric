/*
 *  pdutest.cpp
 *  libPDU
 *
 *  Created by James Pitts on 07/02/2005.
 *  Copyright 2005 James Pitts. All rights reserved.
 *
 */

#include <cstdio>
 
#include "Pdu.h"
#include "pdu_deliver.h"
#include "pdu_userdata.h"

//#define TEST_SINGLE
//#define TEST_SIMPLE
//#define TEST_MMS
//#define TEST_MULTIPART

void decodeAndDump ( const char *pdustr )
{
    printf ( "--- PDU DUMP ---\n");
    Pdu *pdu = Pdu::create ( pdustr );

    if ( !pdu )
    {
      printf ( "decoding failed\n" );
    }
    else 
    {
    //  pdu->decode ();///  wrong..
      pdu->dump();
    }
    
/*
    if (pdu.getPdu()->getMTI() == Pdu_Base::SMS_DELIVER)
    {
        const Pdu_Deliver *deliver = pdu.getDeliver();
        deliver->dump();
        
        Pdu_User_Data::iterator iter = deliver->getUserData()->getIterator();
        while  ( iter !=  deliver->getUserData()->getEndIterator ( ) ) 
        {
            
            (*iter)->dump();
            iter++;
        }
    }
*/
    printf ( "\n" );
}

#ifdef TEST_MULTIPART
void testMultipart (void)
{
    decodeAndDump ( "0791449737019037440C91449737371204000050201102818400A00500030A0301A8E5391D442FCFE9207A794E07D1CB733A885A9ED341F4F29C0EAACFCB207A794E07D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E07D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E0751CB733A885E9ED341F57919442FCFE9207A794E07D1CB733A885E9ED341" );
    decodeAndDump ( "0791449737019037440C91449737371204000050201102812500A00500030A0302E8E5391D442FCFE9207A794E07D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E07D1CB733A885E9ED341D4F29C0EA297E774507D5E06D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E07D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9" );
    decodeAndDump ( "0791449737019037440C91449737371204000050201102815500740500030A030340F4F29C0EA297E77410B53CA783E8E5391D549F9741F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E07D1CB733A885E9ED341F4F29C0EA297E77410BD3CA783E8E5391D442FCFE9207A794E07D1CB733A885E9ED341F4F29C0E" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141726500400500030B04046835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141727400A00500030B040162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD68" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141232500A00500030C040162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD68" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141236500A00500030C04026AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD70" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141239500A00500030C040372B0986C46ABD96EB81C2C269BD16C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD68B61B2E078BC966" );
    decodeAndDump ( "0791449737019037440C91449737371204000050203141332000400500030C04046835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560" );
}
#endif

int main (int argc, char *argv[])
{


#ifdef TEST_SIMPLE
    decodeAndDump ( "0791447758100650040C914477960926520000502011813390000141" ); // "A"
    decodeAndDump ( "0791447758100650040C91447796092652000050201181330500024131" ); // "Ab"
    decodeAndDump ( "0791447758100650040C914477960926520000502011814370000341F118" ); // "Abc"
    decodeAndDump ( "0791447758100650040C914477960926520000502011815365000441F1980C" ); // "Abcd"
#endif
   
    // this is MMS
#ifdef TEST_MMS
    decodeAndDump ( "07914497370190374410D04F79D87D2E379B530004502031022284004C0B05040B8423F000033902020581031BAF7F83687474703A2F2F6D6D732E6F72616E67652E636F2E756B2F51675F334638436F344D4D41414746494141414145674141453759414141414100" );
    decodeAndDump ( "07914497370190374410D04F79D87D2E379B530004502031022284008C0B05040B8423F000033902013A06356170706C69636174696F6E2F766E642E7761702E6D6D732D6D65737361676500B13139322E3136382E3232342E31303200AF84B4818C829851675F334638436F344D4D414147464941414141456741414537594141414141008D908919802B3434373937333733323134302F545950453D504C4D4E008A808E02280088" );
#endif
    
    
#ifdef TEST_MULTIPART
   testMultipart ();
#endif

#ifdef TEST_SINGLE
                   //         1111111111222222222233333333334444444444555555555566666
                   //1234567890123456789012345678901234567890123456789012345678901234
    decodeAndDump ( "0791447758100650040C914477960926520000502011813390000141" ); // "A"
    decodeAndDump ( "06915406999099040B918496061645F339F150205100055100054132BB2C07" );
#endif

    decodeAndDump ("0791449737019037040C9144973737120400005020821271140009D4F29C0EA297E774");

    return 0;
}
