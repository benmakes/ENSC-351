#ifndef SENDER_H
#define SENDER_H

#include <unistd.h>

#include "PeerX.h"

class SenderX : public PeerX
{
	friend
	void testSenderX(const char* iFileName, int mediumD)
	;

public:
	SenderX(const char *fname, int d);
	void sendFile();
	ssize_t bytesRd;  // The number of bytes last read from the input file.

private:
	uint8_t blkBuf[BLK_SZ_CRC];     // a  block
	// blkT blkBuf;    // A block // causes inability to debug this array.
	uint8_t blkNum;	// number of the current block to be acknowledged
	void genBlk(blkT blkBuf);
};

#endif
