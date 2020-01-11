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
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//          Spencer Pauls
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"
#include <cstdlib>
//using namespace std;

ReceiverX::
ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc), 
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false),
goodBlk1st(false), 
syncLoss(true),
numLastGoodBlk(0),
recalcCS(0)
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	transferringFileD = PE2(myCreat(fileName, mode), fileName);

	// ***** improve this member function *****

	// below is just an example template.  You can follow a
	// 	different structure if you want.

	// inform sender that the receiver is ready and that the
	//		sender can send the first block
	sendByte(NCGbyte);

	while(PE_NOT(myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == SOH))
	{
		getRestBlk();
		if(syncLoss == true)
        { //cancel transfer, some sync error in receiving wrong block
                can8();
        }
		else if(goodBlk == false)
		{ //ask sender to resend byte
		    sendByte(NAK);
		}
		else if(goodBlk == true)
		{
		    sendByte(ACK);
		    if(goodBlk1st)
		    {
		        writeChunk();
		        numLastGoodBlk++;
		        goodBlk1st = false;
		    }
		}
	};
	if(rcvBlk[0] == EOT)
	{
	    sendByte(NAK); // NAK the first EOT
	    PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
	    (close(transferringFileD));
	    if(rcvBlk[0] == EOT)
	    {
	        // check if the file closed properly.  If not, result should be something other than "Done".
	        result = "Done"; //assume the file closed properly.
	        sendByte(ACK); // ACK the second EOT
	    }
	    else
	    {
	        std::cout << "Sender received totally unexpected char #"
	        << rcvBlk[0] << ":" << (char) rcvBlk[0] << std::endl;
	    }
	}
	else
	{
	    can8();
	    (close(transferringFileD));
	    result = "EOT not received"; // by us
	}
	 // presumably read in another EOT

}

/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{

    // ********* this function must be improved ***********
    if(Crcflg)
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
    else
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CS, REST_BLK_SZ_CS, 0, 0), REST_BLK_SZ_CS);
        uint8_t nextGoodBlk= numLastGoodBlk+1;
        if(rcvBlk[1]+rcvBlk[2]!=255)
        {
            goodBlk = false;
            goodBlk1st = false;
            syncLoss = false;
        }
        else
        {
            if((rcvBlk[1]!=nextGoodBlk) && (rcvBlk[1]!=numLastGoodBlk))
            {
                syncLoss = true;
            }
            else
            {
                syncLoss = false;
                if(Crcflg)
                {
                    crc16ns((uint16_t*)&recalcCRC[0], &rcvBlk[DATA_POS]);

                    if(recalcCRC[0]!=rcvBlk[131] || recalcCRC[1]!=rcvBlk[132])
                    {
//                        recalcCRC[0] = 0;
//                        recalcCRC[1] = 0;
                        goodBlk = goodBlk1st = false;
                    }
                    else
                    {
                        if(rcvBlk[1]==nextGoodBlk)
                        {
                            goodBlk1st = true;
                        }
                        goodBlk = true;
                    }
                }
                else if(!Crcflg)
                { //for checksum
                   for(int ii=DATA_POS; ii<DATA_POS+CHUNK_SZ; ii++)
                   {
                       recalcCS += rcvBlk[ii];
                   }
                      if(rcvBlk[PAST_CHUNK]!=recalcCS)
                      {
                          recalcCS = 0;
                          goodBlk = goodBlk1st = false;
                      }
                      else
                      {
                          recalcCS = 0;
                          if(rcvBlk[1]==nextGoodBlk)
                          {
                              goodBlk1st = true;
                          }
                          goodBlk = true;
                      }
                }
            }
        }
}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver in time
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}
