/* Ensc351Part3-test-priorities.cpp -- November -- Copyright 2019 Craig Scratchley */
#include <sys/socket.h>
#include <stdlib.h>				// for exit()
#include <sched.h>
#include "posixThread.hpp"
#include "AtomicCOUT.h"
#include "VNPE.h"
#include "myIO.h"

/* This program can be used to test your changes to myIO.cpp
 *
 * Put this project in the same workspace as your Ensc351Part2SolnLib and Ensc351 library projects,
 * and build it.
 *
 * With two created threads for a total of 3 threads, the output that I get is:
 *

RetVal 1 in primary: 14 Ba: abcd123456789
RetVal in T42: 0
RetVal in T32: 0
RetVal 2 in primary: 4 Ba: xyz

 *
 */

using namespace std;
using namespace pthreadSupport;

static int daSktPr[2];	  // Descriptor Array for Socket Pair
cpu_set_t set;
int myCpu=0;

void threadT42Func(void)
{
    PE(sched_setaffinity(0, sizeof(set), &set)); // set processor affinity for current thread

	PE_NOT(myWrite(daSktPr[1], "ijkl", 5), 5);
	int RetVal = PE(myTcdrain(daSktPr[1])); // will block until myClose
	cout << "RetVal in T42: " << RetVal << endl << flush;
}

void threadT32Func(void) // starts at priority 60
{
    PE(sched_setaffinity(0, sizeof(set), &set)); // set processor affinity for current thread

    PE_NOT(myWrite(daSktPr[0], "abcd", 4), 4);
	PE(myTcdrain(daSktPr[0])); // will block until 1st myReadcond

	setSchedPrio(40);

	PE_NOT(myWrite(daSktPr[0], "123456789", 10), 10); // don't forget nul termination character

	PE(myWrite(daSktPr[0], "xyz", 4));
	int RetVal = PE(myClose(daSktPr[0]));

	cout << "RetVal in T32: " << RetVal << endl << flush;
}

void threadT41Func(void)
{
    PE(sched_setaffinity(0, sizeof(set), &set)); // set processor affinity for current thread

	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPr));
	posixThread threadT32(SCHED_FIFO, 60, threadT32Func);

	posixThread threadT42(SCHED_FIFO, 70, threadT42Func);

    char	Ba[200];
    int
	RetVal = PE(myReadcond(daSktPr[1], Ba, 200, 12, 0, 0));  // will block until myWrite of 10 characters
	cout << "RetVal 1 in primary: " << RetVal << " Ba: " << Ba << endl << flush;

    RetVal = PE(myReadcond(daSktPr[1], Ba, 200, 12, 0, 0)); // will block until myClose

	threadT32.join();
	threadT42.join(); // only needed if you created the thread above.
	cout << "RetVal 2 in primary: " << RetVal << " Ba: " << Ba << endl << flush;
}

int main() {
    CPU_SET(myCpu, &set);
    PE(sched_setaffinity(0, sizeof(set), &set)); // set processor affinity for current thread

	sched_param sch;
	sched_param sch2;
	try{
		int policy = -1;
		getSchedParam(&policy, &sch2);
		std::cout << "Primary Thread executing at priority " <<  sch2.sched_priority << '\n';
		std::cout << "Primary Thread executing at policy " <<  policy << '\n';
		sch.__sched_priority = 30;
		setSchedParam(SCHED_FIFO, sch); //SCHED_FIFO == 1, SCHED_RR == 2
		getSchedParam(&policy, &sch2);
		std::cout << "Primary Thread executing at priority(after 30) " <<  sch2.sched_priority << '\n';
		std::cout << "Primary Thread executing at policy(after 1) " <<  policy << '\n';
	}
	catch (std::system_error& error){
		std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
	}

	posixThread T41(SCHED_FIFO, 50, threadT41Func);
	T41.join();
	return 0;
}


