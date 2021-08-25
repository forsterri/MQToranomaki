
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* includes for MQI */
#include <cmqc.h>

#define QMGR_NAME	"PTP"    /* Name of the queue manager to be used. Change name as needed */
#define Q_NAME		"PTP.LQ" /* Name of the queue to put the messages in. Change name as needed */

int main(int argc, char **argv) {
	/*   Declare MQI structures needed                                */
	MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
	MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
	MQPMO   pmo = {MQPMO_DEFAULT};   /* put message options           */

	MQHCONN  Hcon;                   /* connection handle             */
	MQHOBJ   Hobj;                   /* object handle                 */
	MQLONG   O_options;              /* MQOPEN options                */
	MQLONG   C_options;              /* MQCLOSE options               */
	MQLONG   CompCode;               /* completion code               */
	MQLONG   OpenCode;               /* MQOPEN completion code        */
	MQLONG   Reason;                 /* reason code                   */
	MQLONG   CReason;                /* MQCONN reason code            */
	MQLONG   messlen;                /* message length                */
	char     buffer[100];            /* message buffer                */
	char     QMName[50];             /* queue manager name            */

	printf("Send/Forget Sample");

	//   Connect to queue manager
	strcpy(QMName, QMGR_NAME);
	MQCONN(QMName,&Hcon,&CompCode,&CReason);

	/* report reason and stop if it failed     */
	if (CompCode == MQCC_FAILED) {
		printf("MQCONN ended with reason code %ld\n", CReason);
		exit( (int) CReason );
	}

	// Set queue name in the object descriptor
	strcpy(od.ObjectName, Q_NAME);
	printf("Target queue is %s\n", od.ObjectName);

	// Open the target message queue for output                    
	O_options = MQOO_OUTPUT           /* open queue for output        */
		   + MQOO_FAIL_IF_QUIESCING; /* but not if MQM stopping      */
	MQOPEN(Hcon,&od,O_options,&Hobj,&OpenCode,&Reason);

	/* report reason, if any; stop if failed      */
	if (Reason != MQRC_NONE) {
		printf("MQOPEN ended with reason code %ld\n", Reason);
	}

	if (OpenCode == MQCC_FAILED) {
		printf("Unable to open queue for output\n");
	} else {

		memcpy(md.Format,           /* character string format            */
			  MQFMT_STRING, (size_t)MQ_FORMAT_LENGTH);

		// Set message descriptor and put message options on version 2
		md.Version = MQMD_VERSION_2;
		pmo.Version = MQPMO_VERSION_2;

		// Request a new messageId for this message
		pmo.Options |= MQPMO_NEW_MSG_ID;

		strcpy(buffer, "This is a simple Send/Forget sample");
		messlen = strlen(buffer);

		MQPUT(Hcon,Hobj,&md,&pmo,messlen,buffer,&CompCode,&Reason);

		/* report reason, if any */
		if (Reason != MQRC_NONE) {
			printf("MQPUT ended with reason code %ld\n", Reason);
		}

		//   Close the target queue (if it was opened)
		if (OpenCode != MQCC_FAILED) {
			C_options = 0;                  /* no close options             */
			MQCLOSE(Hcon,&Hobj,C_options,&CompCode,&Reason);

			/* report reason, if any     */
			if (Reason != MQRC_NONE) {
				printf("MQCLOSE ended with reason code %ld\n", Reason);
			}
		}
	}

	//   Disconnect from MQM if not already connected
	if (CReason != MQRC_ALREADY_CONNECTED) {
	 MQDISC(&Hcon,&CompCode,&Reason);

		 /* report reason, if any     */
		if (Reason != MQRC_NONE) {
			printf("MQDISC ended with reason code %ld\n", Reason);
		}
	}

	printf("Send/Forget Sample end\n");
	return(0);
}
