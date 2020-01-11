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
// Name        : ReceiverX.cpp
// Copyright   : Copyright 2019, Craig Scratchley
// Description : Starting point for ENSC 351 Project Part 5 
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <sys/select.h>
//#include <sys/dcmd_chr.h> // for DCMD_CHR_GETOBAND
#include <memory>
#include "Linemax.h"
#include "myIO.h"
#include "ReceiverX.h"
#include "ReceiverSS.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

// comment out the line below to get rid of Receiver logging information.
//#define REPORT_INFO

using namespace std;
using namespace Receiver_SS;

ReceiverX::
ReceiverX(int d, const char *fname, int conInD, int conOutD, bool useCrc)
:PeerX(d, fname, '(', ')', "/tmp/ReceiverSS.log", conInD, conOutD, useCrc),
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false), 
goodBlk1st(false), 
syncLoss(false), 
tmSoh(0),
numLastGoodBlk(0),
firstBlock(true)
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	transferringFileD = myCreat(fileName, mode);
	if(transferringFileD == -1) {
		CERR << strerror(errno) << ".  Error opening output file named: " << fileName << endl;
		can8();
		result = "OpenError";
	}
	else {
		auto myReceiverSmSp(make_shared<ReceiverSS>(this, false)); // or use make_unique perhaps
#ifdef REPORT_INFO
		transferCommon(myReceiverSmSp, true);
#else
		transferCommon(myReceiverSmSp, false);
#endif
		COUT << "\n"; // insert new line.
	}
}

// Update the below documentation?
/* Only called after an SOH character has been received and
posted to the Receiver_SS statechart. The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
 The function will set or reset a Boolean variable,
goodBlk. This variable will be made false if either
	� the needed number of bytes have not yet been received and another
	byte does not arrive within the character timeout since the last byte
	(or within the character timeout of the function being called).
	� the needed number of bytes (or more) are received and the block
	created using the needed number of bytes has something
	wrong with it, like the checksum being incorrect.
	� more than the needed number of bytes are received.
The function will also set or reset another Boolean variable,
syncLoss. syncLoss will only be set to true when there is a
fatal loss of syncronization as described in the XMODEM
specification. If goodBlk has not already been made false
and if syncLoss is false, then goodBlk will be set to true.  The
first time each block is received and is good, goodBlk1st will be
set to true.  This is an indication of when a block should be
written to disk.  If goodBlk is false and at
least the needed number of bytes were received in the function,
then a purge() function should be called before returning from
getRestBlk(). The purge() subroutine will read and discard
characters until nothing is received over a character timeout period.
*/
void ReceiverX::getRestBlk()
{ 
	const int restBlkSz = Crcflg ? REST_BLK_SZ_CRC : REST_BLK_SZ_CS;
    // here, we can read about 30 more characters than we hope to get,
    //         but keep min at restBlkSz, so any extra
    //         characters that happen to come from the serial port
    //         can be grabbed while we are calling myReadcond.
    int bytesRead = PE(myReadcond( mediumD, rcvBlk+1, BUF_SZ - 1, restBlkSz, dSECS_PER_UNIT*TM_CHAR, dSECS_PER_UNIT*TM_CHAR));
    if( bytesRead < restBlkSz) {
    	COUT << "(Sh" << bytesRead << ")" << flush;
    	goodBlk = goodBlk1st = false; // short block
    	// return;
    }
    else { // not needed if we put return in above.
    	const char* badReason;
   	 	if( bytesRead > restBlkSz) { // got an extra byte or two -- maybe there are more
			goodBlk = false; //things are fishy -- let's not take chances
			badReason = "be"; // "bad -- extra (bytes)"
		}
		else if (rcvBlk[2] != (255 - rcvBlk[1])) {
			//  block # and its complement are not matched
			goodBlk = false;
			badReason = "bm"; // "bad -- (complement not) matched"
		}
		else {
			goodBlk1st = (rcvBlk[1] == (uint8_t) (numLastGoodBlk + 1)); // but might be made false below
			if (!goodBlk1st) {
				// determine fatal loss of synchronization
				if (firstBlock || (rcvBlk[1] != numLastGoodBlk)) {
					syncLoss = true;
					goodBlk = false;
					COUT << "(s" << (unsigned) rcvBlk[1] << ":" << (unsigned) numLastGoodBlk << ")" << flush;
					purge();
					return;
				}
	//#define ALLOW_DEEMED_GOOD
	#ifdef ALLOW_DEEMED_GOOD
				else { // (rcvBlk[1] == numLastGoodBlk)
					goodBlk = true; // "deemed" good block
					COUT << "(d" << (unsigned) rcvBlk[1] << ")" << flush;
					// purge(); // ??
					return;
				}
	#endif
			}
			badReason = "bd"; // "bad data (in chunk)"
			// detect if data error in chunk
			// consider receiving checksum/CRC after calculating local checksum/CRC
			if (Crcflg) {
				uint16_t CRCbytes;
				crc16ns(&CRCbytes, &rcvBlk[DATA_POS]);
				goodBlk = (*((uint16_t*) &rcvBlk[PAST_CHUNK]) == CRCbytes);
			}
			else {
				uint8_t sum;
				checksum(&sum, rcvBlk);
				goodBlk = (rcvBlk[PAST_CHUNK] == sum);
			}
		}
		if (!goodBlk) {
			goodBlk1st = false; // but the block was "bad".
			COUT << "(" << badReason << (unsigned) rcvBlk[1] << ")" << flush;
			purge();  // discard chars until line idles for the character timeout period.
			return;
		}
#ifndef ALLOW_DEEMED_GOOD
		else if (!goodBlk1st) {
			COUT << "(r" << (unsigned) rcvBlk[1] << ")" << flush; // "resent" good block
			return;
		}
#endif
		// good block for the "first" time.
		numLastGoodBlk = rcvBlk[1];
		firstBlock = false; // if that was "first" block, next blocks won't be
		COUT << "(f" << (unsigned) rcvBlk[1] << ")" << endl;
	}
}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(myWrite(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

int
ReceiverX::
closeTransferredFile()
{
	return(myClose(transferringFileD));
}

//Send CAN_LEN of CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space in time CAN chars coming from receiver
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

/*
Read and discard contiguous CAN characters. Read
characters one-by-one in a loop until either nothing is
received over a "2CHAR" timeout period period or a character other than
CAN is received. If received, send a non-CAN character to
the console.
*/
void ReceiverX::clearCan()
{
	char character=CAN;
	//******** fill this function **************

	int clearCanAmount = 1;
	while( clearCanAmount == 1 && character == CAN)
	{
	clearCanAmount = myReadcond(mediumD, &character, 1, 0, dSECS_PER_UNIT*TM_2CHAR, dSECS_PER_UNIT*TM_2CHAR);

	}
	if (character != CAN)
		CON_OUT(consoleInId, character);
}

//The purge() subroutine will read and discard
//characters until nothing is received over a 1-second period.
void ReceiverX::purge()
{
	// ********** you will need to fill in this function ***********

    int purgedAmount = 1;
    char buffer;
    while(purgedAmount == 1 )
    {
        purgedAmount = myReadcond(mediumD,&buffer,1,0,dSECS_PER_UNIT*TM_CHAR,dSECS_PER_UNIT*TM_CHAR);
    }
}
