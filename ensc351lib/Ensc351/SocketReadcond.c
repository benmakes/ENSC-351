/* Additional QNX-like functions.  Simon Fraser University -- Copyright (C) 2008-2019, By
 *  - Craig Scratchley
 *  - Zhenwang Yao.
 */

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#endif

#include <sys/socket.h>
#include <sys/time.h>	// for timeval ???
#include <stdio.h>	    // fprintf()
#include <fcntl.h>
#include <errno.h>
#include "VNPE.h"

// a version of readcond() that works with socket(pair)s and, on QNX, terminal devices
int wcsReadcond( int fd,
              void * buf,
              int n,
              int min,
              int time,
              int timeout )
{
    //int bytesSoFar = 0;
    int bytesRead;
    struct timeval tv, tvHold;
    int minHold;
    socklen_t lenMinHold;
    socklen_t tvLenHold=sizeof(tvHold);

    if (-1 == getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tvHold, &tvLenHold)) {
#ifdef __QNXNTO__
    	if (errno == ENOTSOCK) /* Socket operation on non-socket - documented? */
    		// the fd is not a socket, try normal readcond
    		return readcond( fd, buf, n, min, time, timeout);
    	else
#endif
    	{
    		// some errors, like bad descriptor, should just be returned.
    		//VNS_errorReporter ("1st getsockopt() in wcsReadcond()", __FILE__, __func__, __LINE__, errno, NULL);
    		return -1;
    	};
    }
    if (time != timeout) {
    	fprintf(stderr, "wcsReadcond() requires for sockets that time == timeout\n");
    	errno = EINVAL;
    	return -1;
    };

    lenMinHold=sizeof(minHold);
    PE(getsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &minHold, &lenMinHold)); // should we return if -1?

    if (min != 0) {
    	if (time != 0) {
		    // add timeout to read() call
		   	tv.tv_sec = time/10;
		   	tv.tv_usec = time%10 * 100000;
		   	PE(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));
    	};

	    // set socket to grab min bytes before unblocking
	    PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &min, sizeof(min)));

		do {
			// need to update timeout if looping back from EINTR
			bytesRead = read(fd, buf, n);
		}
		while ((bytesRead == -1 && errno == EINTR));
    }
	if( min == 0 || (bytesRead == -1 && errno == EWOULDBLOCK) ) { // do we have to check if nonblocking?
		//timeout occurred, attempt to read everything off the socket with
		//a non-blocking read()
		//if nothing is in the socket, timeout will trigger and set bytesRead to -1

		int lowat=1; // lowat = 0 doesn't stop the need for non-blocking, at least on Linux.
		int flags;
		PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &lowat, sizeof(lowat))); // should we return if -1?

		//set non-blocking
		if( -1==(flags=PE(fcntl(fd, F_GETFL, 0))) ) {
    		return -1;  // if PE doesn't exit()
		}
		if( -1 == PE(fcntl(fd, F_SETFL, flags | O_NONBLOCK)) ) {
    		return -1;  // if PE doesn't exit()
		}

		bytesRead = read(fd, buf, n);
		if( bytesRead == -1)
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				bytesRead = 0;

		//set blocking
		if( -1 == PE(fcntl(fd, F_SETFL, flags)) ) {
    		return -1;  // if PE doesn't exit()
		}
	}

	//reset the socket
	PE(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tvHold, sizeof(tvHold)));
	PE(setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &minHold, sizeof(minHold)));

   	return bytesRead;
}

