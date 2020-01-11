//% Student Name 1: Benjamin Martin
//% Student 1 #: 3********
//% Student 1 userid (email): bpmartin (at) sfu.ca (stu1@sfu.ca)
//
//% Student Name 2: Yogesh Mundhra
//% Student 2 #: 3********
//% Student 2 userid (email): ymundhra (at) sfu.ca (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
// Spencer Pauls, Nitish Mallavarapu, Zavier!!!!!!
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P5_<userid1>_<userid2>" (eg. P5_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// Name        : PeerX.cpp
// Version     : November
// Copyright   : Copyright 2019, Craig Scratchley
// Description :  ENSC 351 Project 
//============================================================================

//#include <arpa/inet.h>  // for htons() -- not available with MinGW
#include "PeerX.h"
//#include <fstream>
#include <sys/time.h>
#include <string.h>
#include "VNPE.h"
#include "Linemax.h"
#include "myIO.h"
#include "AtomicCOUT.h"

using namespace std;

uint16_t my_htons(uint16_t n)
{
    unsigned char *np = (unsigned char *)&n;

    return
        ((uint16_t)np[0] << 8) |
        (uint16_t)np[1];
}

/* update CRC */
/*
The following XMODEM crc routine is taken from "rbsb.c".  Please refer to
    the source code for these programs (contained in RZSZ.ZOO) for usage.
As found in Chapter 8 of the document "ymodem.txt".
    Original 1/13/85 by John Byrns
    */

/*
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

unsigned short
updcrc(register int c, register unsigned crc)
{
	register int count;

	for (count=8; --count>=0;) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
			crc ^= 0x1021;
		}
		else {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
		}
	}
	return crc;
}

// Should return via crc16nsP a crc16 in 'network byte order'.
// Derived from code in "rbsb.c" (see above).
// Line comments in function below show lines removed from original code.
void
crc16ns (uint16_t* crc16nsP, uint8_t* buf)
{
	 register int wcj;
	 register uint8_t *cp;
	 unsigned oldcrc=0;
	 for (wcj=CHUNK_SZ,cp=buf; --wcj>=0; ) {
		 //sendline(*cp);

		 /* note the octal number in the line below */
		 oldcrc=updcrc((0377& *cp++), oldcrc);

		 //checksum += *cp++;
	 }
	 //if (Crcflg) {
		 oldcrc=updcrc(0,updcrc(0,oldcrc));
		 /* at this point, the CRC16 is in oldcrc */

		 /* This is where rbsb.c "wrote" the CRC16.  Note how the MSB
		  * is sent before the LSB.
		  * sendline is a function to "send a byte over a telephone line"
		  */
		 //sendline((int)oldcrc>>8);
		 //sendline((int)oldcrc);

		 /* in our case, we want the bytes to be in the memory pointed to by crc16nsP
		  * in the correct 'network byte order'
		  */
		 *crc16nsP = my_htons((uint16_t) oldcrc);
		 /* *crc16nsP = htons((uint16_t) oldcrc); */

	 //}
	 //else
		 //sendline(checksum);
}

void
checksum(uint8_t* sumP, blkT blkBuf)
{
	*sumP = blkBuf[DATA_POS];
	for( int ii=DATA_POS + 1; ii<PAST_CHUNK; ii++ )
		*sumP += blkBuf[ii];
}

PeerX::
PeerX(int d, const char *fname, char left, char right, const char *smLogN, int conInD, int conOutD, bool useCrc)
:result("ResultNotSet"),
 errCnt(0),
 Crcflg(useCrc),
 KbCan(false),
 mediumD(d),
 fileName(fname),
 transferringFileD(-1), 
 logLeft(left),
 logRight(right),
 smLogName(smLogN),
 consoleInId(conInD),
 consoleOutId(conOutD),
 reportInfo(false),
 absoluteTimeout(0),
 holdTimeout(0)
{
	struct timeval tvNow;
	PE(gettimeofday(&tvNow, NULL));
	sec_start = tvNow.tv_sec;
}

void
PeerX::
transferCommon(std::shared_ptr<StateMgr> mySM, bool reportInfoParam)
{
	reportInfo = reportInfoParam;
	// use this code to send stateChart logging information to a file.
	/*ofstream smLogFile; // need '#include <fstream>' above
	smLogFile.open(smLogName, ios::binary|ios::trunc);
	if(!smLogFile.is_open()) {
		CERR << "Error opening sender state chart log file named: " << smLogName << endl;
		exit(EXIT_FAILURE);
	}
	mySM->setDebugLog(&smLogFile);
	// */

	// comment out the line below if you want to see logging information which will,
	//	by default, go to cout.
	mySM->setDebugLog(nullptr); // this will affect both peers.  Is this okay?

	mySM->start();

	/* ******** You may need to add code here ******** */
	fd_set set;
	FD_ZERO(&set);
	struct timeval tv;
	struct timeval tv0;
	tv0.tv_sec = 0;
	tv0.tv_usec = 0;

	while(mySM->isRunning()) {
		// ************* this loop is going to need more work ************
		tv.tv_sec=0;
		uint32_t now = elapsed_usecs();
		FD_SET(mediumD, &set);
		FD_SET(consoleInId, &set);
		if (now >= absoluteTimeout) {
		    //tv.tv_sec=0;
		    int rv = PE(select( max(mediumD,consoleInId)+1, &set, NULL, NULL, &tv0));
		    //FD_SET(mediumD, &set);
		    //FD_SET(consoleInId, &set);
		    if(rv == 0)
		    {
		        mySM->postEvent(TM);
		    }
		    else
		    {
                if( FD_ISSET( consoleInId, &set ) )
                {
                    char temp[LINEMAX] = "";
                    int charsRead = PE(myRead(consoleInId, &temp, size_t(LINEMAX)));

                    char temp2[charsRead+1];

                    for (int i = 0; i < charsRead;i++)
                    {
                        temp2[i] = temp[i];
                    }

                    temp2[charsRead] = '\0';

                    if(strcmp(temp2, CANC_C)==0)
                    {
                        mySM->postEvent(KB_C);
                    }

                    //check to see if the keyboard input is the same as KB_C
                };
		        if( FD_ISSET( mediumD, &set ) )
		        {
		                        //route message from medium back to screen
	                //read character from medium
	                char byteToReceive;
	                PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
	                if (reportInfo)
	                    COUT << logLeft << 1.0*(absoluteTimeout - now)/MILLION << ":" << (int)(unsigned char) byteToReceive << ":" << byteToReceive << logRight << flush;
	                mySM->postEvent(SER, byteToReceive);
                };
		    }

		} else {
			// ...
			/****/ {
			    //FD_SET(mediumD, &set);
			    //FD_SET(consoleInId, &set);
			    tv.tv_usec = (absoluteTimeout - now);
			    int rv = PE(select( max(mediumD,consoleInId)+1, &set, NULL, NULL, &tv));

			    if(rv == 0)
	            {
	                mySM->postEvent(TM);
	            }

	            else
	                if( FD_ISSET( consoleInId, &set ) )
	                {
	                    char temp[LINEMAX] = "";
	                    int charsRead = PE(myRead(consoleInId, &temp, size_t(LINEMAX)));

	                    char temp2[charsRead+1];

	                    for (int i = 0; i < charsRead;i++)
	                    {
	                        temp2[i] = temp[i];
	                    }

	                    temp2[charsRead] = '\0';

	                    if(strcmp(temp2, CANC_C) == 0)
	                    {
	                        mySM->postEvent(KB_C);
	                    }

	                    //check to see if the keyboard input is the same as KB_C
	                };
	                if( FD_ISSET( mediumD, &set ) )
	                {
	                                //route message from medium back to screen
	                    //read character from medium
	                    char byteToReceive;
	                    PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
	                    if (reportInfo)
	                        COUT << logLeft << 1.0*(absoluteTimeout - now)/MILLION << ":" << (int)(unsigned char) byteToReceive << ":" << byteToReceive << logRight << flush;
	                    mySM->postEvent(SER, byteToReceive);
	                };
			}
		}
	}
	PE(close(transferringFileD));
//		smLogFile.close();
}

//Send a byte to the remote peer across the medium
void
PeerX::
sendByte(uint8_t byte)
{
	if (reportInfo) {
		COUT << logLeft << byte << logRight << flush;
	}
	PE_NOT(myWrite(mediumD, &byte, sizeof(byte)), sizeof(byte));
}

// get rid of any characters that may have arrived from the medium.
void
PeerX::
dumpGlitches()
{
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
}

// returns microseconds elapsed since this peer was constructed (within 1 second)
uint32_t PeerX::elapsed_usecs()
{
	struct timeval tvNow;
	PE(gettimeofday(&tvNow, NULL));
	/*_CSTD */ time_t	tv_sec = tvNow.tv_sec;
	return (tv_sec - sec_start) * MILLION + tvNow.tv_usec; // casting needed?
}

/*
set a timeout time at an absolute time timeoutUnits into
the future. That is, determine an absolute time to be used
for the next one or more XMODEM timeouts by adding
timeoutUnits to the elapsed time.
*/
void PeerX::tm(int timeoutUnits)
{
	absoluteTimeout = elapsed_usecs() + timeoutUnits * uSECS_PER_UNIT;
}

/* make the absolute timeout earlier by reductionUnits */
void PeerX::tmRed(int unitsToReduce)
{
	absoluteTimeout -= (unitsToReduce * uSECS_PER_UNIT);
}

/*
Store the current absolute timeout, and create a temporary
absolute timeout timeoutUnits into the future.
*/
void PeerX::tmPush(int timeoutUnits)
{
	holdTimeout = absoluteTimeout;
	absoluteTimeout = elapsed_usecs() + timeoutUnits * uSECS_PER_UNIT;
}

/*
Discard the temporary absolute timeout and revert to the
stored absolute timeout
*/
void PeerX::tmPop()
{
	absoluteTimeout = holdTimeout;
}

