//============================================================================
// Name        : myIO.cpp
// Author(s)   : Craig Scratchley
//			   : with assistance from E. Kan
//			   :
// Version     : Oct., 2019 -- tcdrain for socketpairs.
// Copyright   : Copyright 2019, Engineering Science, SFU
// Description : An implementation of tcdrain-like behaviour for socketpairs.
//============================================================================

#include <sys/socket.h>
#include <unistd.h>				// for posix i/o functions
#include <termios.h>			// for tcdrain()
#include <fcntl.h>				// for open/creat
#include <errno.h>
#include <mutex>				
#include <condition_variable>	
#include <map>
#include <memory>
#include <shared_mutex>
#include "AtomicCOUT.h"
#include "SocketReadcond.h"
#include "VNPE.h"

// Pretend that we have a C++17 compiler instead of our C++14 one
#define shared_mutex shared_timed_mutex

// Uncomment the line below to turn on debugging output from the medium
//#define REPORT_INFO

using namespace std;

//Unnamed namespace
namespace{

	class socketInfoClass;

	typedef shared_ptr<socketInfoClass> socketInfoClassSp;
	map<int, socketInfoClassSp> desInfoMap = {
			{0, nullptr}, // init for stdin, stdout, stderr
			{1, nullptr},
			{2, nullptr}
	};

//	A shared mutex used to protect desInfoMap so only a single thread can modify the map at a time.
//  This also means that only one call to functions like mySocketpair() or myClose() can make progress at a time.
//  This mutex is also used to prevent a paired socket from being closed at the beginning of a myWrite or myTcdrain function.
shared_mutex mapMutex;

class socketInfoClass {
	int buffered = 0;
	condition_variable cvDrain;
	mutex socketInfoMutex;
public:
	int pair; 	// Cannot be private because myWrite and myTcdrain using it.
				// -1 when descriptor closed, -2 when paired descriptor is closed

	socketInfoClass(unsigned pairInit)
	:buffered(0), pair(pairInit) {}

	/*
	 * Function:  if necessary, make the calling thread wait for a reading thread to drain the data
	 */
	int draining()
	{ // operating on object for paired descriptor
		unique_lock<mutex> condlk(socketInfoMutex);
		cvDrain.wait(condlk, [this] {return buffered <= 0 || pair < 0 ;});
		if (pair == -2) {
			errno = EBADF; // check errno
			return -1;
		}
		return 0;
	}

	/*
	 * Function:	Writing on the descriptor and update the reading buffered
	 */
	int writing(int des, const void* buf, size_t nbyte)	{
		// operating on object for paired descriptor
		lock_guard<mutex> condlk(socketInfoMutex);
		// consider unlocking mapMutex
		int written = write(des, buf, nbyte);
		if (written != -1)
			buffered += written;
#ifdef REPORT_INFO
		COUT << " mw: " << des << "," << buffered << flush;
#endif
		return written;
	}

	/*
	 * Function:  Checking buffered and waking up the wait-for-draining threads if needed.
	 */
	int reading(int des, void * buf, int n, int min, int time, int timeout)
	{
		int bytesRead;
		unique_lock<mutex> condlk(socketInfoMutex);
		if (buffered >= min || pair < 0) {
			// wcsReadcond should not wait in this situation.
			bytesRead = wcsReadcond(des, buf, n, min, time, timeout);
			if (bytesRead > 0) {
				buffered -= bytesRead;
				if (!buffered /* && pair >= 0 */)
					cvDrain.notify_all();
			}
#ifdef REPORT_INFO
			COUT << " mrn: " << des << "," << bytesRead << "," << buffered << flush;
#endif
		}
		else {
			buffered -= min;
			cvDrain.notify_all(); // buffered must be <= 0
#ifdef REPORT_INFO
			COUT << " mrb: " << des << "," << buffered << flush;
#endif
			condlk.unlock();
			bytesRead = wcsReadcond(des, buf, n, min, time, timeout); // wait to be woken up.
			condlk.lock();

			if (bytesRead == -1) // an error occurred.
				// but be aware that a cvDrain notification will already have occurred above.
				buffered += min;
			else {
				buffered -= (bytesRead - min);
				if (!buffered /* && pair >= 0 */)
					// myTcdrain thread might have snuck in.
					cvDrain.notify_all();
			}
#ifdef REPORT_INFO
			COUT << " mra: " << des << "," << bytesRead << "," << buffered << flush;
#endif
		}
		return bytesRead;
	} // .reading()

	/*
	 * Function:  Closing des. Should be done only after all other operations on des have returned.
	 */
	int closing(int des)
	{
		// mapMutex already locked at this point, so no mySocketpair or other myClose
		if(pair != -2) { // pair has not already been closed
			socketInfoClassSp des_pair(desInfoMap[pair]);
			unique_lock<mutex> condlk(socketInfoMutex, defer_lock);
			unique_lock<mutex> condPairlk(des_pair->socketInfoMutex, defer_lock);
			lock(condPairlk, condlk);
			pair = -1; // this is first socket in the pair to be closed
			des_pair->pair = -2; // paired socket will be the second of the two to close.
			if (des_pair->buffered > 0) {
				// there shouldn't be any threads waiting in myTcdrain on des, but just in case.
				des_pair->cvDrain.notify_all();
			}
			if (buffered > 0) {
				// by closing the socket we are throwing away any buffered data.
				// notification will be sent immediately below to any myTcdrain waiters on paired descriptor.
				buffered = 0;
				cvDrain.notify_all();
			}
			return PE(close(des));
		}
		return PE(close(des)); // pair == -2
	} // .closing()
}; // socketInfoClass

// get shared pointer for des
socketInfoClassSp getDesInfoP(int des) {
	auto iter = desInfoMap.find(des);
	if (iter == desInfoMap.end())
		return nullptr; // des not in use
	else
		return iter->second; // return the shared pointer
}
} // unnamed namespace

/*
 * Function:	Calling the reading member function to read
 * Return:		An integer with number of bytes read, or -1 for an error.
 * see https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout) {
    shared_lock<shared_mutex> desInfoLk(mapMutex);
	socketInfoClassSp desInfoP = getDesInfoP(des);
	desInfoLk.unlock();
	if (!desInfoP)
		// myReadcond currently only supported on open sockets created with mySocketpair()
		// errno will probably indicate not a socket (ENOSYS) or not open (EBADF)
		return wcsReadcond(des, buf, n, min, time, timeout);
	return desInfoP->reading(des, buf, n, min, time, timeout);
}

/*
 * Function:	Reading directly from a file or from a socketpair descriptor)
 * Return:		the number of bytes read , or -1 for an error
 */
ssize_t myRead(int des, void* buf, size_t nbyte) {
    shared_lock<shared_mutex> desInfoLk(mapMutex);
	socketInfoClassSp desInfoP = getDesInfoP(des);
	desInfoLk.unlock();
	if (!desInfoP)
		return read(des, buf, nbyte); // des is closed or not from a socketpair
	// myRead (for sockets) usually reads a minimum of 1 byte
	return desInfoP->reading(des, buf, nbyte, 1, 0, 0);
}

/*
 * Return:		the number of bytes written, or -1 for an error
 */
ssize_t myWrite(int des, const void* buf, size_t nbyte) {
    shared_lock<shared_mutex> desInfoLk(mapMutex);
	socketInfoClassSp desInfoP = getDesInfoP(des);
	if(desInfoP && desInfoP->pair >= 0) {
		// locking mapMutex above makes sure that desinfoP->pair is not closed here
		auto desPairInfoSp = desInfoMap[desInfoP->pair]; // make a local shared pointer
		desInfoLk.unlock();
		// des or its pair could be closed at this point.
		return desPairInfoSp->writing(des, buf, nbyte);
	}
	desInfoLk.unlock();
	return write(des, buf, nbyte); // des is not from a pair of sockets or socket closed
}

/*
 * Function:  make the calling thread wait for a reading thread to drain the data
 */
int myTcdrain(int des) {
    shared_lock<shared_mutex> desInfoLk(mapMutex);
	socketInfoClassSp desInfoP = getDesInfoP(des);
	if(desInfoP) {
		if (desInfoP->pair == -2)
			return 0; // paired descriptor is closed.
		else { // pair == -1 won't be in *desInfoP now
			// locking mapMutex above makes sure that desinfoP->pair is not closed here
			auto desPairInfoP = desInfoMap[desInfoP->pair]; // make a local shared pointer
			desInfoLk.unlock(); // ***
			return desPairInfoP->draining(); //***
		}
	}
	desInfoLk.unlock();
	return tcdrain(des); // des is not from a pair of sockets or socket closed
}

/*
 * Function:	Open a file and get its file descriptor.
 * Return:		return value of open
 */
int myOpen(const char *pathname, int flags, mode_t mode)
{
	lock_guard<shared_mutex> desInfoLk(mapMutex);
	int des = open(pathname, flags, mode);
	if (des != -1)
		desInfoMap[des] = nullptr;
	return des;
}

/*
 * Function:	Create a new file and get its file descriptor.
 * Return:		return value of creat
 */
int myCreat(const char *pathname, mode_t mode)
{
	lock_guard<shared_mutex> desInfoLk(mapMutex);
	int des = creat(pathname, mode);
	if (des != -1)
		desInfoMap[des] = nullptr;
	return des;
}

/*
 * Function:	Create pair of sockets and put them in desInfoMap
 * Return:		return an integer that indicate if it is successful (0) or not (-1)
 */
int mySocketpair(int domain, int type, int protocol, int des[2]) {
	lock_guard<shared_mutex> desInfoLk(mapMutex);
	int returnVal = socketpair(domain, type, protocol, des);
	if(returnVal != -1) {
		desInfoMap[des[0]] = make_shared<socketInfoClass>(des[1]);
		desInfoMap[des[1]] = make_shared<socketInfoClass>(des[0]);
	}
	return returnVal;
}

/*
 * Function:	Closing des
 * 		myClose() should not be called until all other calls using the descriptor have finished.
 */
int myClose(int des) {
	int retVal = 1;
	lock_guard<shared_mutex> desInfoLk(mapMutex);
	auto iter = desInfoMap.find(des);
	if (iter != desInfoMap.end()) { // if in the map
		if (iter->second) // if shared pointer exists
			retVal = iter->second->closing(des); // -1 or 0
		desInfoMap.erase(des);
	}
	if (retVal == 1) // if not-in-use or from a socketpair //***
		retVal = close(des);
	return retVal;
}

