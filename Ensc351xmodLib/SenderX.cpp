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
// File Name   : SenderX.cpp
// Copyright   : Copyright 2019, Craig Scratchley
// Description : Starting point for ENSC 351 Project Part5
//============================================================================

#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <fcntl.h>	// for O_RDONLY
#include <memory>
#include <thread>
#include <chrono>

#include "AtomicCOUT.h"
#include "myIO.h"
#include "SenderX.h"
#include "VNPE.h"
#include "SenderSS.h"

// comment out the line below to get rid of Sender logging information.
//#define REPORT_INFO

using namespace std;
using namespace Sender_SS;

SenderX::SenderX(const char *fname, int d, int conInD, int conOutD)
:PeerX(d, fname, '[', ']', "/tmp/SenderSS.log", conInD, conOutD),
 bytesRd(-2), // initialize with unique value.
 firstCrcBlk(true),
 blkNum(0)  	// but first block sent will be block #1, not #0
{
}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDONLY, 0);
	if(transferringFileD == -1) {
		COUT /* CERR */ << strerror(errno) << ".  Error opening input file named: " << fileName << endl;
		can8();
		result = "OpenError";
	}
	else {
		auto mySenderSmSp(make_shared<SenderSS>(this, false)); // or use make_unique perhaps
#ifdef REPORT_INFO
		transferCommon(mySenderSmSp, true);
#else
		transferCommon(mySenderSmSp, false);
#endif
	}
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.

//uint8_t SenderX::sendMostBlk(blkT blkBuf)
uint8_t SenderX::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	const int mostBlockSize = (this->Crcflg ? BLK_SZ_CRC : BLK_SZ_CS) - 1;
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte of a block to the receiver
void
SenderX::
sendLastByte(uint8_t lastByte)
{
	PE(myTcdrain(mediumD)); // wait for previous part of block to be completely drained from the descriptor
	dumpGlitches();			// dump any received glitches

	PE_NOT(myWrite(mediumD, &lastByte, sizeof(lastByte)), sizeof(lastByte));
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
//void SenderX::genBlk(blkT blkBuf)
void SenderX::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(myRead(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if (bytesRd>0) {
		blkBuf[0] = SOH; // can be pre-initialized for efficiency
		//block number and its complement
		uint8_t nextBlkNum = blkNum + 1;
		blkBuf[SOH_OH] = nextBlkNum;
		blkBuf[SOH_OH + 1] = ~nextBlkNum;
		if (this->Crcflg) {
			/*add padding*/
			if(bytesRd<CHUNK_SZ)
			{
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
			}

			/* calculate and add CRC in network byte order */
			crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
		}
		else {
			//checksum
			blkBuf[PAST_CHUNK] = blkBuf[DATA_POS];
			for( int ii=DATA_POS + 1; ii < DATA_POS+bytesRd; ii++ )
				blkBuf[PAST_CHUNK] += blkBuf[ii];

			//padding
			if( bytesRd < CHUNK_SZ )  { // this line could be deleted
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
				blkBuf[PAST_CHUNK] += CTRL_Z * padSize;
			}
		}
	}
}

/* tries to prepare the first block.
*/
void SenderX::prep1stBlk()
{
	genBlk(blkBufs[1]);
}

/* refit the 1st block with a checksum
*/
void
SenderX::
cs1stBlk()
{
	// maybe better to use something derived from variable blkNum instead of 1 below
	checksum (&blkBufs[1][PAST_CHUNK], blkBufs[1]);
}

/* while sending the now current block for the first time, prepare the next block if possible.
*/
void SenderX::sendBlkPrepNext()
{
	blkNum ++; // 1st block about to be sent or previous block was ACK'd
#ifdef REPORT_INFO
	// block will be "w"ritten to mediumD
	COUT << "\n[w" << (int)blkNum << "]" << flush;
#endif
	uint8_t lastByte = sendMostBlk(blkBufs[blkNum%2]);
	genBlk(blkBufs[(blkNum+1)%2]); // prepare next block
	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the xmodem receiver
void SenderX::resendBlk()
{
	// resend the block including the checksum or crc16
#ifdef REPORT_INFO
	// block will be "r"ewritten
	COUT << "[r" << (int)(blkNum) << "]" << flush;
#endif
	sendLastByte(sendMostBlk(blkBufs[blkNum%2]));
}

//Send CAN_LEN copies of CAN characters in a row (in pairs spaced in time) to the
//  XMODEM receiver, to inform it of the cancelling of a file transfer.
//  There should be a total of (canPairs - 1) delays of
//  ((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT) milliseconds
//  between the pairs of CAN characters.
void SenderX::can8()
{
	const int canBurst=2;
    char buffer[canBurst];
    memset( buffer, CAN, canBurst);

	const int canPairs=CAN_LEN/canBurst; 
	int x = 1;
	while (PE_NOT(myWrite(mediumD, buffer, canBurst), canBurst),
			x<canPairs) {
		++x;
	    this_thread::sleep_for (chrono::milliseconds((int)((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT)));
	}
}

/*
Read and discard contiguous CAN characters. Read characters
from the medium one-by-one in a loop until either nothing is
received over a 1-second period or a character other than
CAN is received. If received, send a non-CAN character to
the console.
*/
void SenderX::clearCan()
{
	char character=CAN;
	//******** fill this function in **************
	int clearCanAmount = 1;
	while( clearCanAmount == 1 && character == CAN)
	{
	    clearCanAmount = myReadcond(mediumD, &character, 1, 0, dSECS_PER_UNIT*TM_CHAR, dSECS_PER_UNIT*TM_CHAR);
	}
	if (character != CAN)
		CON_OUT(consoleInId, character << flush);
}


