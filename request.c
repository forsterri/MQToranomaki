//  "MQ Tora no Maki" sample program
//  [type] request-reply

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// includes for MQI 
#include <cmqc.h>

#define QMGR_NAME	 "PTP\0"     // Name of the queue manager to be used. 
                                     // Change name as needed 
#define REQUEST_Q_NAME   "PTP.REQ\0" // Name of the queue to put the request 
                                     // messages in. Change name as needed 
#define REPLY_Q_NAME     "PTP.REP\0" // Name of the queue to put the reply 
                                     // messages in. Change name as needed 

int main(int argc, char **argv)
{
  // Declare MQI structures needed
  MQOD  odreq = {MQOD_DEFAULT};    // Object Descriptor for request 
  MQOD  odrep = {MQOD_DEFAULT};    // Object Descriptor for reply
  MQMD     md = {MQMD_DEFAULT};    // Message Descriptor 
  MQMD    gmd = {MQMD_DEFAULT};    // Message Descriptor
  MQPMO   pmo = {MQPMO_DEFAULT};   // put message options (MQPUT)
  MQGMO   gmo = {MQGMO_DEFAULT};   // get message options (MQGET)

  MQHCONN  Hcon;                   // connection handle, return of MQCONN
  MQHOBJ   Hobj_request;           // request object handle, return of MQOPEN
  MQHOBJ   Hobj_reply;             // reply object handle, return of MQOPEN
  MQLONG   O_options = 0;          // MQOPEN options
  MQLONG   C_options = 0;          // MQCLOSE options
  MQLONG   CompCode  = 0;          // completion code
  MQLONG   OpenCode  = 0;          // MQOPEN completion code
  MQLONG   Reason    = 0;          // reason code
  MQLONG   CReason   = 0;          // MQCONN reason code
  MQLONG   buflen    = 0;          // buffer length
  MQLONG   messlen   = 0;          // message length 
  char     buffer[100];            // message buffer
  char     QMName[50];             // queue manager name

  printf("Request/Reply Sample - Request program\n");

  //   Connect to queue manager
  strcpy(QMName, QMGR_NAME);
  MQCONN( QMName,                  // queue manager name
          &Hcon,                   // (return) connection handle
          &CompCode,               // (return) completion code
          &CReason);               // (return) reason code

  // report reason and stop if it failed
  if (CompCode == MQCC_FAILED) 
  {
    printf("MQCONN ended with reason code %ld\n", CReason);
    exit( (int) CReason );
  }

  // Set the request queue name in the object descriptor.
  strcpy(odreq.ObjectName, REQUEST_Q_NAME );
  printf("The request queue is %s\n", odreq.ObjectName);

  // Open the request message queue for output                    
  O_options = MQOO_OUTPUT             // open queue for output (MQPUT)
            + MQOO_FAIL_IF_QUIESCING; // but not if MQM stopping

  MQOPEN( Hcon,                     // connection handle
          &odreq,                   // pointer of MQOD structure
          O_options,                // MQOPEN options
          &Hobj_request,            // (return) object handle
          &CompCode,                // (return) completion code
          &Reason );                // (return) reason code

  // report reason, if any; stop if failed
  if( Reason != MQRC_NONE )
  {
    printf("MQOPEN ended with reason code %ld\n", Reason);
  }

  OpenCode = CompCode;

  // Set the reply queue name in the object descriptor.
  strcpy(odrep.ObjectName, REPLY_Q_NAME );
  printf("The reply queue is   %s\n", odrep.ObjectName);

  // Open the reply message queue for input 
  O_options = MQOO_INPUT_SHARED        // open queue for output (MQGET)
            + MQOO_FAIL_IF_QUIESCING;  // but not if MQM stopping

  MQOPEN( Hcon,               // connection handle
          &odrep,             // MQOD structure for Reply-to Q
          O_options,          // MQOPEN options
          &Hobj_reply,        // (return) object handle
          &CompCode,          // completion code
          &Reason );          // reason code

  // report reason, if any; stop if failed
  if (Reason != MQRC_NONE)
  {
    printf("MQOPEN ended with reason code %ld\n", Reason);
  }
 
  // If any of them fail to open then the program exits
  OpenCode |= CompCode;

  if (OpenCode == MQCC_FAILED)
  {
    printf("Unable to open queue for output\n");
  }
  else
  {

    memcpy( md.Format,           // contents of request message are made from chars.
            MQFMT_STRING,        // character string format
            (size_t)MQ_FORMAT_LENGTH );

    // Set message descriptor and put message options on version 2
    md.Version = MQMD_VERSION_2;
    pmo.Version = MQPMO_VERSION_2;
    gmo.Version = MQMD_VERSION_2;

    // Request a new messageId and correlID for this message
    pmo.Options |= MQPMO_NEW_MSG_ID;
    pmo.Options |= MQPMO_NEW_CORREL_ID;


    // initializing MQMD.ReplyToQMgr & MQMD.ReplyToQ
    memset( md.ReplyToQMgr, '\0', (size_t)MQ_Q_MGR_NAME_LENGTH );
    memset( md.ReplyToQ,    '\0', (size_t)MQ_Q_NAME_LENGTH );

    // set reply to queue name into MQMD.ReplyToQ & MQMD.ReplyToQMgr
    strncpy( md.ReplyToQMgr, QMGR_NAME, sizeof(QMGR_NAME) ); 
    strncpy( md.ReplyToQ, REPLY_Q_NAME, sizeof(REPLY_Q_NAME) );

    strcpy(buffer, "This is a request message demonstrating simple Request/Reply\0");
    messlen = strlen(buffer) + 1;

    MQPUT( Hcon,               // connection handle
           Hobj_request,       // object handle for Request queue
           &md,                // MQMD structure
           &pmo,               // MQPMO structure
           messlen,            // message length to be MQPUT
           buffer,             // message content
           &CompCode,          // (return) completion code
           &Reason );          // (return) reason code

    /* report reason, if any */
    if (Reason != MQRC_NONE)
    {
      printf("MQPUT ended with reason code %ld\n", Reason);
    }

    if (OpenCode != MQCC_FAILED)
    {
      gmo.Options = MQGMO_WAIT      // wait for new message arrival
                  + MQGMO_CONVERT;  // convert if necessary
      gmo.WaitInterval = 15 * 1000; // 15 second limit for waiting (msec) 

      buflen = sizeof(buffer);       // buffer length 
      memset(buffer, '\0', buflen);  // initializing buffer

      // copy CorrelID from request message's MQMD, which is MQBYTE24 (binary)
      strcpy( gmd.CorrelId,          // CorrelId for next MQGET
              md.MsgId  );           // request message's MsglId (return of MQPUT)
      gmo.MatchOptions = MQMO_MATCH_CORREL_ID; 


      // because of MQMO_MATCH_CORREL_ID,  we will receive only response message
      MQGET( Hcon,             // connection handle
             Hobj_reply,       // object handle for reply queue
             &gmd,             // MQGMO structure
             &gmo,             // MQMD structure for MQGET
             buflen,           // buffer length 
             buffer,           // buffer itself
             &messlen,         // (return) actual message length
             &CompCode,        // (return) completion code
             &Reason );        // (return) reason code

      // report reason, if any 
      if (Reason != MQRC_NONE)
      {
        printf("MQGET ended with reason code %ld\n", Reason);
      }
      else
      { // i.e: MQRC_NONE,  MQGET without any errors
        // display reply message
        printf("The reply message    <%s>\n",buffer);
      }

      //   Close the request queue (if it was opened)
      C_options = 0;               // no close options 
      MQCLOSE( Hcon,               // connection handle
               &Hobj_request,      // object handle for request queue to be released
               C_options,          // close option
               &CompCode,          // (return) completion code
               &Reason );          // (return) reason code

      // report reason, if any 
      if (Reason != MQRC_NONE)
      {
        printf("MQCLOSE for %s ended with reason code %ld\n", odreq.ObjectName, Reason);
      }

      //   Close the reply queue (if it was opened)
      C_options = 0;               // no close options 
      MQCLOSE( Hcon,               // connection handle
               &Hobj_reply,        // object handle for reply queue to be released
               C_options,          // close option
               &CompCode,          // (return) completion code
               &Reason );          // (return) reason code

      /* report reason, if any     */
      if (Reason != MQRC_NONE)
      {
        printf("MQCLOSE for %s ended with reason code %ld\n", odrep.ObjectName, Reason);
      }
    }
  }

  //   Disconnect from MQM if not already connected
  if(CReason != MQRC_ALREADY_CONNECTED)
  {
    MQDISC(&Hcon,&CompCode,&Reason);

    /* report reason, if any     */
    if(Reason != MQRC_NONE)
    {
      printf("MQDISC ended with reason code %ld\n", Reason);
    }
  }

  printf("Request/Reply Sample end\n");
  return(0);
}  // end of program
