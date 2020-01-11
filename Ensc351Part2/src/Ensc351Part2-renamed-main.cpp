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
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// Version     : September, 2019
// Copyright   : Copyright 2019, Craig Scratchley
// Description : Starting point for ENSC 351 Project Part 2
//============================================================================

#include <stdlib.h> // EXIT_SUCCESS
#include <sys/socket.h>
#include <pthread.h>
#include <thread>
#include <chrono>         // std::chrono::
#include "myIO.h"
#include "SenderX.h"
#include "ReceiverX.h"
#include "Medium.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

using namespace std;

enum  {Term1, Term2};
enum  {TermSkt, MediumSkt};

static int daSktPr[2];	  //Socket Pair between term1 and term2
static int daSktPrT1M[2];	  //Socket Pair between term1 and medium
static int daSktPrMT2[2];	  //Socket Pair between medium and term2

void termFunc(int termNum)
{
	// ***** modify this function to communicate with the "Kind Medium" *****

	if (termNum == Term1) {
		const char *receiverFileName = "transferredFile";
		COUT << "Will try to receive to file:  " << receiverFileName << endl;
		ReceiverX xReceiver(daSktPrT1M[TermSkt], receiverFileName); //maybe array index should be termNum?
		xReceiver.receiveFile();
		COUT << "xReceiver result was: " << xReceiver.result << endl;
/*	    const char *receiverFileName = "transferredFile-cs";
	            COUT << "Will try to receive with Checksum to file:  " << receiverFileName << endl;
	            //ReceiverX xReceiver(daSktPr[Term1], receiverFileName, false);
	            ReceiverX xReceiverCS(daSktPrT1M[TermSkt], receiverFileName, false);
	            xReceiverCS.receiveFile();
	            COUT << "xReceiver result was: " << xReceiverCS.result << endl;

	            COUT << "Term1 waiting for 2 seconds" << endl;
	            std::this_thread::sleep_for (std::chrono::seconds(2));
	            COUT << endl;
	            std::this_thread::sleep_for (std::chrono::milliseconds(1));

	            receiverFileName = "transferredFile-crc16";
	            COUT << "Will try to receive with CRC16 to file:  " << receiverFileName << endl;
	            //ReceiverX xReceiver(daSktPr[Term1], receiverFileName, true);
	            ReceiverX xReceiver(daSktPrT1M[TermSkt], receiverFileName, true);
	            xReceiver.receiveFile();
	            COUT << "xReceiver result was: " << xReceiver.result << endl;*/
	}
	else {
		PE_0(pthread_setname_np(pthread_self(), "T2")); // give the thread (terminal 2) a name
		const char *senderFileName = "/etc/mailcap"; // for ubuntu target
		// const char *senderFileName = "/etc/printers/epijs.cfg"; // for QNX 6.5 target
		// const char *senderFileName = "/etc/system/sapphire/PasswordManager.tr"; // for BB Playbook target
		COUT << "Will try to send the file:  " << senderFileName << endl;
		SenderX xSender(senderFileName, daSktPrMT2[TermSkt]);
		xSender.sendFile();
		COUT << "xSender result was: " << xSender.result << endl;
		/*const char *senderFileName = "/etc/init.d/vmware-tools"; // for ubuntu target
		        COUT << "Will try to send the file:  " << senderFileName << endl;
		        //SenderX xSender(senderFileName, daSktPr[Term2]);
		        SenderX xSender(senderFileName, daSktPrMT2[TermSkt]);
		        xSender.sendFile();
		        COUT << "xSender result was: " << xSender.result << endl;

		        if (xSender.result != "UnexpectedC") {
		            std::this_thread::sleep_for (std::chrono::milliseconds(1));
		            COUT << endl;
		            std::this_thread::sleep_for (std::chrono::milliseconds(1));

		            COUT << "Will try to send the file:  " << senderFileName << endl;
		            //SenderX xSender(senderFileName, daSktPr[Term2]);
		            SenderX xSender2(senderFileName, daSktPrMT2[TermSkt]);
		            xSender2.sendFile();
		            COUT << "xSender result was: " << xSender2.result << endl;
		        }*/
	}
    std::this_thread::sleep_for (std::chrono::milliseconds(2));
    if(termNum == Term1)
    {
        PE(myClose(daSktPrT1M[TermSkt]));
    }
    else
    {
        PE(myClose(daSktPrMT2[TermSkt]));
    }
}

// ***** you will need this at some point *****
void mediumFunc(void)
{
	PE_0(pthread_setname_np(pthread_self(), "M")); // give the thread (medium) a name
	Medium medium(daSktPrT1M[MediumSkt],daSktPrMT2[MediumSkt], "xmodemData.dat");
	medium.run();
}


int myMain()
{
	// ***** Modify this function to create the "Kind Medium" thread and communicate with it *****

	PE_0(pthread_setname_np(pthread_self(), "P-T1")); // give the primary thread (terminal 1) a name

	// ***** switch from having one socketpair for direct connection to having two socketpairs
	//			for connection through medium thread *****
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrT1M));
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrMT2));
	//daSktPr[Term1] =  PE(/*myO*/open("/dev/ser2", O_RDWR));

	thread term2Thrd(termFunc, Term2);

	// ***** create thread for medium *****
	thread mediumThrd(mediumFunc);
	termFunc(Term1);

	term2Thrd.join();
	// ***** join with thread for medium *****
	mediumThrd.join();
	return EXIT_SUCCESS;
}
