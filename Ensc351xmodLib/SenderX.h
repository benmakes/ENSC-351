#ifndef SENDER_H
#define SENDER_H

#include <unistd.h> // for ssize_t ?
#include <stdint.h> // uint8_t
#include "PeerX.h"

class SenderX : public PeerX
{
public:
	SenderX(const char *fname, int d, int conInId, int conOutD);
	
	void sendFile();

	void resendBlk();
	void prep1stBlk();
	void cs1stBlk(); /* refit the 1st block with a checksum */
	void sendBlkPrepNext();
	void can8();
	void clearCan();

	ssize_t bytesRd; // The number of bytes last read from the input file.

	// firstCrcBlk should be true after getting a 'C' for an NCGbyte and until the first block gets an ACK
	bool firstCrcBlk;
	
private:
	blkT blkBufs[2];	// Array of two blocks
	uint8_t blkNum;		// number of current block to be acknowledged

//	void genBlk(blkT blkBuf);
	void genBlk(uint8_t blkBuf[BLK_SZ_CRC])
	;

	// Send the block, less the block's last byte, to the receiver
//	uint8_t sendMostBlk(blkT blkBuf);
	uint8_t sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
	;

	// Send the last byte of a block to the receiver
	void
	sendLastByte(uint8_t lastByte)
	;
};

#endif
