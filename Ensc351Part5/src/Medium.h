/*
 * Medium.h
 *
 *      Author: Craig Scratchley
 *      Copyright(c) 2013 (Fall) Craig Scratchley
 */

#ifndef MEDIUM_H_
#define MEDIUM_H_

//comment out "define USE_PART2A_R1_TO_S2"
// to use the final terminal 1->2 medium. It can drop chars, glitch, etc.
#define USE_PART2A_R1_TO_S2

//comment out "define USE_PART2A_S2_TO_R1"
// to use the final terminal 2->1 medium. It can drop chars, glitch, etc.
#define USE_PART2A_S2_TO_R1

class Medium {
public:
	Medium(int d1, int d2, const char *fname);
	virtual ~Medium();

	void start();

private:
	int Term1D;	// descriptor for Term1
	int Term2D;	// descriptor for Term2
	const char* logFileName;
	int logFileD;	// descriptor for log file

#ifndef USE_PART2A_S2_TO_R1
	unsigned int fromT2ByteCount;
	unsigned int corruptThreshold;
	unsigned int dropThreshold;
	bool fromT2Glitch;
#else
	int byteCountPlus1;
#endif

#ifndef USE_PART2A_R1_TO_S2
	unsigned char glitchCount;
	unsigned int fromT1ByteCount;
	unsigned int sentCount;
#else
	int ACKreceived;
	int ACKforwarded;
#endif
	bool sendExtraAck;

	bool MsgFromTerm1();
	bool MsgFromTerm2();
};

#endif /* MEDIUM_H_ */
