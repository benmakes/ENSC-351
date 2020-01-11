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
// File Name   : PeerX.cpp
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

//#include <arpa/inet.h> // for htons() -- not available with MinGW
#include <stdio.h> // for fprintf()
#include <string.h> // for strerror()
#include <stdlib.h>	// for exit()
#include <errno.h>
#include <iostream>

#include "PeerX.h"
#include "myIO.h"

void ErrorPrinter (const char* functionCall, const char* file, int line, int error)
{
	fprintf (stdout/*stderr*/, " \n!!! Error %d (%s) occurred at line %d of file %s\n"
			"\t resulted from invocation: %s\n"
			"\t Exiting program!\n",
			error, strerror(error), line, file, functionCall);
	fflush (stdout); // with MinGW the error doesn't show up sometimes.
	exit(EXIT_FAILURE);
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

// Should return a crc16 in 'network byte order'.
// Derived from code in "rbsb.c" (see above).
// Line comments in function below show lines removed from original code.
void
PeerX::
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
		  * is sent before the LSB
		  */
		 //sendline((int)oldcrc>>8);
		 //sendline((int)oldcrc);

		 /* in our case, we want the bytes to be in the memory pointed to by crc16nsP
		  * in the correct 'network byte order'
		  */

		 // ********* The next line needs to be changed ***********
		 *crc16nsP = oldcrc;
	 //}
	 //else
		 //sendline(checksum);
}

PeerX::
PeerX(int d, const char *fname)
:result("ResultNotSet"), mediumD(d), fileName(fname), transferringFileD(-1), Crcflg(true)
{
}

//Send a byte to the remote peer across the medium
void
PeerX::
sendByte(uint8_t byte)
{
	switch (int retVal = myWrite(mediumD, &byte, sizeof(byte))) {
		case 1:
			return;
		case -1:
			ErrorPrinter("myWrite(mediumD, &byte, sizeof(byte))", __FILE__, __LINE__, errno);
			break;
		default:
			std::cout /* cerr */ << "Wrong number of bytes written: " << retVal << std::endl;
			exit(EXIT_FAILURE);
	}
}

