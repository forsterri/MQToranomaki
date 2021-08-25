//  "MQ Tora no Maki" sample program
//  [type] request-reply

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* includes for MQI */
#include <cmqc.h>

#define QMGR_NAME	   "PTP\0"      // Name of the queue manager to be used. 
                                        // Change name as needed 
#define REQUEST_Q_NAME	   "PTP.REQ\0"  // Name of the queue to put the messages in. 
                                        // Change name as needed 
#define REPLY_Q_NAME	   "PTP.REP\0"  // Name of the queue to get the messages out. 
                                        // Change name as needed 

int main(int argc, char **argv) 
{
  /*   Declare MQI structures needed                                */
  MQOD  odreq = {MQOD_DEFAULT};    // Object Descriptor for request queue
  MQOD  odrep = {MQOD_DEFAULT};    // Object Descriptor for reply queue
  MQMD     md = {MQMD_DEFAULT};    // Message Descriptor
  MQMD    gmd = {MQMD_DEFAULT};    // Message Descriptor
  MQPMO   pmo = {MQPMO_DEFAULT};   // put message options
  MQGMO   gmo = {MQGMO_DEFAULT};   // get message options 

  MQHCONN  Hcon;                   // connection handle 
  MQHOBJ   Hobj_request;           // request object handle
  MQHOBJ   Hobj_reply;             // reply object handle
  MQLONG   O_options  = 0;         // MQOPEN options
  MQLONG   C_options  = 0;         // MQCLOSE options
  MQLONG   CompCode   = 0;         // completion code
  MQLONG   OpenCode   = 0;         // MQOPEN completion code
  MQLONG   Reason     = 0;         // reason code
  MQLONG   CReason    = 0;         // MQCONN reason code
  MQLONG   buflen     = 0;         // buffer length
  MQLONG   messlen    = 0;         // message length
  char     buffer[100];            // message buffer
  char     QMName[50];             // queue manager name

  printf("Request/Reply Sample - Reply program\n");

  // Connect to queue manager
  strcpy(QMName, QMGR_NAME);

  MQCONN(  QMName,         // queue manager name
           &Hcon,          // (return) connection handle
           &CompCode,      // (return) completion code
           &CReason );     // (return) reason code

  // report reason and stop if it failed
  if( CompCode == MQCC_FAILED )
  {
    printf("MQCONN ended with reason code %ld\n", CReason);
    exit( (int) CReason );
  }

  // because initializing by MQOD_DEFAULT macro, 
  // there is no need settig od.ObjetType=MQOT_Q

  // Set the request queue name in the object descriptor.
  strcpy(odreq.ObjectName, REQUEST_Q_NAME );
  printf("The request queue is  %s\n", odreq.ObjectName);
 
  // Open the request message queue for input                    
  O_options = MQOO_INPUT_SHARED        // open queue for input (MQGET)
            + MQOO_FAIL_IF_QUIESCING;  // but not if MQM stopping 

  MQOPEN(  Hcon,             // connection handle
           &odreq,              // MQOD structure
           O_options,        // options
           &Hobj_request,    // (return) object handle for request queue
           &CompCode,        // (return) completion code
           &Reason );        // (return) reason

  OpenCode = CompCode;

  // report reason, if any; stop if failed
  if(Reason != MQRC_NONE)
  {
    printf("MQOPEN ended with reason code %ld\n", Reason);
  }
  else
  {
    // MQGET  : read request message from request-queue
    // Set message descriptor and put message options on version 2
    gmd.Version = MQMD_VERSION_2;

    // initializing MQMD.ReplyToQ & MQMD.ReplyToQMgr
    // memset( gmd.ReplyToQMgr, '\0', (size_t)MQ_Q_MGR_NAME_LENGTH );
    // memset( gmd.ReplyToQ,    '\0', (size_t)MQ_Q_NAME_LENGTH );

    gmo.Version = MQMD_VERSION_2;

    gmo.Options = MQGMO_WAIT           // wait for new messages
                + MQGMO_CONVERT;       // convert if necessary 
    gmo.WaitInterval = MQWI_UNLIMITED; // Waiting infinitely. But in normal case, 
                                       // not recommended
    buflen = sizeof(buffer)-1;

    // returned gmd structure from this MQGET,  
    // we use gmd.ReplyToQMgr, gmd.ReplyToQ, gmd.Msg.Id
    // to back response message
    MQGET(  Hcon,           // connection handle
            Hobj_request,   // object handle for request queue
            &gmd,           // MQMD structre
            &gmo,           // MQGMO structure
            buflen,         // buffer length
            buffer,         // (return) buffer which will store request message content
            &messlen,       // (return) actual length of request message
            &CompCode,      // (return) completion code
            &Reason );      // (return) reason code

    /* report reason, if any */
    if (Reason != MQRC_NONE)
    {
      printf("MQGET ended with reason code %ld\n", Reason);
    }
    else
    { 
      // display request message
      printf("The request message   <%s>\n",buffer);

      // reply message operation

      // set MQOD structure
      odrep.Version = MQOD_VERSION_2;

      // set the ReplyToQ name for MQOPEN
      memset( odrep.ObjectName,     '\0', (size_t)MQ_Q_NAME_LENGTH );
      memcpy( odrep.ObjectName,     gmd.ReplyToQ,    (size_t)MQ_Q_NAME_LENGTH ); 
      printf("The reply queue is    %s \n", odrep.ObjectName );

      // set the ReplyToQMgr name for MQOPEN (but not necessary for this sample)
      memset( odrep.ObjectQMgrName, '\0', (size_t)MQ_Q_MGR_NAME_LENGTH );
      memcpy( odrep.ObjectQMgrName, gmd.ReplyToQMgr, (size_t)MQ_Q_MGR_NAME_LENGTH );

      // set MQPUT options
      O_options = MQOO_OUTPUT              // open queue for output 
		+ MQOO_FAIL_IF_QUIESCING;  // but not if MQM stopping

      // set MQPMO structure
      pmo.Version  = MQPMO_VERSION_2;
      pmo.Options |= MQPMO_NEW_MSG_ID;
      
      // set MQMD structure
      md.Version  = MQMD_VERSION_2;
      md.CodedCharSetId = MQCCSI_Q_MGR;

      memcpy( md.Format,           // character string format 
              MQFMT_STRING,        // string format. so MQ can convert message contents
              (size_t)MQ_FORMAT_LENGTH );

      // Setting the correlId from gmd.MsgId
      strcpy(md.CorrelId, gmd.MsgId);  //, sizeof(md.MsgId));

      strcpy(buffer, "This is a reply message demonstrating simple Request/Reply");
      messlen = strlen(buffer) + 1;

      MQPUT1(  Hcon,         // connection handle
               &odrep,       // MQOD structure
               &md,          // MQMD structure
               &pmo,         // MQPMO structure
               messlen,      // message length
               buffer,       // message contents
               &CompCode,    // completion code
               &Reason);     // reason code

      /* report reason, if any */
      if (Reason != MQRC_NONE)
      {
        printf("MQPUT1 ended with reason code %ld\n", Reason);
      }


    }

    //   Close the target queue (if it was opened)
    C_options = 0;                  /* no close options             */
    MQCLOSE(Hcon,&Hobj_request,C_options,&CompCode,&Reason);

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
       printf("MQCLOSE ended with reason code %ld\n", Reason);
    }


    //   Disconnect from MQM if not already connected
   if (CReason != MQRC_ALREADY_CONNECTED)
   {
      MQDISC(&Hcon,&CompCode,&Reason);

      /* report reason, if any     */
      if (Reason != MQRC_NONE)
      {
        printf("MQDISC ended with reason code %ld\n", Reason);
      }
   }

  }

  printf("Request/Reply Sample end\n");
  return(0);
} // end of program
