//============================================================================
//
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
// Helpers: Spencer Pauls
//          Nitish Mallavarapu
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
//% * Your group name should be "P1_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : SenderX.cc
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <iostream>
#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR

#include "myIO.h"
#include "SenderX.h"

using namespace std;

SenderX::
SenderX(const char *fname, int d)
:PeerX(d, fname), bytesRd(-1), blkNum(255)
{
}

//-----------------------------------------------------------------------------

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block was
prepared or if the input file is empty (i.e. has 0 length).
*/
void SenderX::genBlk(blkT blkBuf)
{
	// ********* The next line needs to be changed *********** the chunk starts at index 3
	if (-1 == (bytesRd = myRead(transferringFileD, &blkBuf[3], CHUNK_SZ )))
		ErrorPrinter("myRead(transferringFileD, &blkBuf[0], CHUNK_SZ )", __FILE__, __LINE__, errno);
	if(bytesRd>=1 && bytesRd<128){
	    for(int i = bytesRd; i<128; i++){
	        blkBuf[3+i] = CTRL_Z;
	    }
	}
	// ********* and additional code must be written *********** at end of file myRead POSIX returns 0




    blkBuf[0]= SOH;
    if(blkNum >= 255)
        blkNum = 0;
    blkBuf[1] = blkNum;
    //blkNum++; increment already set

    blkBuf[2] = 255 - blkNum;

    // ********* The next couple lines need to be changed ***********
	if(this->Crcflg){
		uint16_t myCrc16ns;
		this->crc16ns(&myCrc16ns, &blkBuf[3]);
		blkBuf[131] = myCrc16ns >> 8; //erase the first 8 bits
		blkBuf[132] = myCrc16ns & 0xFF; //just take the first 8 bits
	}
	else{
	    int checksum = 0;
	    for(int i=3;i<131;i++) //changed from 131 to bytesRd
	    {
	        checksum += blkBuf[i];
	    }
	    blkBuf[131] = checksum % 256;
	}
}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDWR, 0);
	if(transferringFileD == -1) {
		// ********* fill in some code here to write 2 CAN characters ***********
		cout /* cerr */ << "Error opening input file named: " << fileName << endl;
		result = "OpenError";
		sendByte(CAN);
		sendByte(CAN);
	}
	else {
		cout << "Sender will send " << fileName << endl;

        // ********* re-initialize blkNum as you like ***********
        blkNum = 1; // but first block sent will be block #1, not #0

		// do the protocol, and simulate a receiver that positively acknowledges every
		//	block that it receives.

		// assume 'C' or NAK received from receiver to enable sending with CRC or checksum, respectively
		genBlk(blkBuf); // prepare 1st block
		while (bytesRd)
		{
			blkNum ++; // 1st block about to be sent or previous block was ACK'd

			// ********* fill in some code here to write a block ***********
			if(this->Crcflg)
			    {myWrite(mediumD, &blkBuf, 133);}
			else if (this->Crcflg == 0)
			{myWrite(mediumD, &blkBuf, 132);}
			/*for(int i=0; i<133; i++){
			    sendByte(blkBuf[i]); our first attempt*/

			// assume sent block will be ACK'd
			genBlk(blkBuf); // prepare next block
			// assume sent block was ACK'd
		};
		// finish up the protocol, assuming the receiver behaves normally
		// ********* fill in some code here ***********
	    sendByte(EOT);
	    sendByte(EOT);
		//(myClose(transferringFileD));
		if (-1 == myClose(transferringFileD))
			ErrorPrinter("myClose(transferringFileD)", __FILE__, __LINE__, errno);
		result = "Done";
	}
}

