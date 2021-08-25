
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 /* includes for MQI  */
#include <cmqc.h>

#define QMGR_NAME	"PTP"     /* Name of the queue manager to be used. Change name as needed */
#define Q_NAME		"PTP.LQ"  /* Name of the queue to get the messages out. Change name as needed */

int main(int argc, char **argv) {
	/*   Declare MQI structures needed                                */
	MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
	MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
	MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */
	  /** note, sample uses defaults where it can **/

	MQHCONN  Hcon;                   /* connection handle             */
	MQHOBJ   Hobj;                   /* object handle                 */
	MQLONG   O_options;              /* MQOPEN options                */
	MQLONG   C_options;              /* MQCLOSE options               */
	MQLONG   CompCode;               /* completion code               */
	MQLONG   OpenCode;               /* MQOPEN completion code        */
	MQLONG   Reason;                 /* reason code                   */
	MQLONG   CReason;                /* reason code for MQCONN        */
	MQBYTE   buffer[101];            /* message buffer                */
	MQLONG   buflen;                 /* buffer length                 */
	MQLONG   messlen;                /* message length received       */
	char     QMName[50];             /* queue manager name            */

	printf("Consumer sample\n");

	//   Create object descriptor for subject queue
	strcpy(od.ObjectName, Q_NAME);
	strcpy(QMName, QMGR_NAME);

	//   Connect to queue manager
	MQCONN(QMName,&Hcon,&CompCode,&CReason);

	/* report reason and stop if it failed     */
	if (CompCode == MQCC_FAILED) {
		printf("MQCONN ended with reason code %ld\n", CReason);
		exit( (int)CReason );
	}

	// Open the named message queue for input shared 
	O_options = MQOO_INPUT_SHARED    /* open queue for input         */
		 + MQOO_FAIL_IF_QUIESCING;   /* but not if MQM stopping      */

	MQOPEN(Hcon,&od,O_options,&Hobj,&OpenCode,&Reason);

	/* report reason, if any; stop if failed      */
	if (Reason != MQRC_NONE) {
		printf("MQOPEN ended with reason code %ld\n", Reason);
	}

	if (OpenCode == MQCC_FAILED) {
		printf("unable to open queue for input\n");
	}


	// These options cause the MsgId and CorrelId to be replaced, so  
	// that there is no need to reset them before each MQGET          
	gmo.Version = MQGMO_VERSION_2;      /* Avoid need to reset Message */
	gmo.MatchOptions = MQMO_NONE;       /* ID and Correlation ID after */
									    /* every MQGET                 */
	gmo.Options = MQGMO_WAIT            /* wait for new messages       */
			   + MQGMO_CONVERT;         /* convert if necessary        */
	gmo.WaitInterval = 15000;           /* 15 second limit for waiting */

	buflen = sizeof(buffer) - 1;        /* buffer size available for GET */

	/****************************************************************/
	/*                                                              */
	/*   MQGET sets Encoding and CodedCharSetId to the values in    */
	/*   the message returned, so these fields should be reset to   */
	/*   the default values before every call, as MQGMO_CONVERT is  */
	/*   specified.                                                 */
	/*                                                              */
	/****************************************************************/

	md.Encoding       = MQENC_NATIVE;
	md.CodedCharSetId = MQCCSI_Q_MGR;

	MQGET(Hcon,Hobj,&md,&gmo,buflen,buffer,&messlen,&CompCode,&Reason);

	/* report reason, if any     */
	if (Reason != MQRC_NONE) {
		if (Reason == MQRC_NO_MSG_AVAILABLE) {                         
			/* special report for normal end    */
			printf("no more messages\n");
		} else { /* general report for other reasons */
			printf("MQGET ended with reason code %ld\n", Reason);

			/*   treat truncated message as a failure for this sample   */
			if (Reason == MQRC_TRUNCATED_MSG_FAILED) {
				CompCode = MQCC_FAILED;
			}
		}
	}

	/****************************************************************/
	/*   Display each message received                              */
	/****************************************************************/
	if (CompCode != MQCC_FAILED) {
		buffer[messlen] = '\0';            /* add terminator          */
		printf("message <%s>\n", buffer);
	}

	/******************************************************************/
	/*                                                                */
	/*   Close the source queue (if it was opened)                    */
	/*                                                                */
	/******************************************************************/
	if (OpenCode != MQCC_FAILED) {
		C_options = 0;                   /* no close options            */
		MQCLOSE(Hcon,&Hobj,C_options,&CompCode,&Reason);

		/* report reason, if any     */
		if (Reason != MQRC_NONE) {
			printf("MQCLOSE ended with reason code %ld\n", Reason);
		}
	}

	//   Disconnect from MQM if not already connected
	if (CReason != MQRC_ALREADY_CONNECTED ) {
		MQDISC(&Hcon,&CompCode,&Reason);

		/* report reason, if any     */
		if (Reason != MQRC_NONE) {
			printf("MQDISC ended with reason code %ld\n", Reason);
		}
	}

	printf("Consumer sample end\n");
	return(0);
}
