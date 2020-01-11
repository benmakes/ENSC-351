// Solution to ENSC 351 Assignment 3B.  Prepared by:
//      - Craig Scratchley, Simon Fraser University
//      - Zhenwang Yao

#include <thread>
#include <iostream>
#include <sys/socket.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "myIO.h"
#include "Linemax.h"
#include "Medium.h"
#include "terminal.h"
#include "Kvm.h"
#include "VNPE.h"
#include "SocketReadcond.h"

using namespace std;

static int fdaSktPrTermMed[2][2];	//Socket Pairs between terminals and Medium
static int fdaSktPrTermKvm[2][2];	//  "      " between terminals and kvm

enum  {TERM_SIDE, OTHER_SIDE};

//kvm thread, handles all keyboard input and routes it to the selected terminal
void kvmFunc() {
	int d[2];
	d[TERM2] = fdaSktPrTermKvm[TERM2][OTHER_SIDE];
	d[TERM1] = fdaSktPrTermKvm[TERM1][OTHER_SIDE];

	Kvm(d);
	PE(myClose(d[TERM1]));
	PE(myClose(d[TERM2]));
	return;
}

//terminal thread
//at least 2 terminal threads are required for a file transfer on a single computer
void termFunc(int termNum)
{
	int mediumD, inD, outD;
	mediumD = fdaSktPrTermMed[termNum][TERM_SIDE];
	//mediumD = open("/dev/ser2", O_RDWR);
	inD = outD = fdaSktPrTermKvm[termNum][TERM_SIDE];
//		cout << "Term " << termNum << " THREAD STARTED" << endl;

	Terminal(termNum + 1, inD, outD, mediumD);
	PE(myClose(mediumD));
}

void mediumFunc(void)
{
	Medium medium(fdaSktPrTermMed[TERM1][OTHER_SIDE],fdaSktPrTermMed[TERM2][OTHER_SIDE], "/tmp/xmodemData.dat");
	medium.start();
}

int main()
{
	cout.precision(2);
	// PE_0(pthread_setname_np(pthread_self(), "P-KVM")); // give the primary thread (does kvm) a name

	// lower the priority of the primary thread to 4
//	PE_EOK(pthread_setschedprio(pthread_self(), 4));

	//Create and wire socket pairs
	// creating socket pair between terminal1 and Medium
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, fdaSktPrTermMed[TERM1]));
	
	// creating socket pair between terminal2 and Medium
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, fdaSktPrTermMed[TERM2]));
	
	// opening kvm-term2 socket pair
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, fdaSktPrTermKvm[TERM2])); 
	
	// opening kvm-term1 socket pair
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, fdaSktPrTermKvm[TERM1]));

	//Create 3 threads

	// create thread for medium, make its priority 5 levels higher (i.e. priority 9)
//	pthread_t mediumThrd;
//	PE_EOK(wcs_thread_create(&mediumThrd, mediumFunc, NULL, 9, "M" ));

	// ***** create thread for medium *****
	thread mediumThrd(mediumFunc);

	thread term1Thrd(termFunc, TERM1);
	thread term2Thrd(termFunc, TERM2);
	
//	pthread_t term1Thrd, term2Thrd;
//	PE_EOK(wcs_thread_create(&term1Thrd, termFunc, (void *)TERM1, 4, "T1" ));
//	PE_EOK(wcs_thread_create(&term2Thrd, termFunc, (void *)TERM2, 4, "T2" ));

//	PE_0(pthread_setname_np(pthread_self(), "P-K"));
	kvmFunc();
	
	term2Thrd.join();
	term1Thrd.join();
	// ***** join with thread for medium *****
	mediumThrd.join();

//	// do joining with threads here.
//	PE_EOK(pthread_join(term1Thrd, NULL));
//	PE_EOK(pthread_join(term2Thrd, NULL));
//	PE_EOK(pthread_join(mediumThrd, NULL));
	
	return EXIT_SUCCESS;
}
