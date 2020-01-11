/*
 * Medium.cpp
 *
 *      Author: Craig Scratchley
 *      Copyright(c) 2014 (Spring) Craig Scratchley
 */

#include <fcntl.h>
#include <unistd.h> // for write()
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include "Medium.h"
#include "myIO.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

#include "PeerX.h"

// Uncomment the line below to turn on debugging output from the medium
//#define REPORT_INFO

//This medium contains both kind and non-kind versions.
//	See Medium.h

#ifndef USE_PART2A_S2_TO_R1
#define T2toT1_CORRUPT_FIRST_BYTE	264
#define T2toT1_CORRUPT_BYTE			790	//current algorithm only works when
										// T2toT1_CORRUPT_BYTE is greater than bufSz. Also applies to T2toT1_DROP_BYTE
#define T2toT1_DROP_FIRST_BYTE		659
#define T2toT1_DROP_BYTE			790

#define T2toT1_GLITCH_BYTES			25
#endif

#ifndef USE_PART2A_R1_TO_S2
#define T1toT2_CORRUPT_BYTE		6 //4 //6
#define T1toT2_DROP_BYTE		7 //6 //7
#define T1toT2_GLITCH_BYTE		3 //2 //3
#define	T1toT2_SEND_GLITCH_ACK	50
#endif

using namespace std;

Medium::Medium(int d1, int d2, const char *fname)
:Term1D(d1), Term2D(d2), logFileName(fname)
{
#ifndef USE_PART2A_S2_TO_R1
	fromT2ByteCount = 0;
	corruptThreshold = T2toT1_CORRUPT_FIRST_BYTE;
	dropThreshold = T2toT1_DROP_FIRST_BYTE;
	fromT2Glitch = false;
#else
	byteCountPlus1 = 1;
#endif

#ifndef USE_PART2A_R1_TO_S2
	glitchCount = 0;
	fromT1ByteCount = 0;
	sentCount = 0;
#else
	ACKforwarded = 0;
	ACKreceived = 0;
#endif
	sendExtraAck = false;
	logFileD = -1;
	// mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	//logFileD = PE2(creat(logFileName, mode), logFileName);
}

Medium::~Medium() {
}

bool Medium::MsgFromTerm2()
{
#ifndef USE_PART2A_S2_TO_R1
	const int mediumBufSz = 160; // allow for enough glitch bytes
	uint8_t bytesReceived[mediumBufSz];
	memset(bytesReceived, 'g', mediumBufSz); // initialize buffer, so glitches will be deterministic
	int fromT2GlitchBytes;

	int numOfByteReceived = PE(myRead(Term2D, bytesReceived, 70));
	if (numOfByteReceived == 0) {
		COUT << "Medium thread: TERM2's socket closed, Medium terminating" << endl;
		return true;
	}
	fromT2ByteCount += numOfByteReceived;

	// deal with corrupt byte
	if( fromT2ByteCount >= corruptThreshold ) {
		int byteToCorrupt = numOfByteReceived-1-(fromT2ByteCount-corruptThreshold);
		bytesReceived[byteToCorrupt] = ~bytesReceived[byteToCorrupt];
#ifdef REPORT_INFO
		COUT << "<" << byteToCorrupt << "x" << fromT2ByteCount - (numOfByteReceived - 1 - byteToCorrupt) << ">" << flush;
#endif
		corruptThreshold += T2toT1_CORRUPT_BYTE;
		if (fromT2Glitch) {
			fromT2GlitchBytes = T2toT1_GLITCH_BYTES;
#ifdef REPORT_INFO
		COUT << "<+" << T2toT1_GLITCH_BYTES << ">" << flush;
#endif
		}
		else
			fromT2GlitchBytes = 0;
		fromT2Glitch = !fromT2Glitch;
	}

	//deal with drop byte, if only 1 byte is being sent, then the operation does not
	// matter, since 1 less byte will be sent in the end
	int drop = 0;
	if (fromT2ByteCount >= dropThreshold ) {
		int byteToDrop = numOfByteReceived-1-(fromT2ByteCount-dropThreshold);
		memmove( bytesReceived + sizeof(char)*byteToDrop,
			bytesReceived + sizeof(char)*(byteToDrop+1), (mediumBufSz-1-byteToDrop));
#ifdef REPORT_INFO
		COUT << "<" << byteToDrop << "-" << fromT2ByteCount - (numOfByteReceived - 1 - byteToDrop) << ">" << flush;
#endif
		dropThreshold += T2toT1_DROP_BYTE;

		//send 1 less byte
		drop = 1;
	}
	// sometimes inject glitches at this time on Term2D
	int fromT2bytesToWrite = numOfByteReceived - drop + fromT2GlitchBytes;

	PE_NOT(write(logFileD, bytesReceived, 1), 1);
	//Forward the bytes to RECEIVER,
	PE_NOT(myWrite(Term1D, bytesReceived, 1), 1);

	if (sendExtraAck) {
#ifdef REPORT_INFO
			COUT << "{" << "+A" << "}" << flush;
#endif
		uint8_t buffer = ACK;
		PE_NOT(write(logFileD, &buffer, 1), 1);
		//Forward the buffer to term2,
		PE_NOT(myWrite(Term2D, &buffer, 1), 1);

		sendExtraAck = false;
	}

	PE_NOT(write(logFileD, &bytesReceived[1], fromT2bytesToWrite-1), fromT2bytesToWrite-1);
	//Forward the bytes to Terminal 1,
	PE_NOT(myWrite(Term1D, &bytesReceived[1], fromT2bytesToWrite-1), fromT2bytesToWrite-1);

//	PE_NOT(write(logFile, bytesReceived, fromT2bytesToWrite), fromT2bytesToWrite);
//	PE_NOT(myWrite(Term1D, bytesReceived, fromT2bytesToWrite), fromT2bytesToWrite);

	return false;
#else // USE_PART2A_S2_TO_R1 is defined
	blkT bytesReceived;
	int numOfByteReceived;
	int byteToCorrupt;

	if (!(numOfByteReceived = PE(myRead(Term2D, bytesReceived, BLK_SZ_CRC)))) {
		COUT << "Medium thread: TERM2's socket closed, Medium terminating" << endl;
		return true;
	}

	byteCountPlus1 += numOfByteReceived;
	if (byteCountPlus1 >= 392) {
		byteCountPlus1 = byteCountPlus1 - 392;
		byteToCorrupt = numOfByteReceived - byteCountPlus1;
			bytesReceived[byteToCorrupt] = (255 - bytesReceived[byteToCorrupt]);
#ifdef REPORT_INFO
		COUT << "<" << byteToCorrupt << "x>" << flush;
#endif
	}

	PE_NOT(write(logFileD, bytesReceived, 1), 1);
	//Forward the bytes to RECEIVER,
	PE_NOT(myWrite(Term1D, bytesReceived, 1), 1);

	if (sendExtraAck & (numOfByteReceived >= 2)) {
#ifdef REPORT_INFO
			COUT << "{" << "+A" << "}" << flush;
#endif
		uint8_t buffer = ACK;
		PE_NOT(write(logFileD, &buffer, 1), 1);
		//Forward the buffer to term2,
		PE_NOT(myWrite(Term2D, &buffer, 1), 1);

		sendExtraAck = false;
	}

	PE_NOT(write(logFileD, &bytesReceived[1], numOfByteReceived-1), numOfByteReceived-1);
	//Forward the bytes to RECEIVER,
	PE_NOT(myWrite(Term1D, &bytesReceived[1], numOfByteReceived-1), numOfByteReceived-1);

	return false;
#endif
}

bool Medium::MsgFromTerm1()
{
#ifndef USE_PART2A_R1_TO_S2
	char byteReceived;

	int numOfByteReceived = PE(myRead(Term1D, &byteReceived, sizeof(byteReceived)));
	if (numOfByteReceived == 0) {
		COUT << "Medium thread: TERM1's socket closed, Medium terminating" << endl;
		return true;
	}
	fromT1ByteCount+= numOfByteReceived;

	if ( fromT1ByteCount % T1toT2_DROP_BYTE == 0 ) {
		// sends nothing
#ifdef REPORT_INFO
		COUT << "{" << (int)(unsigned char)byteReceived << "-" << fromT1ByteCount << "}" << flush;
#endif
	} else {
		sentCount++;
		if( sentCount % T1toT2_GLITCH_BYTE == 0 ) {
#ifdef REPORT_INFO
			COUT << "{" << sentCount << "+" << (int)glitchCount << "}" << flush;
#endif
			PE_NOT(write(logFileD,&glitchCount, sizeof(glitchCount)), sizeof(byteReceived));
			PE_NOT(myWrite(Term2D, &glitchCount, sizeof(glitchCount)), sizeof(byteReceived));
			glitchCount++;
		}
		if( fromT1ByteCount % T1toT2_CORRUPT_BYTE == 0 ) {
#ifdef REPORT_INFO
			COUT << "{" << (int)(unsigned char)byteReceived << "xN" << "}" << flush;
#endif
			byteReceived = NAK;
		}
		if( fromT1ByteCount % T1toT2_SEND_GLITCH_ACK == 0 ) {
			sendExtraAck = true;
		}
		// should we sometimes inject glitches at this time on Term1D?
		PE_NOT(write(logFileD, &byteReceived, sizeof(byteReceived)), sizeof(byteReceived));
		PE_NOT(myWrite(Term2D, &byteReceived, sizeof(byteReceived)), sizeof(byteReceived));
	}
	return false;
#else // USE_PART2A_R1_TO_S2 is defined
	uint8_t buffer[CAN_LEN];
	int numOfByte = PE(myRead(Term1D, buffer, CAN_LEN));
	if (numOfByte == 0) {
		COUT << "Medium thread: TERM1's socket closed, Medium terminating" << endl;
		return true;
	}

	/*note that we record the errors in ACK so that we can check in the log file*/
	if(buffer[0]==ACK)
	{
		ACKreceived++;

		if((ACKreceived%9)==0)
		{
			ACKreceived = 0;
			buffer[0]=NAK;
#ifdef REPORT_INFO
			COUT << "{" << "AxN" << "}" << flush;
#endif
		}
		else/*actually forwarded ACKs*/
		{
			ACKforwarded++;

			if((ACKforwarded%6)==0)/*Note that this extra ACK is not an ACK forwarded from receiver to the sender, so we don't increment ACKforwarded*/
			{
				ACKforwarded = 0;
				sendExtraAck = true;
			}
		}
	}

	PE_NOT(write(logFileD, buffer, numOfByte), numOfByte);

	//Forward the buffer to term2,
	PE_NOT(myWrite(Term2D, buffer, numOfByte), numOfByte);
	return false;
#endif
}

void Medium::start()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	logFileD = PE2(creat(logFileName, mode), logFileName);
	fd_set cset;
	FD_ZERO(&cset);

	bool finished=false;
	while(!finished) {

		//note that the file descriptor bitmap must be reset every time
		FD_SET(Term1D, &cset);
		FD_SET(Term2D, &cset);

		int rv = PE(select( max(Term2D,
							Term1D)+1, &cset, NULL, NULL, NULL ));
		if( rv == 0 ) {
			// timeout occurred
			CERR << "The medium should not timeout" << endl;
			exit (EXIT_FAILURE);
		} else {
			if( FD_ISSET( Term1D, &cset ) ) {
				finished = MsgFromTerm1(); //Term1D,Term2D);
			}
			if( FD_ISSET( Term2D, &cset ) ) {
				finished = MsgFromTerm2(); //Term1D,Term2D);
			}
		}
	};
	PE(close(logFileD));
	PE(myClose(Term1D));
	PE(myClose(Term2D));
}
