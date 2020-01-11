// ENSC 351 Assignment 3.  2019  Prepared by:
//      - Craig Scratchley, Simon Fraser University
//			with help from:
//      - Zhenwang Yao
//		- Phoenix Yuan

#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include "myIO.h"
#include "SenderX.h"
#include "ReceiverX.h"
#include "Linemax.h"
//#include "WcsQnx.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

using namespace std;

//terminal input commands
#define SEND_C		"&s"
#define RECV_C		"&r"

#define TERM_QUIT_C		"&q!"

//function used by the terminal threads, process input from the medium
//	return true when terminal should terminate.
bool MediumReady(int mediumD, int outD)
{
	char bytesReceived[BUF_SZ];

	int numOfByteReceived = PE(myRead(mediumD, bytesReceived, BUF_SZ));
	if (numOfByteReceived == 0) {
		COUT << "MediumReady finds Medium Descriptor Closed" << endl;
		return true;
	}
	PE_NOT(myWrite(outD, bytesReceived, numOfByteReceived), numOfByteReceived);
	return false;
}

//function used by the terminal threads, process input from the KeyBoard
//	return true when terminal should terminate.
bool KbReady(int inD, int outD, int term, int mediumD)
{
	char bytesReceived[LINEMAX];
	// should we make sure we get just a single line of input
	int numOfByteReceived = PE(myRead(inD, bytesReceived, LINEMAX_SAFE));
	if (numOfByteReceived == 0) {
		COUT << "TERM " << term << ": inD Closed" << endl;
		return true;
	}
	bytesReceived[numOfByteReceived]=0; // is this needed?
		
	//grab command and possibly file name from input buffer
	char cmd[LINEMAX]; // longer than necessary?
	char fname[LINEMAX];
	char useCrcOption[LINEMAX]; // longer than necessary?
	int numItemsMatched = sscanf( bytesReceived, "%s %s %s", cmd, fname, useCrcOption );
	if( numItemsMatched >= 1) {
		if (strcmp( cmd, SEND_C ) == 0) {
			//default filename
			if( numItemsMatched < 2 ) {// strlen(fname) == 0 ) {
#ifdef FAST_SIM
				strcpy(fname, "/etc/protocols");
#else
				strcpy(fname, "/etc/anacrontab");
#endif
			}
			CON_OUT(outD, "TERM " << term << ": Will request sending of '" << fname << "'"<< endl);
			SenderX xSender(fname, mediumD, inD, outD); 
			xSender.sendFile();
			CON_OUT(outD, "\nTERM " << term << ": xSender result was: " << xSender.result << endl);
			return false;
		} else if( strcmp( cmd, RECV_C ) == 0) {
			bool useCrc = true;
			//default filename
			if( numItemsMatched < 2 ) {// strlen(fname) == 0 ) {
				strcpy(fname, "/tmp/transferred-file");
			}
			else
				if( numItemsMatched == 3 ) {
					if (strcmp( useCrcOption, "-cs" ) == 0) {
						useCrc = false;
					}
				}

			CON_OUT(outD, "TERM " << term << ": Will request receiving to '" << fname << "'"<< endl);
			ReceiverX xReceiver(mediumD, fname, inD, outD, useCrc);
			xReceiver.receiveFile();
			CON_OUT(outD, "\nTERM " << term << ": xReceiver result was: " << xReceiver.result << endl);
			return false;
		} else if( strcmp( cmd, TERM_QUIT_C ) == 0) {
			CON_OUT(outD, "TERM " << term << " TERMINATING" << endl);
			return true;
		}
	} 
	int numOfBytesSent = 0;
	while((numOfBytesSent+=PE(myWrite(mediumD, bytesReceived + numOfBytesSent, numOfByteReceived - numOfBytesSent))) < numOfByteReceived)
		PE(myTcdrain(mediumD));
	return false;
}

void Terminal(int termNum, int inD, int outD, int mediumD)		
{
	// empty any amount of data that might be previously buffered
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
	// the above is dumpGlitches -- avoid this duplicated code.
	
	bool finished = false;

	fd_set set;
	FD_ZERO(&set);
	do
	{
		FD_SET(mediumD, &set);
		FD_SET(inD, &set);
		
		int rv = PE(select( max(mediumD,
							inD)+1, &set, NULL, NULL, NULL ));
		if( rv == 0 ) {
			// timeout occurred
			CERR << "This peer term (" << termNum << ") should not timeout" << endl;
			exit (EXIT_FAILURE);
		} else {
			if( FD_ISSET( mediumD, &set ) ) {
				//route message from medium back to screen
				finished = MediumReady(mediumD, outD);
			};
			if( FD_ISSET( inD, &set ) ) {
				finished |= KbReady(inD, outD, termNum, mediumD);
			};
		}					
	} 	while(!finished);
	return;
}
